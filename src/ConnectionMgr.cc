#include "ConnectionMgr.h"

namespace core_socket{

//////////////////////////////////////////////////////////////////////////
//for ListenSocket
class ListenSocket:public TcpConnection
{
public:
	ListenSocket();
	virtual ~ListenSocket();

protected:
	virtual bool OnRead()
	{
		struct sockaddr_in sa;
		socklen_t sa_len = sizeof(struct sockaddr);
		int a_s = accept(fd_, (struct sockaddr *)&sa, &sa_len);

		if (a_s == INVALID_SOCKET)
		{
			mgr_->Log(this, "accept", Errno, StrError(Errno), LOG_LEVEL_ERROR);
			return true;
		}
		if (mgr_->GetCount() >= FD_SETSIZE)
		{
			mgr_->Log(this, "accept", mgr_->GetCount(), "ISocketHandler fd_set limit reached", LOG_LEVEL_FATAL);
			closesocket(a_s);
			return true;
		}
		TcpConnection *conn = creator_(mgr_);
		conn->Attach(a_s);
		conn->remote_addr(sa);
		conn->SetNonblocking(true);
		mgr_->Put(conn);
		return true;
	}
	virtual void OnClose();
	ConnectionFactoryFunc creator_;
};

//////////////////////////////////////////////////////////////////////////
//for ConnectionMgr

TcpConnection *ConnectionMgr::Get(int fd)
{
	auto it = std::lower_bound(conntions_.begin(),conntions_.end(),fd,tcpconnection_fd_cmp_func());
	if(it != conntions_.end() && (*it)->fd_ == fd)
		return *it;
	return NULL;
}

bool ConnectionMgr::Remove(TcpConnection *conn)
{
	auto it = std::lower_bound(conntions_.begin(),conntions_.end(),conn->fd_,tcpconnection_fd_cmp_func());
	if(it != conntions_.end() && (*it) == conn){		
		conntions_.erase(it);
		return true;
	}
	return false;
}

void ConnectionMgr::Insert(TcpConnection *conn)
{
	auto it = std::lower_bound(conntions_.begin(),conntions_.end(),conn->fd_,tcpconnection_fd_cmp_func());
	conntions_.insert(it,conn);
}

bool ConnectionMgr::Listen(std::string host,int port,ConnectionFactoryFunc creator)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
		return false;
	
	int yes = 1;
	if (setsockopt(s , SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int)) == -1) {
		return false;
	}

	/*绑定地址*/
	struct sockaddr_in server_addr;				/*server address information*/
	server_addr.sin_family = AF_INET;			/*host byte order*/
	server_addr.sin_port = htons(port);			/*short, network byte order*/
	//server_addr.sin_addr.s_addr = INADDR_ANY;	/*automatically fill with my IP*/
	server_addr.sin_addr.s_addr = inet_addr(host.c_str());
	memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
	if (bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		return false;
	}

	if (listen(s, 20) == -1)
		return false;

	return true;
}

bool ConnectionMgr::Connection(std::string host,int port,ConnectionFactoryFunc creator,int trytimes /* = 5 */)
{	
	//解析出host及端口	
	struct hostent *he;	
	if((he = gethostbyname(host.c_str()))==NULL){
		return false;
	}

	//远程地址
	struct sockaddr_in _addr; 
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
	_addr.sin_addr = *((struct in_addr *)he->h_addr);

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
		return false;

	//设置超时
	struct timeval tv = {2,0};
	setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(tv));
	setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(char*)&tv,sizeof(tv));
	
	//阻塞式连接
	do 
	{
		int n = connect(s,(struct sockaddr*)&_addr,sizeof(_addr));
		if(n == -1){
			// check error code that means a connect is in progress
			//#ifdef _WIN32
			//		if (Errno != WSAEWOULDBLOCK)
			//#else
			//		if (Errno != EINPROGRESS)
			//#endif
			//		{
			//		}
			return false;
		}
		//重试n次后成功,否则返回失败
		if(n == 0) break;
	} while (trytimes-- > 0);

	TcpConnection *conn = creator(this);
	conn->Attach(s);
	conn->remote_addr(_addr);
	conn->SetNonblocking(true);
	Put(conn);	//下一帧开始接收数据

	return true;
}

void ConnectionMgr::Select(int ms)
{
	//先处理需要主动关闭的连接
	//触发关闭事件后从连接容器中移除并释放内存
	for (auto conn:closing_sockets_) {
		conn->OnClose();		
		Remove(conn);
		delete conn;
	}
	closing_sockets_.clear();

	//插入到已连接列表	
	for (auto conn:opening_sockets_)	
		Insert(conn);
	opening_sockets_.clear();	
	
	if(conntions_.empty())
		return;

	//开始查询了。。
	FD_ZERO(&read_set_);
	FD_ZERO(&write_set_);
	FD_ZERO(&error_set_);
	
	for (auto conn:conntions_){
		FD_SET(conn->fd_,&read_set_);
		FD_SET(conn->fd_,&write_set_);
		FD_SET(conn->fd_,&error_set_);		
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000*ms;
	int ret =  select((int)conntions_.size(), &read_set_, &write_set_, &error_set_, &tv);

	if(ret < 0 && errno != 0){
		fprintf (stderr, " select error . errno=%d [%s]\n", errno,strerror (errno));
		//return ret;
	}

	for (auto conn:conntions_){
		if (FD_ISSET(conn->fd_, &read_set_))
			conn->OnRead();
		if (FD_ISSET(conn->fd_, &write_set_))
			conn->OnWrite();
		if (FD_ISSET(conn->fd_, &read_set_))
			conn->OnError();
	}
}

}
