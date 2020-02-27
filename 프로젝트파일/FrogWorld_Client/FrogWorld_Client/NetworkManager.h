#pragma once

#include "Player.h"

#define		WM_SOCKET				WM_USER + 1

#define		MAX_CHAT_LINE	5

class NetworkManager
{
private:
	HWND				m_hWnd;
	HWND				m_hChatName;
	HWND				m_hChatBoard;
	list<TCHAR*>		m_chatList;
	
	SOCKET				m_Socket;
	SOCKADDR_IN			m_tServerAdrr;
	WSABUF				m_wsaSendBuf;
	WSABUF				m_wsaRecvBuf;
	char				m_sendBuf[PACKET_BUF_SIZE];
	char				m_recvBuf[PACKET_BUF_SIZE];
	char				m_saveBuf[PACKET_BUF_SIZE];
	int					m_iCurrPacketSize;
	int					m_iStoredPacketSize;

	int					m_id;
	int					m_db_id;
	map<UINT, Player*>	*m_Players;

private:
	void				processPacket(int recvByte);
	void				processContents();
	void				sendBufData();

public:
	NetworkManager();
	~NetworkManager();
	bool				initialize();
	void				setIpAddress(DWORD address);
	bool				connectServer(HWND hWnd, HWND hName, HWND hBoard);
	void				processSocketMessage(LPARAM lParam);

	void				sendLogin();
	void				sendMove();
	void				sendAttack();
	void				sendUseItem(ITEM_TYPE item);
	void				setPlayer(map<UINT, Player*> *p);
	void				sendChat(char* buf);
	int					getMyID();
	Player*				getMyPlayer();
	void				set_id_ipAddress(int id, DWORD address);

};

