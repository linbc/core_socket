#include <gtest/gtest.h>
#include "ConnectionMgr.h"

namespace core_socket
{

//class RotateSocket:public TcpConnection
//{
//public:
//	RotateSocket()
//	{
//	};
//	virtual ~RotateSocket()
//	{
//	};
//
//	void creator(ConnectionFactoryFunc f){creator_ = f;}
//
//protected:
//	virtual bool OnRead()
//	{
//		struct sockaddr_in sa;
//		socklen_t sa_len = sizeof(struct sockaddr);
//		int a_s = accept(fd_, (struct sockaddr *)&sa, &sa_len);
//
//		if (a_s == INVALID_SOCKET)
//		{
//			mgr_->Log(this, "accept", Errno, StrError(Errno), LOG_LEVEL_ERROR);
//			return true;
//		}
//		if (mgr_->GetCount() >= FD_SETSIZE)
//		{
//			mgr_->Log(this, "accept", mgr_->GetCount(), "ISocketHandler fd_set limit reached", LOG_LEVEL_FATAL);
//			closesocket(a_s);
//			return true;
//		}
//		fd_ = a_s;
//		TcpConnection::OnRead();
//		Send(tmp_buf_, strlen(tmp_buf_));
//
//		return true;
//	}
//	//virtual void OnClose();
//	ConnectionFactoryFunc creator_;
//};

	//for RotateSocket

	//UINT32 old_data_len = 0; //发出去的字符长度
	//UINT32 new_data_len = 0; //返回的字符长度

string src_data;	//原始字符串
string c_come_data;
string s_return_data;
void Handle_Client_Come(TcpConnection *sender,PacketType *pkt)
{
	int fd = 0;
	*pkt >> c_come_data;
	if (sender)
		sender->SendPacket(*pkt);
	int a = 0;
}

void Handle_Server_Return(PacketType *&pkt)
{
	short old_len = 0;
	string data;
	*pkt >> old_len;
	*pkt >> s_return_data;
	int a = 0;
}

TcpConnection* FuncServer(int fd)
{
	TcpConnection* conn = new TcpConnection();
	conn->onPacket([conn](PacketType*& pkt){
		Handle_Client_Come(conn,pkt);
	});

	return conn;
}

TcpConnection* FunClient(int fd)
{
	TcpConnection* conn = new TcpConnection();
	src_data = "client send";
	PacketType pkt;
	pkt.writeUTF(src_data);
	//pkt.writeBytes((uint8_t*)_data.c_str(),_data.length()+1);
	conn->SendPacket(pkt);
	conn->onPacket(Handle_Server_Return);
	return conn;
}



TEST(core_socket,ConnectionMgr)
{
	ConnectionMgr serverMgr;
	ConnectionMgr clientOne;//, clientTwo;

	serverMgr.Listen("127.0.0.1",5999, &FuncServer);
	clientOne.Connection("127.0.0.1",5999,&FunClient);	

	clientOne.Select(1000);
	serverMgr.Select(1000);
	serverMgr.Select(1000);
	clientOne.Select(1000);

	//判断发包一个来回后字符串是否还是一样
	ASSERT_TRUE(src_data.compare(c_come_data) == 0);
	ASSERT_TRUE(src_data.compare(s_return_data) == 0);

}

}

