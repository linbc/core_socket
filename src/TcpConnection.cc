#include "TcpConnection.h"
#include "ConnectionMgr.h"

namespace core_socket
{

//////////////////////////////////////////////////////////////////////////
//for TcpConnection
TcpConnection::TcpConnection():fd_(0),mgr_(0)
{
}

TcpConnection::~TcpConnection()
{
	if (fd_ != INVALID_SOCKET){
		closesocket(fd_);
		fd_ = 0;
	}
}

bool TcpConnection::OnRead()
{
	int n = recv(fd_,tmp_buf_,TCP_BUFSIZE_READ, MSG_NOSIGNAL);
	if(n > 0){
		return true;
	}
	if(n == -1){
		mgr_->Log(this, "read", Errno, StrError(Errno), LOG_LEVEL_FATAL);		
	}else if(n == 0){

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
				return false;
			}
		}
		last = send_buf_.add_read(n);
	}
	return true;
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

}
