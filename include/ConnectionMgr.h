#ifndef _CONNECTION_MGR_H_
#define _CONNECTION_MGR_H_

#include "socket_include.h"
#include "TcpConnection.h"

namespace core_socket{

typedef std::function<TcpConnection*(int sock)> ConnectionFactoryFunc;

//���ӹ�������
//��Ҫ����:�����������ӵ�Զ�̶˿�,��������Ϊ����ʽ�ӿ�,����Ϊ������
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

	//��ʼ����
	bool Listen(std::string host,int port,ConnectionFactoryFunc creator);
	//����ʽ����
	bool Connection(std::string host,int port,ConnectionFactoryFunc creator,int trytimes = 5);

	void Select(int ms);	
	void Put(TcpConnection *conn){opening_sockets_.push_back(conn);};
	void Log(TcpConnection *p,const std::string& user_text,int err,const std::string& sys_err,loglevel_t t = LOG_LEVEL_WARNING);

	//��ǰ��������
	int GetCount(){return conntions_.size()+opening_sockets_.size();}
	//�ر����ӹ�����
	void Close();
protected:
	bool Remove(TcpConnection *conn);
	void Insert(TcpConnection *conn);			

	//�Ѿ�����
	ConnectionVec conntions_;
	//�������¼���ѯ������������
	ConnectionVec opening_sockets_;
	//���ر�����
	ConnectionVec closing_sockets_;

	fd_set	read_set_;
	fd_set	write_set_;	
	fd_set	error_set_;
};


}
#endif
