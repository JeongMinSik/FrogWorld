#include "stdafx.h"
#include "NetworkManager.h"


NetworkManager::NetworkManager()
{
	this->initialize();
}


NetworkManager::~NetworkManager()
{
	closesocket(m_Socket);
	WSACleanup();
}

bool NetworkManager::initialize()
{
	ZeroMemory(m_sendBuf, sizeof(m_sendBuf));
	ZeroMemory(m_recvBuf, sizeof(m_recvBuf));
	ZeroMemory(m_saveBuf, sizeof(m_saveBuf));
	m_iCurrPacketSize = 0;
	m_iStoredPacketSize = 0;
	m_hWnd = NULL;

	WSADATA	wsa;

	if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
		return false;

	m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_Socket == INVALID_SOCKET)
	{
		printf(" socket() error! \n");
		return false;
	}

	
	m_wsaSendBuf.buf = m_sendBuf;
	m_wsaSendBuf.len = sizeof(m_sendBuf);
	m_wsaRecvBuf.buf = m_recvBuf;
	m_wsaRecvBuf.len = sizeof(m_recvBuf);

	return true;
}

void NetworkManager::setIpAddress(DWORD address)
{
	ZeroMemory(&m_tServerAdrr, sizeof(m_tServerAdrr));
	m_tServerAdrr.sin_family = AF_INET;
	m_tServerAdrr.sin_port = htons(SERVER_PORT);
	m_tServerAdrr.sin_addr.s_addr = htonl(address);
}

bool NetworkManager::connectServer(HWND hWnd, HWND hName, HWND hBoard)
{
	m_hWnd = hWnd;
	m_hChatName = hName;
	m_hChatBoard = hBoard;

	int retval = connect(m_Socket, (SOCKADDR *)&m_tServerAdrr, sizeof(m_tServerAdrr));
	if (retval == SOCKET_ERROR)
	{
		printf(" connect() error! \n");
		return false;
	}
	retval = WSAAsyncSelect(m_Socket, m_hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE | FD_CONNECT);
	if (retval == SOCKET_ERROR)
	{
		printf(" WSAAsyncSelect() error! \n");
		return false;
	}
	sendLogin();

	return true;
}

void NetworkManager::sendLogin()
{
	C_LOGIN *pPacket = (C_LOGIN*)m_sendBuf;
	pPacket->header.size = sizeof(C_LOGIN);
	pPacket->header.packetID = PAK_ID::PAK_LOGIN;
	pPacket->id = m_db_id;

	sendBufData();
}

void NetworkManager::sendMove()
{
	C_MOVE *pPacket = (C_MOVE*)m_sendBuf;
	pPacket->header.size = sizeof(C_MOVE);
	pPacket->header.packetID = PAK_ID::PAK_REQ_MOVE;
	pPacket->dir = getMyPlayer()->action_;

	sendBufData();
}

void NetworkManager::sendAttack()
{
	HEADER *pPacket = (HEADER*)m_sendBuf;
	pPacket->size = sizeof(HEADER);
	pPacket->packetID = PAK_ID::PAK_REQ_ATTACK;

	sendBufData();
}

void NetworkManager::sendUseItem(ITEM_TYPE item)
{
	C_USE_ITEM *pPacket = (C_USE_ITEM*)m_sendBuf;
	pPacket->header.size = sizeof(C_USE_ITEM);
	pPacket->header.packetID = PAK_ID::PAK_USE_ITEM;
	pPacket->item = item;

	sendBufData();
}

void NetworkManager::setPlayer(map<UINT,Player*>* p)
{
	m_Players = p;
}

void NetworkManager::sendChat(char* buf)
{
	C_CHAT *pPacket = (C_CHAT*)m_sendBuf;
	pPacket->header.size = sizeof(C_CHAT);
	pPacket->header.packetID = PAK_ID::PAK_REQ_CHAT;
	strncpy_s(pPacket->buf, buf, sizeof(pPacket->buf));
	sendBufData();
}

int NetworkManager::getMyID()
{
	return m_id;
}

Player* NetworkManager::getMyPlayer()
{
	auto p =  m_Players->find(m_id);
	if (p == m_Players->end()) {
		return NULL;
	}
	return p->second;
}

void NetworkManager::set_id_ipAddress(int db_id, DWORD address)
{
	ZeroMemory(&m_tServerAdrr, sizeof(m_tServerAdrr));
	m_tServerAdrr.sin_family = AF_INET;
	m_tServerAdrr.sin_port = htons(SERVER_PORT);
	m_tServerAdrr.sin_addr.s_addr = htonl(address);

	m_db_id = db_id;
}


void NetworkManager::sendBufData()
{
	m_wsaSendBuf.len = ((HEADER*)m_wsaSendBuf.buf)->size;

	DWORD iobyte;
	if (WSASend(m_Socket, &m_wsaSendBuf,1, &iobyte, 0, NULL, NULL) == SOCKET_ERROR) 
	{
		printf(" send() Error! \n");
	}
}


void NetworkManager::processSocketMessage(LPARAM lParam)
{
	if (WSAGETSELECTERROR(lParam))
	{
		printf(" WSAGETSELECTERROR()! \n");
		MessageBox(m_hWnd, L"Network Error!", NULL, MB_OK);
		exit(0);
		return;
	}

	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_READ:
	{
		DWORD iobyte, ioflag = 0;
		int retval = WSARecv(m_Socket, &m_wsaRecvBuf, 1, &iobyte, &ioflag,  NULL, NULL);
		if (GetLastError() == WSAEWOULDBLOCK)
		{
			PostMessage(m_hWnd, WM_SOCKET, m_Socket, FD_READ);
		}
		this->processPacket(iobyte);
		break;
	}
	case FD_WRITE:
		break;
	case FD_CLOSE:
		printf(" FD_CLOSE! Network Disconnect... \n");
		MessageBox(m_hWnd, L"Network Disconnect!", NULL, MB_OK);
		exit(0);
		break;
	}
}

void NetworkManager::processPacket(int recvByte)
{
	char *pRecvBuf = m_recvBuf;

	while (0 < recvByte)
	{
		if (0 == m_iCurrPacketSize)
		{
			if (recvByte + m_iStoredPacketSize >= sizeof(HEADER))
			{
				int restHeaderSize = sizeof(HEADER) - m_iStoredPacketSize;
				memcpy(m_saveBuf + m_iStoredPacketSize, pRecvBuf, restHeaderSize);
				pRecvBuf += restHeaderSize;
				m_iStoredPacketSize += restHeaderSize;
				recvByte -= restHeaderSize;
				m_iCurrPacketSize = ((HEADER*)m_saveBuf)->size;
			}
			else
			{
				memcpy(m_saveBuf + m_iStoredPacketSize, pRecvBuf, recvByte);
				m_iStoredPacketSize += recvByte;
				recvByte = 0;
				break;
			}
		}

		int restSize = m_iCurrPacketSize - m_iStoredPacketSize;

		if (restSize <= recvByte)
		{
			memcpy(m_saveBuf + m_iStoredPacketSize, pRecvBuf, restSize);

			this->processContents();

			m_iCurrPacketSize = m_iStoredPacketSize = 0;

			pRecvBuf += restSize;
			recvByte -= restSize;
		}
		else
		{
			memcpy(m_saveBuf + m_iStoredPacketSize, pRecvBuf, recvByte);

			m_iStoredPacketSize += recvByte;
			recvByte = 0;
		}
	}
}

void NetworkManager::processContents()
{
	HEADER* header = (HEADER*)m_saveBuf;

	switch (header->packetID)
	{
	case PAK_ID::PAK_LOGIN_FAIL:
	{
		S_LOGIN_FAIL* pContentsPacket = (S_LOGIN_FAIL*)m_saveBuf;
		switch (pContentsPacket->reason)
		{
		case FAIL_REASON::WRONG_ID:
			printf(" 입력하신 id[%d]는 DB에 등록되지 않은 id 입니다. \n", m_db_id);
			break;
		case FAIL_REASON::CONNECTING:
			printf(" 입력하신 id[%d]는 현재 접속 중인 id 입니다. \n", m_db_id);
			break;
		}
		break;
	}
	case PAK_ID::PAK_LOGIN:
	{
		S_LOGIN* pContentsPacket = (S_LOGIN*)m_saveBuf;
		m_id = pContentsPacket->id;
		Player* myPlayer = new Player(pContentsPacket->position, pContentsPacket->level, pContentsPacket->hp, pContentsPacket->exp, -1);
		memcpy_s(myPlayer->items_, sizeof(myPlayer->items_), pContentsPacket->items, sizeof(myPlayer->items_));
		m_Players->insert(make_pair(m_id, myPlayer));

		printf(" login success. my id: %d \n", m_db_id);
		TCHAR buf[MAX_CHAT_BUF] = { 0 };
		wsprintf(buf, L"id[%2d] lv:%02d Hp:%4d/%4d Exp:%5d/%5d", m_db_id, myPlayer->level_, myPlayer->hp_, myPlayer->maxHp_, myPlayer->exp_, myPlayer->maxExp_);
		SetWindowText(m_hChatName, (LPCWSTR)buf);
		break;
	}
	case PAK_ID::PAK_PUT_OBJECT:
	{
		S_PUT_OBJECT* pContentsPacket = (S_PUT_OBJECT*)m_saveBuf;
		m_Players->insert(make_pair(pContentsPacket->id, new Player(pContentsPacket->position, pContentsPacket->level, pContentsPacket->hp, 0, pContentsPacket->npc_type, pContentsPacket->look)));
		printf(" [%d] object appears. \n", pContentsPacket->id);
		break;
	}
	case PAK_ID::PAK_REMOVE_OBJECT:
	{
		S_REMOVE_OBJECT* pContentsPacket = (S_REMOVE_OBJECT*)m_saveBuf;
		auto findPlayer = m_Players->find(pContentsPacket->id);
		if (findPlayer != m_Players->end())
		{
			delete findPlayer->second;
			m_Players->erase(findPlayer);
			printf(" [%d] object disappears. \n", pContentsPacket->id);
		}
		break;
	}
	case PAK_ID::PAK_MOVE_OBJECT:
	{
		S_MOVE_OBJECT* pContentsPacket = (S_MOVE_OBJECT*)m_saveBuf;
		auto findPlayer = m_Players->find(pContentsPacket->id);
		if (findPlayer != m_Players->end())
		{
			findPlayer->second->setPos(pContentsPacket->position, pContentsPacket->look);
		}
		else
		{

		}
		break;
	}
	case PAK_ID::PAK_ANS_CHAT:
	{
		S_CHAT* pContentsPacket = (S_CHAT*)m_saveBuf;
		TCHAR *recvBuf = new TCHAR[MAX_CHAT_BUF];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pContentsPacket->buf, -1, recvBuf, MAX_CHAT_BUF);
		m_chatList.push_back(recvBuf);

		if (m_chatList.size() > MAX_CHAT_LINE)
		{
			TCHAR* oldChat = m_chatList.front();
			delete oldChat;
			m_chatList.pop_front();
		}
		const unsigned int boardSize = (MAX_CHAT_BUF) * MAX_CHAT_LINE + MAX_CHAT_LINE;
		TCHAR strBoard[boardSize] = L"";
		for (auto c : m_chatList)
		{
			wcscat_s(strBoard, c);
			wcscat_s(strBoard, L"\n");
		}
		SetWindowText(m_hChatBoard, strBoard);

		break;
	}
	case PAK_ID::PAK_STAT_CHANGE:
	{
		S_STAT_CHANGE* pContentsPacket = (S_STAT_CHANGE*)m_saveBuf;
		auto findPlayer = m_Players->find(pContentsPacket->id);
		if (findPlayer != m_Players->end())
		{
			if (findPlayer->second->hp_ > pContentsPacket->hp)
			{
				findPlayer->second->isSuffing_ = true;
			}
			findPlayer->second->setLv(pContentsPacket->level);
			findPlayer->second->hp_ = pContentsPacket->hp;
			findPlayer->second->exp_ = pContentsPacket->exp;
			printf("debug: %d의 hp: %d \n", findPlayer->first, findPlayer->second->hp_);

			if (m_id == findPlayer->first)
			{
				TCHAR buf[MAX_CHAT_BUF] = { 0 };
				wsprintf(buf, L"id[%2d] lv:%02d Hp:%4d/%4d Exp:%5d/%5d", m_db_id, findPlayer->second->level_, findPlayer->second->hp_, findPlayer->second->maxHp_, findPlayer->second->exp_, findPlayer->second->maxExp_);
				SetWindowText(m_hChatName, (LPCWSTR)buf);
			}
		}

		break;
	}
	case PAK_ID::PAK_GET_ITEM:
	{
		S_GET_ITEM* pContentsPacket = (S_GET_ITEM*)m_saveBuf;
		m_Players->find(m_id)->second->items_[pContentsPacket->item]++;
		break;
	}
	case PAK_ID::PAK_USE_ITEM:
	{
		S_USE_ITEM* pContentsPacket = (S_USE_ITEM*)m_saveBuf;
		m_Players->find(m_id)->second->items_[pContentsPacket->item]--;
		break;
	}
	default:
		printf(" undefined packet id error! \n");
	}
	InvalidateRect(m_hWnd, NULL, 0);
}
