#include "TcpConnection.h"
#include "ConnectionMgr.h"

namespace core_socket
{

//////////////////////////////////////////////////////////////////////////
//for TcpConnection
TcpConnection::TcpConnection():fd_(0),mgr_(0),tmp_buf_count_(0)
{
	cur_pkt_ = new PacketType;
}

TcpConnection::~TcpConnection()
{
	if (fd_ != INVALID_SOCKET){
		closesocket(fd_);
		fd_ = 0;
	}
	if(cur_pkt_){
		delete cur_pkt_;
		cur_pkt_ = nullptr;
	}
}

bool TcpConnection::OnRead()
{	
	//��������
	int n = recv(fd_,tmp_buf_+tmp_buf_count_,TCP_BUFSIZE_READ-tmp_buf_count_, MSG_NOSIGNAL);
	if(n > 0){		
		tmp_buf_count_ += n;		
		return OnRead();
	} else if(n == -1) {
#if WIN32
		if(Errno == WSAEWOULDBLOCK )
#else
		if(Errno == EWOULDBLOCK || Errno == EAGAIN)
#endif
		{
			ReadPacket();
			return true;
		}
		mgr_->Log(this, "read", Errno, StrError(Errno), LOG_LEVEL_FATAL);		
	}else if(n == 0){
		// ������ʾ�Զ˵�socket�������ر�.
		return false;
	}else{
		mgr_->Log(this, "OnRead", n, "abnormal value from recv", LOG_LEVEL_ERROR);		
	}	
	return false;
}

bool TcpConnection::OnWrite()
{
	int last = send_buf_.length();
	while (last > 0) {
		int n = send(fd_,send_buf_.data(),last,MSG_NOSIGNAL);
		if (n == -1){
			// normal error codes:
			// WSAEWOULDBLOCK
			//       EAGAIN or EWOULDBLOCKdata
#ifdef _WIN32
			if (Errno != WSAEWOULDBLOCK)
#else
			if (Errno != EWOULDBLOCK)
#endif
			{
				mgr_->Log(this,"OnWrite", n, "abnormal value from send",LOG_LEVEL_ERROR);
				return false;
			}
		}else if(n == 0){
			//���ӹر�
			return false;
		}
		last = send_buf_.add_read(n);
	}
	return true;
}

void TcpConnection::OnClose()
{	
	shutdown(fd_,SD_BOTH);
	closesocket(fd_);  
	std::stringstream ss;
	ss << remote_ip() << ":" << remote_port();
	mgr_->Log(this,"TcpConnection::OnClose",0,ss.str(),LOG_LEVEL_INFO);
}

void TcpConnection::OnError()
{
	int err = 0;
	socklen_t len = sizeof(err);
	if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, (char *)&err, &len) == -1)
	{
		mgr_->Log(this, "getsockopt(SOL_SOCKET, SO_ERROR)", Errno, StrError(Errno), LOG_LEVEL_FATAL);
		return;
	}
	mgr_->Log(this, "exception on select", err, StrError(err), LOG_LEVEL_FATAL);
}

bool TcpConnection::SetNonblocking(bool bNb)
{
#ifdef _WIN32
	unsigned long l = bNb ? 1 : 0;
	int n = ioctlsocket(fd_, FIONBIO, &l);
	if (n != 0)
	{
		mgr_->Log(this, "ioctlsocket(FIONBIO)", Errno, "");
		return false;
	}
	return true;
#else
	if (bNb)
	{
		if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1)
		{
			mgr_->Log(this, "fcntl(F_SETFL, O_NONBLOCK)", Errno, StrError(Errno), LOG_LEVEL_ERROR);
			return false;
		}
	}
	else
	{
		if (fcntl(fd_ F_SETFL, 0) == -1)
		{
			mgr_->Log(this, "fcntl(F_SETFL, 0)", Errno, StrError(Errno), LOG_LEVEL_ERROR);
			return false;
		}
	}
	return true;
#endif
}

void TcpConnection::ReadPacket()
{
	//û�����ɶ���		;
	//���Խ�����
	if (tmp_buf_count_ >= 2){			
		//�ж�һ�»�����Ҫ��Ҫ�ظ�,ֻ����������,0������1
		if(ComplatePacket((uint8_t*)tmp_buf_,tmp_buf_count_)){
			//������ʣ��һ���ֽڣ��ƶ�����������ǰ��
			tmp_buf_[0] = tmp_buf_[tmp_buf_count_-1];
			tmp_buf_count_ = 0;
		}
	}
}

int TcpConnection::ComplatePacket(uint8_t *buf,int len)
{
	//�������ֽڶ�������
	const int PACKET_HEAD_SIZE = sizeof(short);

	int total = 0;
	//ʣ�µĴ���ָʾ�����ľͿ��Զ�
	while (len > PACKET_HEAD_SIZE){
		if(!cur_pkt_)
			cur_pkt_ = new PacketType;

		//δ��ȡ��ͷ
		if(cur_pkt_->length() < PACKET_HEAD_SIZE){
			//�������������ݻ����������ֽڶ�ȡ��ͷ,������
			if(len < PACKET_HEAD_SIZE)
				return 0;
			else {
				//д��ĩβ
				cur_pkt_->position(cur_pkt_->length());
				cur_pkt_->writeBytes(buf,PACKET_HEAD_SIZE);				

				//������ƫ��
				total += PACKET_HEAD_SIZE;				
				buf += PACKET_HEAD_SIZE;
				len -= PACKET_HEAD_SIZE;
			}
		}

		int packet_size = cur_pkt_->readUnsignedShort(0);
		//δ��ȡ����������֮ǰ�м䲻���ܻ��ж԰���λ�ý��в�����λ��
		if(cur_pkt_->position() != cur_pkt_->length())
			throw std::exception("cur_pkt_->position()!=cur_pkt_->length()");
		//��ô���������Ѿ�������,��Ȼ��û�е��ô����������϶������쳣
		if(cur_pkt_->length() >= packet_size)
			throw std::exception("cur_pkt_->length() >= packet_size");

		//��һ�ο��Զ�ȡ��ֵ���ǿ��Զ���δ����ʣ�µĲ���
		int readlen = std::min(len,packet_size - cur_pkt_->length());

		cur_pkt_->position(cur_pkt_->length());
		cur_pkt_->writeBytes(buf,readlen);		

		//����ƫ����
		total += readlen;				
		buf += readlen;
		len -= readlen;

		//�����Ѿ����������ˣ��Ϳ��Ե��ô����������Ҹ��ݷ���ֵ�Ƿ���
		if(cur_pkt_->length() == packet_size){
			//���ô��������������з������ݰ������պ�ʹ��,����û�з��ؾ͹���һ���µ�
			if(onPacket_){
				//��������
				cur_pkt_->position(PACKET_HEAD_SIZE);
				onPacket_(cur_pkt_);
			}
			if(cur_pkt_ == nullptr)
				cur_pkt_ = new PacketType();
			else
				cur_pkt_->clear();
		}		
	}	

	//�Ѷ�ȡ��ͷ
	return len;
}



}


