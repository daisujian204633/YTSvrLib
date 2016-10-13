/*MIT License

Copyright (c) 2016 Zhe Xu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
#include "stdafx.h"
#include "YTSocketMutiClient.h"
#include "YTSocketMutiClientController.h"
#ifndef LIB_WINDOWS
#include <fcntl.h>
#endif // !LIB_WINDOWS

namespace YTSvrLib
{

	void ITCPMUTICLIENT::OnError(int nErrCode)
	{
		LOG("ITCPMUTICLIENT::OnError : %d", nErrCode);

		SafeClose();
	}

	void ITCPMUTICLIENT::SafeClose()
	{
		m_bIsConnected = FALSE;
		m_bIsClosed = TRUE;
		m_bIsConnecting = FALSE;
		m_bIsDisconnecting = FALSE;

		if (m_pbufferevent)
		{
			bufferevent_free(m_pbufferevent);
			m_pbufferevent = NULL;
		}

		if (m_fd)
		{
			closesocket(m_fd);
			m_fd = 0;
		}
	}

	void ITCPMUTICLIENT::OnDisconnecting()
	{
		m_bIsDisconnecting = TRUE;

		if (m_pbufferevent)
		{
			bufferevent_free(m_pbufferevent);
			m_pbufferevent = NULL;
		}

		PostDisconnectMsg(eDisconnect);
	}

	BOOL ITCPMUTICLIENT::CreateClient(ITCPMUTICLIENTCONTROLLER* pController, const char* pszIP, int nPort)
	{
		if (m_bIsConnecting)
		{
			return FALSE;
		}

		if (pController == NULL)
		{
			LOG("pController == NULL : Are you kidding me?");
			return FALSE;
		}

		if (pController->GetEvent() == NULL)
		{
			return FALSE;
		}

		if (m_fd == 0 || m_fd == INVALID_SOCKET)
		{
			SOCKET nSock = socket(AF_INET, SOCK_STREAM, 0);
			if (nSock == 0 || nSock == INVALID_SOCKET)
			{
				m_bIsConnecting = FALSE;
				return FALSE;
			}

			m_fd = nSock;
			strncpy_s(m_szHost, pszIP, 31);
			m_nPort = nPort;

			sockaddr_in sAddrMy;
#ifdef LIB_WINDOWS
			sAddrMy.sin_family = AF_INET;
			sAddrMy.sin_addr.S_un.S_addr = INADDR_ANY;
			sAddrMy.sin_port = 0;

			u_long flag = 1;
			int nResult = ioctlsocket(nSock, FIONBIO, &flag);
			if (nResult != NO_ERROR)
			{
				OnError(GetLastError());
				closesocket(nSock);
				m_fd = 0;
				m_bIsConnecting = FALSE;
				return FALSE;
			}
#else
			sAddrMy.sin_family = AF_INET;
			sAddrMy.sin_addr.s_addr = INADDR_ANY;
			sAddrMy.sin_port = 0;

			int flags = 0;
			if ((flags = fcntl(nSock, F_GETFL)) == -1)
			{
				OnError(GetLastError());
				closesocket(nSock);
				m_bIsConnecting = FALSE;
				m_fd = 0;
				return FALSE;
			}

			flags |= O_NONBLOCK;

			if (fcntl(nSock, F_SETFL, flags) == -1)
			{
				OnError(GetLastError());
				closesocket(nSock);
				m_fd = 0;
				return FALSE;
			}
#endif // LIB_WINDOWS

			if (::bind(nSock, (sockaddr*) &sAddrMy, sizeof(sockaddr_in)))
			{
				m_bIsConnecting = FALSE;
				return FALSE;
			}
		}
		
		sockaddr_in sAddrDst;
#ifdef LIB_WINDOWS
		sAddrDst.sin_family = AF_INET;
		sAddrDst.sin_addr.S_un.S_addr = inet_addr(pszIP);
		sAddrDst.sin_port = htons((u_short) nPort);
#else
		sAddrDst.sin_family = AF_INET;
		sAddrDst.sin_addr.s_addr = inet_addr(pszIP);
		sAddrDst.sin_port = htons((u_short) nPort);
#endif // LIB_WINDOWS

		if (m_pbufferevent == NULL)
		{
			// BEV_OPT_THREADSAFE 不打的话OnDisconnecting崩溃
			m_pbufferevent = bufferevent_socket_new(pController->GetEvent(),GetSocket() , BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

			bufferevent_setwatermark(m_pbufferevent, EV_READ, 0, 0);

			bufferevent_setcb(m_pbufferevent, OnRead, NULL, ITCPBASE::OnError, this);

			bufferevent_enable(m_pbufferevent, EV_READ | EV_SIGNAL | EV_PERSIST);
		}

		if (bufferevent_socket_connect(m_pbufferevent, (sockaddr*) &sAddrDst, sizeof(sAddrDst)))
		{
			OnError(GetLastError());
			if (m_pbufferevent)
			{// Don't be too naive...bufferevent_socket_connect里面可能会先触发回调导致m_pbufferevent被删除
				bufferevent_free(m_pbufferevent);
				m_pbufferevent = NULL;
			}
			if (m_fd)
			{
				closesocket(GetSocket());
				m_fd = 0;
			}
			m_bIsConnecting = FALSE;
			return FALSE;
		}

		m_pController = pController;

		m_bIsClosed = FALSE;

		return TRUE;
	}
}