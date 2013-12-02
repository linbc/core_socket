/** \file ISocketHandler.h
 **	\date  2004-02-13
 **	\author grymse@alhem.net
**/
/*
Copyright (C) 2004-2007  Anders Hedstrom

This library is made available under the terms of the GNU GPL.

If you would like to use this library in a closed-source application,
a separate license agreement is available. For information about 
the closed-source license agreement for the C++ sockets library,
please visit http://www.alhem.net/Sockets/license.html and/or
email license@alhem.net.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef _SOCKETS_ISocketHandler_H
#define _SOCKETS_ISocketHandler_H
#include "sockets-config.h"

#include <list>

#include "socket_include.h"
#include "Socket.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

typedef enum {
	LIST_CALLONCONNECT = 0,
#ifdef ENABLE_DETACH
	LIST_DETACH,
#endif
	LIST_TIMEOUT,
	LIST_RETRY,
	LIST_CLOSE
} list_t;

class Ipv4Address;
class Mutex;


/** Socket container class, event generator. 
	\ingroup basic */
class ISocketHandler
{
	friend class Socket;

public:
	/** Connection pool class for internal use by the ISocketHandler. 
		\ingroup internal */
#ifdef ENABLE_POOL
	class PoolSocket : public Socket
	{
	public:
		PoolSocket(ISocketHandler& h,Socket *src) : Socket(h) {
			CopyConnection( src );
			SetIsClient();
		}

		void OnRead() {
			Handler().LogError(this, "OnRead", 0, "data on hibernating socket", LOG_LEVEL_FATAL);
			SetCloseAndDelete();
		}
		void OnOptions(int,int,int,SOCKET) {}

	};
#endif

public:
	virtual ~ISocketHandler() {}

	/** Log error to log class for print out / storage. */
	virtual void LogError(Socket *p,const std::string& user_text,int err,const std::string& sys_err,loglevel_t t = LOG_LEVEL_WARNING) = 0;

	// -------------------------------------------------------------------------
	// Socket stuff
	// -------------------------------------------------------------------------
	/** Add socket instance to socket map. Removal is always automatic. */
	virtual void Add(Socket *) = 0;
private:
	/** Remove socket from socket map, used by Socket class. */
	virtual void Remove(Socket *) = 0;
public:
	/** Get status of read/write/exception file descriptor set for a socket. */
	virtual void Get(SOCKET s,bool& r,bool& w,bool& e) = 0;
	/** Set read/write/exception file descriptor sets (fd_set). */
	virtual void Set(SOCKET s,bool bRead,bool bWrite,bool bException = true) = 0;

	/** Wait for events, generate callbacks. */
	virtual int Select(long sec,long usec) = 0;
	/** This method will not return until an event has been detected. */
	virtual int Select() = 0;
	/** Wait for events, generate callbacks. */
	virtual int Select(struct timeval *tsel) = 0;

	/** Check that a socket really is handled by this socket handler. */
	virtual bool Valid(Socket *) = 0;
	/** Return number of sockets handled by this handler.  */
	virtual size_t GetCount() = 0;

	/** Override and return false to deny all incoming connections. 
		\param p ListenSocket class pointer (use GetPort to identify which one) */
	virtual bool OkToAccept(Socket *p) = 0;

	/** Called by Socket when a socket changes state. */
	virtual void AddList(SOCKET s,list_t which_one,bool add) = 0;

	// -------------------------------------------------------------------------
	// Connection pool
	// -------------------------------------------------------------------------
#ifdef ENABLE_POOL
	/** Find available open connection (used by connection pool). */
	virtual ISocketHandler::PoolSocket *FindConnection(int type,const std::string& protocol,Ipv4Address&) = 0;
	/** Enable connection pool (by default disabled). */
	virtual void EnablePool(bool = true) = 0;
	/** Check pool status. 
		\return true if connection pool is enabled */
	virtual bool PoolEnabled() = 0;
#endif // ENABLE_POOL
};


#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_ISocketHandler_H

