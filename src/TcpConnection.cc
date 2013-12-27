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
	//缓冲区的
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
		// 这里表示对端的socket已正常关闭.
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
			//连接关闭
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
	//没东西可读了		;
	//可以解包了
	if (tmp_buf_count_ >= 2){			
		//判断一下缓冲区要不要重搞,只有两种情况,0或者是1
		if(ComplatePacket((uint8_t*)tmp_buf_,tmp_buf_count_)){
			//如果还剩下一个字节，移动到缓冲区的前面
			tmp_buf_[0] = tmp_buf_[tmp_buf_count_-1];
			tmp_buf_count_ = 0;
		}
	}
}

int TcpConnection::ComplatePacket(uint8_t *buf,int len)
{
	//用两个字节定义包长
	const int PACKET_HEAD_SIZE = sizeof(short);

	int total = 0;
	//剩下的大于指示包长的就可以读
	while (len > PACKET_HEAD_SIZE)
	{
		if(!cur_pkt_)
			cur_pkt_ = new PacketType;

		//未读取包头
		if(cur_pkt_->length() < PACKET_HEAD_SIZE){
			//如果传入的数据还不够两个字节读取包头,如果就
			if(len < PACKET_HEAD_SIZE)
				return 0;
			else {
				//写在末尾
				cur_pkt_->position(cur_pkt_->length());
				cur_pkt_->writeBytes(buf,PACKET_HEAD_SIZE);				

				//缓冲区偏移
				total += PACKET_HEAD_SIZE;				
				buf += PACKET_HEAD_SIZE;
				len -= PACKET_HEAD_SIZE;
			}
		}

		int packet_size = cur_pkt_->readUnsignedShort(0);
		//未读取整个完整包之前中间不可能会有对包的位置进行操作的位置
		if(cur_pkt_->position() != cur_pkt_->length())
			throw std::exception("cur_pkt_->position()!=cur_pkt_->length()");
		//怎么可能整包已经解完了,居然还没有调用处理函数，肯定是有异常
		if(cur_pkt_->length() >= packet_size)
			throw std::exception("cur_pkt_->length() >= packet_size");

		//这一次可以读取的值，是可以读和未读的剩下的部分
		int readlen = std::min(len,packet_size - cur_pkt_->length());

		cur_pkt_->position(cur_pkt_->length());
		cur_pkt_->writeBytes(buf,readlen);		

		//可以偏移了
		total += readlen;				
		buf += readlen;
		len -= readlen;

		//如果已经是完整包了，就可以调用处理函数并且根据返回值是否有
		if(cur_pkt_->length() == packet_size){
			//调用处理函数，如果有返回数据包就清空后使用,如果没有返回就构造一个新的
			if(onPacket_){
				//跳过包长
				cur_pkt_->position(PACKET_HEAD_SIZE);
				onPacket_(cur_pkt_);
			}
			if(cur_pkt_ == nullptr)
				cur_pkt_ = new PacketType();
			else
				cur_pkt_->clear();
		}		
	}	

	//已读取包头
	return len;
}



}


