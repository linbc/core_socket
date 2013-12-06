#ifndef _TCP_CONNECTION_H_
#define _TCP_CONNECTION_H_

#include "socket_include.h"

#define TCP_BUFSIZE_READ 16400

namespace core_socket
{

class ConnectionMgr;

//tcp连接及其处理
class TcpConnection
{
	friend class ConnectionMgr;
public:	
	//用于输出的内存缓冲区
	struct OutputBuffer {
		OutputBuffer():read_pos_(0){}
		void reset() {
			read_pos_ = 0;			
			buf_.clear();
		}
		//可读取的下一个位置
		inline char *data(){
			return buf_.data() + read_pos_;
		}
		//增加已读数量并返回还有多少未读
		int add_read(int s){
			read_pos_ += s;
			if(buf_.size() == read_pos_)
				reset();
			return length();
		}
		//还有未读取的数量
		inline int length(){
			if(buf_.empty())
				return 0;
			return buf_.size() - read_pos_;
		}
		//将内存值写入缓冲区
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
	//发送数据到远程(先进缓冲区)
	void Send(const char *ptr,int size){
		send_buf_.write(ptr,size);
	}
	void Attach(int s){		
		fd_ = s;
	}
	//主动关闭连接
	void Close(){
		closing_++;
	}
	// Set socket non-block operation. 
	bool SetNonblocking(bool bNb);
	//远程ip/端口
	void remote_addr(const sockaddr_in& addr){remote_addr_ = addr;}
	//远程IP
	string remote_ip(){return inet_ntoa(remote_addr_.sin_addr);}
	//端口
	uint16_t remote_port(){return ntohs(remote_addr_.sin_port);}

protected:
	int fd_;
	ConnectionMgr *mgr_;
	virtual bool OnRead();
	virtual void OnClose();
	bool OnWrite();	
	void OnError();	

	OutputBuffer send_buf_;
	struct sockaddr_in remote_addr_;

	//尝试关闭
	int closing_;

private:
	char tmp_buf_[TCP_BUFSIZE_READ];
};

}


#endif
