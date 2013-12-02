/** \file TcpSocket.cpp
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
#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif
#include <stdlib.h>
#else
#include <errno.h>
#endif
#include "ISocketHandler.h"
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>

#include "TcpSocket.h"
#include "Utility.h"
#include "Ipv4Address.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif


//#ifdef _DEBUG
//#define DEB(x) x
//#else
#define DEB(x) 
//#endif


// statics
#ifdef HAVE_OPENSSL
SSLInitializer TcpSocket::m_ssl_init;
#endif


// thanks, q
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
TcpSocket::TcpSocket(ISocketHandler& h) : StreamSocket(h)
,ibuf(TCP_BUFSIZE_READ)
,m_b_input_buffer_disabled(false)
,m_bytes_sent(0)
,m_bytes_received(0)
,m_skip_c(false)
#ifdef SOCKETS_DYNAMIC_TEMP
,m_buf(new char[TCP_BUFSIZE_READ + 1])
#endif
,m_obuf_top(NULL)
,m_transfer_limit(0)
,m_output_length(0)
#ifdef ENABLE_RECONNECT
,m_b_reconnect(false)
,m_b_is_reconnect(false)
#endif
{
}
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
TcpSocket::TcpSocket(ISocketHandler& h,size_t isize,size_t osize) : StreamSocket(h)
,ibuf(isize)
,m_b_input_buffer_disabled(false)
,m_bytes_sent(0)
,m_bytes_received(0)
,m_skip_c(false)
#ifdef SOCKETS_DYNAMIC_TEMP
,m_buf(new char[TCP_BUFSIZE_READ + 1])
#endif
,m_obuf_top(NULL)
,m_transfer_limit(0)
,m_output_length(0)
#ifdef ENABLE_RECONNECT
,m_b_reconnect(false)
,m_b_is_reconnect(false)
#endif
{
}
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


TcpSocket::~TcpSocket()
{
#ifdef SOCKETS_DYNAMIC_TEMP
	delete[] m_buf;
#endif
	// %! empty m_obuf
	while (m_obuf.size())
	{
		output_l::iterator it = m_obuf.begin();
		OUTPUT *p = *it;
		delete p;
		m_obuf.erase(it);
	}
}


bool TcpSocket::Open(ipaddr_t ip,port_t port,bool skip_socks)
{
	Ipv4Address ad(ip, port);
	Ipv4Address local;
	return Open(ad, local, skip_socks);
}

bool TcpSocket::Open(Ipv4Address& ad,bool skip_socks)
{
	Ipv4Address bind_ad("0.0.0.0", 0);
	return Open(ad, bind_ad, skip_socks);
}


bool TcpSocket::Open(Ipv4Address& ad,Ipv4Address& bind_ad,bool skip_socks)
{
	if (!ad.IsValid())
	{
		Handler().LogError(this, "Open", 0, "Invalid Ipv4Address", LOG_LEVEL_FATAL);
		SetCloseAndDelete();
		return false;
	}
	if (Handler().GetCount() >= FD_SETSIZE)
	{
		Handler().LogError(this, "Open", 0, "no space left in fd_set", LOG_LEVEL_FATAL);
		SetCloseAndDelete();
		return false;
	}
	SetConnecting(false);
	// if not, create new connection
	SOCKET s = CreateSocket(ad.GetFamily(), SOCK_STREAM, "tcp");
	if (s == INVALID_SOCKET)
	{
		return false;
	}
	// socket must be nonblocking for async connect
	if (!SetNonblocking(true, s))
	{
		SetCloseAndDelete();
		closesocket(s);
		return false;
	}
	SetClientRemoteAddress(ad);
	int n = 0;
	if (bind_ad.GetPort() != 0)
	{
		bind(s, bind_ad, bind_ad);
	}	
	n = connect(s, ad, ad);
	SetRemoteAddress(ad);
	if (n == -1)
	{
		// check error code that means a connect is in progress
#ifdef _WIN32
		if (Errno == WSAEWOULDBLOCK)
#else
		if (Errno == EINPROGRESS)
#endif
		{
			Attach(s);
			SetConnecting( true ); // this flag will control fd_set's
		}
		else
#ifdef ENABLE_RECONNECT
		if (Reconnect())
		{
			Handler().LogError(this, "connect: failed, reconnect pending", Errno, StrError(Errno), LOG_LEVEL_INFO);
			Attach(s);
			SetConnecting( true ); // this flag will control fd_set's
		}
		else
#endif
		{
			Handler().LogError(this, "connect: failed", Errno, StrError(Errno), LOG_LEVEL_FATAL);
			SetCloseAndDelete();
			closesocket(s);
			return false;
		}
	}
	else
	{
		Attach(s);
		SetCallOnConnect(); // ISocketHandler must call OnConnect
	}

	// 'true' means connected or connecting(not yet connected)
	// 'false' means something failed
	return true; //!Connecting();
}


bool TcpSocket::Open(const std::string &host,port_t port)
{
	ipaddr_t l;
	if (!Utility::u2ip(host,l))
	{
		SetCloseAndDelete();
		return false;
	}
	Ipv4Address ad(l, port);
	Ipv4Address local;
	return Open(ad, local);
}

void TcpSocket::OnRead()
{
	int n = 0;
#ifdef SOCKETS_DYNAMIC_TEMP
	char *buf = m_buf;
#else
	char buf[TCP_BUFSIZE_READ];
#endif
	
	n = recv(GetSocket(), buf, TCP_BUFSIZE_READ, MSG_NOSIGNAL);
	if (n == -1)
	{
		Handler().LogError(this, "read", Errno, StrError(Errno), LOG_LEVEL_FATAL);
		OnDisconnect();
		SetCloseAndDelete(true);
		SetFlushBeforeClose(false);
		SetLost();
		return;
	}
	else if (!n)
	{
		OnDisconnect();
		SetCloseAndDelete(true);
		SetFlushBeforeClose(false);
		SetLost();
		SetShutdown(SHUT_WR);
		return;
	}
	else if (n > 0 && n <= TCP_BUFSIZE_READ)
	{
		m_bytes_received += n;
		if (!m_b_input_buffer_disabled && !ibuf.Write(buf,n))
		{
			Handler().LogError(this, "OnRead", 0, "ibuf overflow", LOG_LEVEL_WARNING);
		}
	}
	else
	{
		Handler().LogError(this, "OnRead", n, "abnormal value from recv", LOG_LEVEL_ERROR);
	}
	
	//
	OnRead( buf, n );
}


void TcpSocket::OnRead( char *buf, size_t n )
{
	// unbuffered
	if (n > 0 && n <= TCP_BUFSIZE_READ)
	{
		if (LineProtocol())
		{
			buf[n] = 0;
			size_t i = 0;
			if (m_skip_c && (buf[i] == 13 || buf[i] == 10) && buf[i] != m_c)
			{
				m_skip_c = false;
				i++;
			}
			size_t x = i;
			for (; i < n && LineProtocol(); i++)
			{
				while ((buf[i] == 13 || buf[i] == 10) && LineProtocol())
				{
					char c = buf[i];
					buf[i] = 0;
					if (buf[x])
					{
						m_line += (buf + x);
					}
					OnLine( m_line );
					i++;
					m_skip_c = true;
					m_c = c;
					if (i < n && (buf[i] == 13 || buf[i] == 10) && buf[i] != c)
					{
						m_skip_c = false;
						i++;
					}
					x = i;
					m_line = "";
				}
				if (!LineProtocol())
				{
					break;
				}
			}
			if (!LineProtocol())
			{
				if (i < n)
				{
					OnRawData(buf + i, n - i);
				}
			}
			else
			if (buf[x])
			{
				m_line += (buf + x);
			}
		}
		else
		{
			OnRawData(buf, n);
		}
	}
	if (m_b_input_buffer_disabled)
	{
		return;
	}
}


void TcpSocket::OnWriteComplete()
{
}


void TcpSocket::OnWrite()
{
	if (Connecting())
	{
		int err = SoError();

		// don't reset connecting flag on error here, we want the OnConnectFailed timeout later on
		if (!err) // ok
		{
			Set(!IsDisableRead(), false);
			SetConnecting(false);
			SetCallOnConnect();
			return;
		}
		Handler().LogError(this, "tcp: connect failed", err, StrError(err), LOG_LEVEL_FATAL);
		Set(false, false); // no more monitoring because connection failed

		if (GetConnectionRetry() == -1 ||
			(GetConnectionRetry() && GetConnectionRetries() < GetConnectionRetry()) )
		{
			// even though the connection failed at once, only retry after
			// the connection timeout.
			// should we even try to connect again, when CheckConnect returns
			// false it's because of a connection error - not a timeout...
			return;
		}
		SetConnecting(false);
		SetCloseAndDelete( true );
		/// \todo state reason why connect failed
		OnConnectFailed();
		return;
	}
	// try send next block in buffer
	// if full block is sent, repeat
	// if all blocks are sent, reset m_wfds

	bool repeat = false;
	size_t sz = m_transfer_limit ? GetOutputLength() : 0;
	do
	{
		output_l::iterator it = m_obuf.begin();
		OUTPUT *p = *it;
		repeat = false;
		int n = TryWrite(p -> Buf(), p -> Len());
		if (n > 0)
		{
			size_t left = p -> Remove(n);
			m_output_length -= n;
			if (!left)
			{
				delete p;
				m_obuf.erase(it);
				if (!m_obuf.size())
				{
					m_obuf_top = NULL;
					OnWriteComplete();
				}
				else
				{
					repeat = true;
				}
			}
		}
	} while (repeat);

	if (m_transfer_limit && sz > m_transfer_limit && GetOutputLength() < m_transfer_limit)
	{
		OnTransferLimit();
	}

	// check output buffer set, set/reset m_wfds accordingly
	{
		bool br;
		bool bw;
		bool bx;
		Handler().Get(GetSocket(), br, bw, bx);
		if (m_obuf.size())
			Set(br, true);
		else
			Set(br, false);
	}
}


int TcpSocket::TryWrite(const char *buf, size_t len)
{
	int n = 0;
	n = send(GetSocket(), buf, (int)len, MSG_NOSIGNAL);
	if (n == -1)
	{
	// normal error codes:
	// WSAEWOULDBLOCK
	//       EAGAIN or EWOULDBLOCK
#ifdef _WIN32
		if (Errno != WSAEWOULDBLOCK)
#else
		if (Errno != EWOULDBLOCK)
#endif
		{	
			Handler().LogError(this, "send", Errno, StrError(Errno), LOG_LEVEL_FATAL);
			OnDisconnect();
			SetCloseAndDelete(true);
			SetFlushBeforeClose(false);
			SetLost();
		}
		return 0;
	}
	
	if (n > 0)
	{
		m_bytes_sent += n;
	}
	return n;
}


void TcpSocket::Buffer(const char *buf, size_t len)
{
	size_t ptr = 0;
	m_output_length += len;
	while (ptr < len)
	{
		// buf/len => pbuf/sz
		size_t space = 0;
		if (m_obuf_top && (space = m_obuf_top -> Space()) > 0)
		{
			const char *pbuf = buf + ptr;
			size_t sz = len - ptr;
			if (space >= sz)
			{
				m_obuf_top -> Add(pbuf, sz);
				ptr += sz;
			}
			else
			{
				m_obuf_top -> Add(pbuf, space);
				ptr += space;
			}
		}
		else
		{
			m_obuf_top = new OUTPUT;
			m_obuf.push_back( m_obuf_top );
		}
	}
}


void TcpSocket::Send(const std::string &str,int i)
{
	SendBuf(str.c_str(),str.size(),i);
}


void TcpSocket::SendBuf(const char *buf,size_t len,int)
{
	if (!Ready() && !Connecting())
	{
		Handler().LogError(this, "SendBuf", -1, "Attempt to write to a non-ready socket" ); // warning
		if (GetSocket() == INVALID_SOCKET)
			Handler().LogError(this, "SendBuf", 0, " * GetSocket() == INVALID_SOCKET", LOG_LEVEL_INFO);
		if (Connecting())
			Handler().LogError(this, "SendBuf", 0, " * Connecting()", LOG_LEVEL_INFO);
		if (CloseAndDelete())
			Handler().LogError(this, "SendBuf", 0, " * CloseAndDelete()", LOG_LEVEL_INFO);
		return;
	}
	if (!IsConnected())
	{
		Handler().LogError(this, "SendBuf", -1, "Attempt to write to a non-connected socket, will be sent on connect" ); // warning
		Buffer(buf, len);
		return;
	}
	if (m_obuf_top)
	{
		Buffer(buf, len);
		return;
	}
	int n = TryWrite(buf, len);
	if (n >= 0 && n < (int)len)
	{
		Buffer(buf + n, len - n);
	}
	// if ( data in buffer || !IsConnected )
	// {
	//   add to buffer
	// }
	// else
	// try_send
	// if any data is unsent, buffer it and set m_wfds

	// check output buffer set, set/reset m_wfds accordingly
	{
		bool br;
		bool bw;
		bool bx;
		Handler().Get(GetSocket(), br, bw, bx);
		if (m_obuf.size())
			Set(br, true);
		else
			Set(br, false);
	}
}


void TcpSocket::OnLine(const std::string& )
{
}


#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
TcpSocket::TcpSocket(const TcpSocket& s)
:StreamSocket(s)
,ibuf(0)
{
}
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


void TcpSocket::Sendf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char slask[5000]; // vsprintf / vsnprintf temporary
#ifdef _WIN32
	vsprintf(slask, format, ap);
#else
	vsnprintf(slask, 5000, format, ap);
#endif
	va_end(ap);
	Send( slask );
}


int TcpSocket::Close()
{
	if (GetSocket() == INVALID_SOCKET) // this could happen
	{
		Handler().LogError(this, "Socket::Close", 0, "file descriptor invalid", LOG_LEVEL_WARNING);
		return 0;
	}
	int n;
	SetNonblocking(true);
	if (!Lost() && IsConnected() && !(GetShutdown() & SHUT_WR))
	{
		if (shutdown(GetSocket(), SHUT_WR) == -1)
		{
			// failed...
			Handler().LogError(this, "shutdown", Errno, StrError(Errno), LOG_LEVEL_ERROR);
		}
	}
	//
	char tmp[1000];
	if (!Lost() && (n = recv(GetSocket(),tmp,1000,0)) >= 0)
	{
		if (n)
		{
			Handler().LogError(this, "read() after shutdown", n, "bytes read", LOG_LEVEL_WARNING);
		}
	}

	return Socket::Close();
}


#ifdef ENABLE_RECONNECT
void TcpSocket::SetReconnect(bool x)
{
	m_b_reconnect = x;
}
#endif


void TcpSocket::OnRawData(const char *buf_in,size_t len)
{
}


size_t TcpSocket::GetInputLength()
{
	return ibuf.GetLength();
}


size_t TcpSocket::GetOutputLength()
{
	return m_output_length;
}


uint64_t TcpSocket::GetBytesReceived(bool clear)
{
	uint64_t z = m_bytes_received;
	if (clear)
		m_bytes_received = 0;
	return z;
}


uint64_t TcpSocket::GetBytesSent(bool clear)
{
	uint64_t z = m_bytes_sent;
	if (clear)
		m_bytes_sent = 0;
	return z;
}


#ifdef ENABLE_RECONNECT
bool TcpSocket::Reconnect()
{
	return m_b_reconnect;
}


void TcpSocket::SetIsReconnect(bool x)
{
	m_b_is_reconnect = x;
}


bool TcpSocket::IsReconnect()
{
	return m_b_is_reconnect;
}
#endif


void TcpSocket::DisableInputBuffer(bool x)
{
	m_b_input_buffer_disabled = x;
}


void TcpSocket::OnOptions(int family,int type,int protocol,SOCKET s)
{
DEB(	fprintf(stderr, "Socket::OnOptions()\n");)
#ifdef SO_NOSIGPIPE
	SetSoNosigpipe(true);
#endif
	SetSoReuseaddr(true);
	SetSoKeepalive(true);
}


void TcpSocket::SetLineProtocol(bool x)
{
	StreamSocket::SetLineProtocol(x);
	DisableInputBuffer(x);
}


bool TcpSocket::SetTcpNodelay(bool x)
{
#ifdef TCP_NODELAY
	int optval = x ? 1 : 0;
	if (setsockopt(GetSocket(), IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval)) == -1)
	{
		Handler().LogError(this, "setsockopt(IPPROTO_TCP, TCP_NODELAY)", Errno, StrError(Errno), LOG_LEVEL_FATAL);
		return false;
	}
	return true;
#else
	Handler().LogError(this, "socket option not available", 0, "TCP_NODELAY", LOG_LEVEL_INFO);
	return false;
#endif
}


TcpSocket::CircularBuffer::CircularBuffer(size_t size)
:buf(new char[2 * size])
,m_max(size)
,m_q(0)
,m_b(0)
,m_t(0)
,m_count(0)
{
}


TcpSocket::CircularBuffer::~CircularBuffer()
{
	delete[] buf;
}


bool TcpSocket::CircularBuffer::Write(const char *s,size_t l)
{
	if (m_q + l > m_max)
	{
		return false; // overflow
	}
	m_count += (unsigned long)l;
	if (m_t + l > m_max) // block crosses circular border
	{
		size_t l1 = m_max - m_t; // size left until circular border crossing
		// always copy full block to buffer(buf) + top pointer(m_t)
		// because we have doubled the buffer size for performance reasons
		memcpy(buf + m_t, s, l);
		memcpy(buf, s + l1, l - l1);
		m_t = l - l1;
		m_q += l;
	}
	else
	{
		memcpy(buf + m_t, s, l);
		memcpy(buf + m_max + m_t, s, l);
		m_t += l;
		if (m_t >= m_max)
			m_t -= m_max;
		m_q += l;
	}
	return true;
}


bool TcpSocket::CircularBuffer::Read(char *s,size_t l)
{
	if (l > m_q)
	{
		return false; // not enough chars
	}
	if (m_b + l > m_max) // block crosses circular border
	{
		size_t l1 = m_max - m_b;
		if (s)
		{
			memcpy(s, buf + m_b, l1);
			memcpy(s + l1, buf, l - l1);
		}
		m_b = l - l1;
		m_q -= l;
	}
	else
	{
		if (s)
		{
			memcpy(s, buf + m_b, l);
		}
		m_b += l;
		if (m_b >= m_max)
			m_b -= m_max;
		m_q -= l;
	}
	if (!m_q)
	{
		m_b = m_t = 0;
	}
	return true;
}

bool TcpSocket::CircularBuffer::SoftRead(char *s, size_t l)
{
    if (l > m_q)
    {
        return false;
    }
    if (m_b + l > m_max)                          // block crosses circular border
    {
        size_t l1 = m_max - m_b;
        if (s)
        {
            memcpy(s, buf + m_b, l1);
            memcpy(s + l1, buf, l - l1);
        }
    }
    else
    {
        if (s)
        {
            memcpy(s, buf + m_b, l);
        }
    }
    return true;
}

bool TcpSocket::CircularBuffer::Remove(size_t l)
{
	return Read(NULL, l);
}


size_t TcpSocket::CircularBuffer::GetLength()
{
	return m_q;
}


const char *TcpSocket::CircularBuffer::GetStart()
{
	return buf + m_b;
}


size_t TcpSocket::CircularBuffer::GetL()
{
	return (m_b + m_q > m_max) ? m_max - m_b : m_q;
}


size_t TcpSocket::CircularBuffer::Space()
{
	return m_max - m_q;
}


unsigned long TcpSocket::CircularBuffer::ByteCounter(bool clear)
{
	if (clear)
	{
		unsigned long x = m_count;
		m_count = 0;
		return x;
	}
	return m_count;
}


std::string TcpSocket::CircularBuffer::ReadString(size_t l)
{
	char *sz = new char[l + 1];
	if (!Read(sz, l)) // failed, debug printout in Read() method
	{
		delete[] sz;
		return "";
	}
	sz[l] = 0;
	std::string tmp = sz;
	delete[] sz;
	return tmp;
}


void TcpSocket::OnConnectTimeout()
{
	Handler().LogError(this, "connect", -1, "connect timeout", LOG_LEVEL_FATAL);

	if (GetConnectionRetry() == -1 ||
		(GetConnectionRetry() && GetConnectionRetries() < GetConnectionRetry()) )
	{
		IncreaseConnectionRetries();
		// ask socket via OnConnectRetry callback if we should continue trying
		if (OnConnectRetry())
		{
			SetRetryClientConnect();
		}
		else
		{
			SetCloseAndDelete( true );
			/// \todo state reason why connect failed
			OnConnectFailed();
		}
	}
	else
	{
		SetCloseAndDelete(true);
		/// \todo state reason why connect failed
		OnConnectFailed();
	}
	//
	SetConnecting(false);
}


#ifdef _WIN32
void TcpSocket::OnException()
{
	if (Connecting())
	{
		if (GetConnectionRetry() == -1 ||
			(GetConnectionRetry() &&
			 GetConnectionRetries() < GetConnectionRetry() ))
		{
			// even though the connection failed at once, only retry after
			// the connection timeout
			// should we even try to connect again, when CheckConnect returns
			// false it's because of a connection error - not a timeout...
		}
		else
		{
			SetConnecting(false); // tnx snibbe
			SetCloseAndDelete();
			OnConnectFailed();
		}
		return;
	}
	// %! exception doesn't always mean something bad happened, this code should be reworked
	// errno valid here?
	int err = SoError();
	Handler().LogError(this, "exception on select", err, StrError(err), LOG_LEVEL_FATAL);
	SetCloseAndDelete();
}
#endif // _WIN32


int TcpSocket::Protocol()
{
	return IPPROTO_TCP;
}


void TcpSocket::SetTransferLimit(size_t sz)
{
	m_transfer_limit = sz;
}


void TcpSocket::OnTransferLimit()
{
}


#ifdef SOCKETS_NAMESPACE
}
#endif

