#ifndef _TCP_CONNECTION_H_
#define _TCP_CONNECTION_H_

#include "socket_include.h"
#include <core_obj/ByteArray.h>
#include <gtest/gtest.h>

typedef core_obj::ByteArray PacketType;

#define TCP_BUFSIZE_READ 16400

namespace core_socket
{

class ConnectionMgr;

typedef std::function<void (PacketType*&)> PacketHandler;

//tcp���Ӽ��䴦��
class TcpConnection
{
	FRIEND_TEST(core_socket,ComplatePacket);
	friend class ConnectionMgr;
public:	
	//����������ڴ滺����
	struct OutputBuffer {
		OutputBuffer():read_pos_(0){}
		void reset() {
			read_pos_ = 0;			
			buf_.clear();
		}
		//�ɶ�ȡ����һ��λ��
		inline char *data(){
			return buf_.data() + read_pos_;
		}
		//�����Ѷ����������ػ��ж���δ��
		int add_read(int s){
			read_pos_ += s;
			if(buf_.size() == read_pos_)
				reset();
			return length();
		}
		//����δ��ȡ������
		inline int length(){
			if(buf_.empty())
				return 0;
			return buf_.size() - read_pos_;
		}
		//���ڴ�ֵд�뻺����
		inline void write(const char *ptr,int size){
			int old_size = buf_.size();
			buf_.resize(old_size+size);
			memcpy(buf_.data()+old_size,ptr,size);
		}		
	private:
		size_t read_pos_;		
		std::vector<char> buf_;
	};

	TcpConnection();
	virtual ~TcpConnection();
	//�������ݵ�Զ��(�Ƚ�������)
	void Send(const char *ptr,int size){
		send_buf_.write(ptr,size);
	}
	void SendPacket(PacketType& pkt){
		uint16_t len = pkt.length()+sizeof(len);
		send_buf_.write((char*)&len,sizeof(len));
		send_buf_.write(pkt.data(),pkt.length());
	}
	void Attach(int s){		
		fd_ = s;
	}
	//�����ر�����
	void Close(){
		closing_++;
	}
	// Set socket non-block operation. 
	bool SetNonblocking(bool bNb);
	//Զ��ip/�˿�
	void remote_addr(const sockaddr_in& addr){remote_addr_ = addr;}
	//Զ��IP
	string remote_ip(){return inet_ntoa(remote_addr_.sin_addr);}
	//�˿�
	uint16_t remote_port(){return ntohs(remote_addr_.sin_port);}
	//��������
	void onPacket(PacketHandler handler){onPacket_ = handler;}
	//������
	void mgr(ConnectionMgr *m){mgr_ = m;}
protected:
	int fd_;
	ConnectionMgr *mgr_;
	virtual bool OnRead();
	virtual void OnClose();
	bool OnWrite();	
	void OnError();	

	//��������,�ж��Ƿ��ѳ�Ϊһ��������������Ѿ���ɣ�
	//��ô���ð��������������ݷ���ֵ�޸����ӵĵ�ǰ���ݰ�
	//���ػ�ʣ�¶��ٵ��ֽ���δ��
	int ComplatePacket(uint8_t *buf,int len);
	void ReadPacket();

	OutputBuffer send_buf_;
	struct sockaddr_in remote_addr_;

	//���Թر�
	int closing_;

private:
	//���ڽ���tcp���ݵĻ�����
	char tmp_buf_[TCP_BUFSIZE_READ];
	//���������յ������ݳ���
	int tmp_buf_count_;

	PacketType *cur_pkt_;
	PacketHandler onPacket_;
};

}


#endif
