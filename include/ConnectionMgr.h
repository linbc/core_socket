#ifndef _CONNECTION_MGR_H_
#define _CONNECTION_MGR_H_

#include "socket_include.h"
#include "TcpConnection.h"

namespace core_socket{

typedef std::function<TcpConnection*(int sock)> ConnectionFactoryFunc;

//连接管理器：
//主要功能:监听或者连接到远程端口,其中连接为阻塞式接口,其他为非阻塞
class ConnectionMgr
{
public:	
	static WSAInitializer INIT_SOCKET;
	
	typedef std::vector<TcpConnection*> ConnectionVec;
	struct tcpconnection_fd_cmp_func{
		bool operator()(ConnectionVec::value_type& l,int fd){
			return l->fd_ < fd;
		}
	};

	ConnectionMgr();
	virtual ~ConnectionMgr();

	TcpConnection *Get(int fd);

	//开始监听
	bool Listen(std::string host,int port,ConnectionFactoryFunc creator);
	//阻塞式连接
	bool Connection(std::string host,int port,ConnectionFactoryFunc creator,int trytimes = 5);

	void Select(int ms);	
	void Put(TcpConnection *conn){opening_sockets_.push_back(conn);};
	void Log(TcpConnection *p,const std::string& user_text,int err,const std::string& sys_err,loglevel_t t = LOG_LEVEL_WARNING);

	//当前连接数量
	int GetCount(){return conntions_.size()+opening_sockets_.size();}
	//关闭连接管理器
	void Close();
protected:
	bool Remove(TcpConnection *conn);
	void Insert(TcpConnection *conn);			

	//已经连接
	ConnectionVec conntions_;
	//待加入事件查询管理器的数据
	ConnectionVec opening_sockets_;
	//待关闭连接
	ConnectionVec closing_sockets_;

	fd_set	read_set_;
	fd_set	write_set_;	
	fd_set	error_set_;
};


}
#endif
