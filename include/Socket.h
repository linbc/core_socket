/** \file Socket.h
 ** \date  2004-02-13
 ** \author grymse@alhem.net
**/
/*
Copyright (C) 2004-2007  Anders Hedstrom

This software is made available under the terms of the GNU GPL.

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
#ifndef _SOCKETS_Socket_H
#define _SOCKETS_Socket_H
#include "socket_include.h"
#include <time.h>
#include "Ipv4Address.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif


class ISocketHandler;
class Ipv4Address;


/** \defgroup basic Basic sockets */
/** Socket base class.
	\ingroup basic */
class Socket
{
	friend class ISocketHandler;

	/** Socket mode flags. */
/*
	enum {
		// Socket
		SOCK_DEL = 			0x01, ///< Delete by handler flag
		SOCK_CLOSE = 			0x02, ///< Close and delete flag
		SOCK_DISABLE_READ = 		0x04, ///< Disable checking for read events
		SOCK_CONNECTED = 		0x08, ///< Socket is connected (tcp/udp)

		SOCK_ERASED_BY_HANDLER = 	0x10, ///< Set by handler before delete
		// HAVE_OPENSSL
		SOCK_ENABLE_SSL = 		0x20, ///< Enable SSL for this TcpSocket
		SOCK_SSL = 			0x40, ///< ssl negotiation mode (TcpSocket)
		SOCK_SSL_SERVER = 		0x80, ///< True if this is an incoming ssl TcpSocket connection

		// ENABLE_IPV6
		SOCK_IPV6 = 			0x0100, ///< This is an ipv6 socket if this one is true
		// ENABLE_POOL
		SOCK_CLIENT = 			0x0200, ///< only client connections are pooled
		SOCK_RETAIN = 			0x0400, ///< keep connection on close
		SOCK_LOST = 			0x0800, ///< connection lost

		// ENABLE_SOCKS4
		SOCK_SOCKS4 = 			0x1000, ///< socks4 negotiation mode (TcpSocket)
		// ENABLE_DETACH
		SOCK_DETACH = 			0x2000, ///< Socket ordered to detach flag
		SOCK_DETACHED = 		0x4000, ///< Socket has been detached
		// StreamSocket
		STREAMSOCK_CONNECTING =		0x8000, ///< Flag indicating connection in progress

		STREAMSOCK_FLUSH_BEFORE_CLOSE = 0x010000L, ///< Send all data before closing (default true)
		STREAMSOCK_CALL_ON_CONNECT =	0x020000L, ///< OnConnect will be called next ISocketHandler cycle if true
		STREAMSOCK_RETRY_CONNECT =	0x040000L, ///< Try another connection attempt next ISocketHandler cycle
		STREAMSOCK_LINE_PROTOCOL =	0x080000L, ///< Line protocol mode flag

	};
*/

public:
	/** "Default" constructor */
	Socket(ISocketHandler&);

	virtual ~Socket();

	/** Socket class instantiation method. Used when a "non-standard" constructor
	 * needs to be used for the socket class. Note: the socket class still needs
	 * the "default" constructor with one ISocketHandler& as input parameter.
	 */
	virtual Socket *Create() { return NULL; }

	/** Returns reference to sockethandler that owns the socket. 
	If the socket is detached, this is a reference to the slave sockethandler.
	*/
	ISocketHandler& Handler() const;

	/** Returns reference to sockethandler that owns the socket. 
	This one always returns the reference to the original sockethandler,
	even if the socket is detached.
	*/
	ISocketHandler& MasterHandler() const;

	/** Called by ListenSocket after accept but before socket is added to handler.
	 * CTcpSocket uses this to create its ICrypt member variable.
	 * The ICrypt member variable is created by a virtual method, therefore
	 * it can't be called directly from the CTcpSocket constructor.
	 * Also used to determine if incoming HTTP connection is normal (port 80)
	 * or ssl (port 443).
	 */
	virtual void Init();

	/** Create a socket file descriptor.
		\param af Address family AF_INET / AF_INET6 / ...
		\param type SOCK_STREAM / SOCK_DGRAM / ...
		\param protocol "tcp" / "udp" / ... */
	SOCKET CreateSocket(int af,int type,const std::string& protocol = "");

	/** Assign this socket a file descriptor created
		by a call to socket() or otherwise. */
	void Attach(SOCKET s);

	/** Return file descriptor assigned to this socket. */
	SOCKET GetSocket();

	/** Close connection immediately - internal use.
		\sa SetCloseAndDelete */
	virtual int Close();

	/** Add file descriptor to sockethandler fd_set's. */
	void Set(bool bRead,bool bWrite,bool bException = true);

	/** Returns true when socket file descriptor is valid
		and socket is not about to be closed. */
	virtual bool Ready();

	/** Returns pointer to ListenSocket that created this instance
	 * on an incoming connection. */
	Socket *GetParent();

	/** Used by ListenSocket to set parent pointer of newly created
	 * socket instance. */
	void SetParent(Socket *);

	/** Get listening port from ListenSocket<>. */
	virtual port_t GetPort();

	/** Set socket non-block operation. */
	bool SetNonblocking(bool);

	/** Set socket non-block operation. */
	bool SetNonblocking(bool, SOCKET);

	/** Total lifetime of instance. */
	time_t Uptime();

	/** Set address/port of last connect() call. */
	void SetClientRemoteAddress(Ipv4Address&);

	/** Get address/port of last connect() call. */
	std::auto_ptr<Ipv4Address> GetClientRemoteAddress();

	/** Common interface for SendBuf used by Tcp and Udp sockets. */
	virtual void SendBuf(const char *,size_t,int = 0);

	/** Common interface for Send used by Tcp and Udp sockets. */
	virtual void Send(const std::string&,int = 0);

	/** Outgoing traffic counter. */
	virtual uint64_t GetBytesSent(bool clear = false);

	/** Incoming traffic counter. */
	virtual uint64_t GetBytesReceived(bool clear = false);

	// LIST_TIMEOUT

	/** Enable timeout control. 0=disable timeout check. */
	void SetTimeout(time_t secs);

	/** Check timeout. \return true if time limit reached */
	bool Timeout(time_t tnow);

	/** Used by ListenSocket. ipv4 and ipv6 */
	void SetRemoteAddress(Ipv4Address&);

	/** \name Event callbacks */
	//@{

	/** Called when there is something to be read from the file descriptor. */
	virtual void OnRead();
	/** Called when there is room for another write on the file descriptor. */
	virtual void OnWrite();
	/** Called on socket exception. */
	virtual void OnException();
	/** Called before a socket class is deleted by the ISocketHandler. */
	virtual void OnDelete();
	/** Called when a connection has completed. */
	virtual void OnConnect();
	/** Called when an incoming connection has been completed. */
	virtual void OnAccept();
	/** Called when a complete line has been read and the socket is in
	 * line protocol mode. */
	virtual void OnLine(const std::string& );
	/** Called on connect timeout (5s). */
	virtual void OnConnectFailed();
	/** Called when a client socket is created, to set socket options.
		\param family AF_INET, AF_INET6, etc
		\param type SOCK_STREAM, SOCK_DGRAM, etc
		\param protocol Protocol number (tcp, udp, sctp, etc)
		\param s Socket file descriptor
	*/
	virtual void OnOptions(int family,int type,int protocol,SOCKET s) = 0;
	/** Connection retry callback - return false to abort connection attempts */
	virtual bool OnConnectRetry();
#ifdef ENABLE_RECONNECT
	/** a reconnect has been made */
	virtual void OnReconnect();
#endif
	/** TcpSocket: When a disconnect has been detected (recv/SSL_read returns 0 bytes). */
	virtual void OnDisconnect();
	/** Timeout callback. */
	virtual void OnTimeout();
	/** Connection timeout. */
	virtual void OnConnectTimeout();
	//@}

	/** \name Socket mode flags, set/reset */
	//@{
	/** Set delete by handler true when you want the sockethandler to
		delete the socket instance after use. */
	void SetDeleteByHandler(bool = true);
	/** Check delete by handler flag.
		\return true if this instance should be deleted by the sockethandler */
	bool DeleteByHandler();

	// LIST_CLOSE - conditional event queue

	/** Set close and delete to terminate the connection. */
	void SetCloseAndDelete(bool = true);
	/** Check close and delete flag.
		\return true if this socket should be closed and the instance removed */
	bool CloseAndDelete();

	/** Return number of seconds since socket was ordered to close. \sa SetCloseAndDelete */
	time_t TimeSinceClose();

	/** Ignore read events for an output only socket. */
	void DisableRead(bool x = true);
	/** Check ignore read events flag.
		\return true if read events should be ignored */
	bool IsDisableRead();

	/** Set connected status. */
	void SetConnected(bool = true);
	/** Check connected status.
		\return true if connected */
	bool IsConnected();

	/** Connection lost - error while reading/writing from a socket - TcpSocket only. */
	void SetLost();
	/** Check connection lost status flag, used by TcpSocket only.
		\return true if there was an error while r/w causing the socket to close */
	bool Lost();

	/** Set flag indicating the socket is being actively deleted by the sockethandler. */
	void SetErasedByHandler(bool x = true);
	/** Get value of flag indicating socket is deleted by sockethandler. */
	bool ErasedByHandler();

	//@}

	/** \name Information about remote connection */
	//@{
	/** Returns address of remote end. */
	std::auto_ptr<Ipv4Address> GetRemoteIpv4Address();
	/** Returns address of remote end: ipv4. */
	ipaddr_t GetRemoteIP4();
	/** Returns remote port number: ipv4 and ipv6. */
	port_t GetRemotePort();
	/** Returns remote ip as string? ipv4 and ipv6. */
	std::string GetRemoteAddress();
	/** ipv4 and ipv6(not implemented) */
	std::string GetRemoteHostname();
	//@}

	/** Returns local port number for bound socket file descriptor. */
	port_t GetSockPort();
	/** Returns local ipv4 address for bound socket file descriptor. */
	ipaddr_t GetSockIP4();
	/** Returns local ipv4 address as text for bound socket file descriptor. */
	std::string GetSockAddress();
	// --------------------------------------------------------------------------
	/** @name IP options
	   When an ip or socket option is available on all of the operating systems
	   I'm testing on (linux 2.4.x, _win32, macosx, solaris9 intel) they are not
	   checked with an #ifdef below.
	   This might cause a compile error on other operating systems. */
	// --------------------------------------------------------------------------

	// IP options
	//@{

	bool SetIpOptions(const void *p, socklen_t len);
	bool SetIpTOS(unsigned char tos);
	unsigned char IpTOS();
	bool SetIpTTL(int ttl);
	int IpTTL();
	bool SetIpHdrincl(bool x = true);
	bool SetIpMulticastTTL(int);
	int IpMulticastTTL();
	bool SetMulticastLoop(bool x = true);
	bool IpAddMembership(struct ip_mreq&);
	bool IpDropMembership(struct ip_mreq&);

#ifdef IP_PKTINFO
	bool SetIpPktinfo(bool x = true);
#endif
#ifdef IP_RECVTOS
	bool SetIpRecvTOS(bool x = true);
#endif
#ifdef IP_RECVTTL
	bool SetIpRecvTTL(bool x = true);
#endif
#ifdef IP_RECVOPTS
	bool SetIpRecvopts(bool x = true);
#endif
#ifdef IP_RETOPTS
	bool SetIpRetopts(bool x = true);
#endif
#ifdef IP_RECVERR
	bool SetIpRecverr(bool x = true);
#endif
#ifdef IP_MTU_DISCOVER
	bool SetIpMtudiscover(bool x = true);
#endif
#ifdef IP_MTU
	int IpMtu();
#endif
#ifdef IP_ROUTER_ALERT
	bool SetIpRouterAlert(bool x = true);
#endif
#ifdef LINUX
	bool IpAddMembership(struct ip_mreqn&);
#endif
#ifdef LINUX
	bool IpDropMembership(struct ip_mreqn&);
#endif
	//@}

	// SOCKET options
	/** @name Socket Options */
	//@{

	bool SoAcceptconn();
	bool SetSoBroadcast(bool x = true);
	bool SetSoDebug(bool x = true);
	int SoError();
	bool SetSoDontroute(bool x = true);
	bool SetSoLinger(int onoff, int linger);
	bool SetSoOobinline(bool x = true);
	bool SetSoRcvlowat(int);
	bool SetSoSndlowat(int);
	bool SetSoRcvtimeo(struct timeval&);
	bool SetSoSndtimeo(struct timeval&);
	bool SetSoRcvbuf(int);
	int SoRcvbuf();
	bool SetSoSndbuf(int);
	int SoSndbuf();
	int SoType();
	bool SetSoReuseaddr(bool x = true);
	bool SetSoKeepalive(bool x = true);

#ifdef SO_BSDCOMPAT
	bool SetSoBsdcompat(bool x = true);
#endif
#ifdef SO_BINDTODEVICE
	bool SetSoBindtodevice(const std::string& intf);
#endif
#ifdef SO_PASSCRED
	bool SetSoPasscred(bool x = true);
#endif
#ifdef SO_PEERCRED
	bool SoPeercred(struct ucred& );
#endif
#ifdef SO_PRIORITY
	bool SetSoPriority(int);
#endif
#ifdef SO_RCVBUFFORCE
	bool SetSoRcvbufforce(int);
#endif
#ifdef SO_SNDBUFFORCE
	bool SetSoSndbufforce(int);
#endif
#ifdef SO_TIMESTAMP
	bool SetSoTimestamp(bool x = true);
#endif
#ifdef SO_NOSIGPIPE
	bool SetSoNosigpipe(bool x = true);
#endif
	//@}

	// TCP options in TcpSocket.h/TcpSocket.cpp

protected:
	/** default constructor not available */
	Socket() : m_handler(m_handler) {}
	/** copy constructor not available */
	Socket(const Socket& s) : m_handler(s.m_handler) {}

	/** assignment operator not available. */
	Socket& operator=(const Socket& ) { return *this; }

//	unsigned long m_flags; ///< boolean flags, replacing old 'bool' members

private:
	ISocketHandler& m_handler; ///< Reference of ISocketHandler in control of this socket
	SOCKET m_socket; ///< File descriptor
	bool m_bDel; ///< Delete by handler flag
	bool m_bClose; ///< Close and delete flag
	time_t m_tCreate; ///< Time in seconds when this socket was created
	Socket *m_parent; ///< Pointer to ListenSocket class, valid for incoming sockets
	bool m_b_disable_read; ///< Disable checking for read events
	bool m_connected; ///< Socket is connected (tcp/udp)
	bool m_b_erased_by_handler; ///< Set by handler before delete
	time_t m_tClose; ///< Time in seconds when ordered to close
	std::auto_ptr<Ipv4Address> m_client_remote_address; ///< Address of last connect()
	std::auto_ptr<Ipv4Address> m_remote_address; ///< Remote end address
	time_t m_timeout_start; ///< Set by SetTimeout
	time_t m_timeout_limit; ///< Defined by SetTimeout
	bool m_bLost; ///< connection lost

#ifdef _WIN32
static	WSAInitializer m_winsock_init; ///< Winsock initialization singleton class
#endif
};

#ifdef SOCKETS_NAMESPACE
}
#endif


#endif // _SOCKETS_Socket_H

