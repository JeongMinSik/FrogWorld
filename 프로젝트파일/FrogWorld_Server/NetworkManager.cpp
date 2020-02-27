#include "NetworkManager.h"

// lua에 멤버함수를 등록하기 위한 것
typedef int (NetworkManager::*mem_func)(lua_State * L);
template <mem_func func>
int dispatch(lua_State * L) {
	NetworkManager * ptr = *static_cast<NetworkManager**>(lua_getextraspace(L));
	return ((*ptr).*func)(L);
}
/////////////////////////////////////////////////////////////

int NetworkManager::API_SetActived(lua_State *L)
{
	int oid = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	if (oid < MAX_PLAYER_SEED)
		return 1;
	else
		npcs_[oid]->isActived = false;
	return 1;
}

int NetworkManager::API_Is_Visible(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -2);
	int target_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);

	POINT t_pos = clients_[target_id]->player->pos_;

	if (getDistance(target_id,npc_id) > 1 && grassMap_[t_pos.y][t_pos.x] == GRASS_TYPE::BUSH)
		lua_pushnumber(L, 0);
	else
		lua_pushnumber(L, 1);
	return 1;
}

int NetworkManager::API_Move_Random(lua_State *L)
{
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);

	if (id < MAX_PLAYER_SEED)	return 1;

	unordered_set<int> view_list = getNpcViewList(id);

	ACTION_TYPE randomDir = ACTION_TYPE(rand() % MAX_MOVE_DIR);
	npcs_[id]->Move(randomDir, collisionMap_, &mapLock_, grassMap_);

	unordered_set<int> new_list = getNpcViewList(id);

	for (auto j : view_list)
	{

		if (0 != new_list.count(j))
		{
			sendMoveObject(j, id); // MOVE
		}
		else
		{
			clients_[j]->vl_lock.lock();
			clients_[j]->view_list.erase(id);
			clients_[j]->vl_lock.unlock();
			sendRemoveObject(j, id); // REMOVE
		}
	}

	for (auto j : new_list)
	{
		if (0 == view_list.count(j))
		{
			sendPutObject(j, id); // PUT
		}
	}

	return 1;
}

int NetworkManager::API_GetX(lua_State *L)
{
	int oid = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	if(oid < MAX_PLAYER_SEED)
		lua_pushnumber(L, clients_[oid]->player->pos_.x);
	else
		lua_pushnumber(L, npcs_[oid]->pos_.x);
	return 1;
}

int NetworkManager::API_GetY(lua_State *L)
{
	int oid = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	if (oid < MAX_PLAYER_SEED)
		lua_pushnumber(L, clients_[oid]->player->pos_.y);
	else
		lua_pushnumber(L, npcs_[oid]->pos_.y);
	return 1;
}

int NetworkManager::API_GetTargetID(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);

	int min_y = npcs_[npc_id]->pos_.y - VIEW_SPACE;
	int min_x = npcs_[npc_id]->pos_.x - VIEW_SPACE;
	if (min_y < 0) min_y = 0;
	if (min_x < 0) min_x = 0;
	int max_y = npcs_[npc_id]->pos_.y + VIEW_SPACE;
	int max_x = npcs_[npc_id]->pos_.x + VIEW_SPACE;
	if (max_y > BOARD_ROW) max_y = BOARD_ROW;
	if (max_x > BOARD_COL) max_x = BOARD_COL;

	// new_list 생성
	int minPlayerID = -1;
	int minDistance = 99999;
	mapLock_.lock();
	for (int y = min_y; y < max_y; ++y)
	{
		for (int x = min_x; x < max_x; ++x)
		{
			if (collisionMap_[y][x] >= MAX_PLAYER_SEED) continue;
			if (collisionMap_[y][x] > 0)
			{
				int player_id = collisionMap_[y][x];
				int dist = getDistance(player_id, npc_id);
				if (dist < minDistance)
				{
					lua_getglobal(L, "get_combat");
					lua_pcall(L, 0, 1, 0);
					int isCombat = (int)lua_tonumber(L, -1);
					lua_pop(L, 1);

					minDistance = dist;
					minPlayerID = player_id;

				}
			}
		}
	}
	mapLock_.unlock();
	
	lua_pushnumber(L, minPlayerID);
	return 1;
}

int NetworkManager::API_Ai_Start(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -2);
	int delayTick = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);
	
	if (npc_id < MAX_PLAYER_SEED) return 1;
	if (npcs_[npc_id]->isAlive == false) return 1;

	timerLock_.lock();
	timer_priority_Queue.push(event_node{ npc_id, GetTickCount() + delayTick,  OPERATION_TYPE::OP_NPC_AI_SELF });
	timerLock_.unlock();

	return 1;
}

int NetworkManager::API_Attack(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -2);
	int p_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);
	
	POINT p_pos  = clients_[p_id]->player->pos_;
	POINT n_pos = npcs_[npc_id]->pos_;

	if		(p_pos.x == n_pos.x - 1 && p_pos.y == n_pos.y) npcs_[npc_id]->lookDir_ = DIR_LEFT;
	else if (p_pos.x == n_pos.x + 1 && p_pos.y == n_pos.y) npcs_[npc_id]->lookDir_ = DIR_RIGHT;
	else if (p_pos.x == n_pos.x		&& p_pos.y == n_pos.y - 1) npcs_[npc_id]->lookDir_ = DIR_UP;
	else if (p_pos.x == n_pos.x		&& p_pos.y == n_pos.y + 1) npcs_[npc_id]->lookDir_ = DIR_DOWN;

	proceedAttack(npc_id);
	auto npc_view = getNpcViewList(npc_id);
	for (auto n : npc_view)
	{
		sendMoveObject(n, npc_id);
	}

	return 1;
}

int NetworkManager::API_FindPath(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -2);
	int p_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);

	POINT p_pos = clients_[p_id]->player->pos_;
	POINT n_pos = npcs_[npc_id]->pos_;

	if (abs(n_pos.x - p_pos.x) + abs(n_pos.y - p_pos.y) == 1) return 1;

	setWalkabilityMap();
	pathLock_.lock();
	pathStatus[npc_id] = FindPath(npc_id, n_pos.x, n_pos.y, p_pos.x, p_pos.y);
	pathLock_.unlock();
	if (pathStatus[npc_id] == nonexistent) {
		return 1; // 도달할 수 없음
	}
	int new_x = xPath[npc_id];
	int new_y = yPath[npc_id];

	unordered_set<int> view_list = getNpcViewList(npc_id);

	ACTION_TYPE newLook = ACTION_TYPE::DIR_STOP;
	if		(new_x == n_pos.x - 1	&& new_y == n_pos.y)		newLook = DIR_LEFT;
	else if (new_x == n_pos.x + 1	&& new_y == n_pos.y)		newLook = DIR_RIGHT;
	else if (new_x == n_pos.x		&& new_y == n_pos.y - 1)	newLook = DIR_UP;
	else if (new_x == n_pos.x		&& new_y == n_pos.y + 1)	newLook = DIR_DOWN;

	if (newLook == ACTION_TYPE::DIR_STOP) return 1;

	npcs_[npc_id]->Move(newLook, collisionMap_, &mapLock_, grassMap_);

	unordered_set<int> new_list = getNpcViewList(npc_id);

	for (auto j : view_list)
	{
		if (0 != new_list.count(j))
		{
			sendMoveObject(j, npc_id); // MOVE
		}
		else
		{
			clients_[j]->vl_lock.lock();
			clients_[j]->view_list.erase(npc_id);
			clients_[j]->vl_lock.unlock();
			sendRemoveObject(j, npc_id); // REMOVE
		}
	}

	for (auto j : new_list)
	{
		if (0 == view_list.count(j))
		{
			sendPutObject(j, npc_id); // PUT
		}
	}

	return 1;
}

NetworkManager::NetworkManager()
{
	InitializePathfinder();

	if (this->initializeDB() == false)
	{
		printf(" initialize DB fail! \n");
		exit(0);
	}
	if (this->initializeServer() == false)
	{
		printf(" initialize Server fail! \n");
		exit(0);
	}
}

NetworkManager::~NetworkManager()
{
	timerThread_->join();

	acceptThread_->join();

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		workerThread_[i]->join();
	}


	for (auto n : npcs_)
	{
		lua_close(n.second->Lua_);
		delete n.second;
	}
	npcs_.clear();

	for (auto c : clients_)
	{
		closesocket(c.second->socket);
		delete c.second->player;
		delete c.second;
	}
	clients_.clear();

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
	SQLDisconnect(hdbc_);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
	SQLFreeHandle(SQL_HANDLE_ENV, henv_);

	WSACleanup();

	EndPathfinder();
}

void NetworkManager::error_display(char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("%s", msg);
	wprintf(L" -> error: %s \n", lpMsgBuf);
	LocalFree(lpMsgBuf);
}

bool NetworkManager::acceptThread()
{
	SOCKET			clientSock;
	SOCKADDR_IN		clientAddr;
	DWORD			flags = 0;
	int				addrLen;

	while (1) 
	{
		addrLen = sizeof(clientAddr);
		clientSock = accept(listenSocket_, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSock == INVALID_SOCKET) 
		{
			printf(" accept error! \n");
			break;
		}

		// 접속차단
		if (playerCount_ >= MAX_PLAYER_COUNT)
		{
			closesocket(clientSock);
			printf(" players.size() is overflow error! \n");
			continue;
		}

		if (seedID_ >= MAX_PLAYER_SEED)
		{
			closesocket(clientSock);
			printf(" seedID_ is overflow error! \n");
			return false;
		}

		clientInfo *pSocketInfo = new clientInfo;
		if (pSocketInfo == NULL) break;
		ZeroMemory(pSocketInfo->saveBuf, sizeof(pSocketInfo->saveBuf));
		pSocketInfo->socket = clientSock;
		pSocketInfo->dbID = -1;
		pSocketInfo->iCurrPacketSize = pSocketInfo->iStoredPacketSize = 0;

		ZeroMemory(pSocketInfo->overEx.IOCP_buf, sizeof(pSocketInfo->overEx.IOCP_buf));
		ZeroMemory(&pSocketInfo->overEx.overlapped, sizeof(pSocketInfo->overEx.overlapped));
		pSocketInfo->overEx.wsabuf.buf = pSocketInfo->overEx.IOCP_buf;
		pSocketInfo->overEx.wsabuf.len = sizeof(pSocketInfo->overEx.IOCP_buf);
		pSocketInfo->overEx.optype = OPERATION_TYPE::OP_RECV;

		// 접속은 했으나 로그인은 안 한 상태
		pSocketInfo->isConnected = true;
		pSocketInfo->player = new Player(pSocketInfo->dbID);
		if (pSocketInfo->player == NULL) break;
		pSocketInfo->vl_lock.lock();
		pSocketInfo->view_list.clear();
		pSocketInfo->vl_lock.unlock();
		pSocketInfo->bl_lock.lock();
		pSocketInfo->bush_list.clear();
		pSocketInfo->bl_lock.unlock();

		clients_.insert(make_pair(seedID_,pSocketInfo));
		int newID = seedID_++;
		++playerCount_;

		printf(" id[%d], ip: %s, port: %d :: accpet. \n", newID, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

		CreateIoCompletionPort((HANDLE)clientSock, iocp_, newID, 0);

		flags = 0;
		int retval = WSARecv(clientSock, &pSocketInfo->overEx.wsabuf, 1, NULL, &flags, &pSocketInfo->overEx.overlapped, NULL);
		
		if (retval == SOCKET_ERROR)
		{
			int err_no = WSAGetLastError();
			if (err_no != WSA_IO_PENDING)
			{
				this->error_display("Recv Error!", err_no);
			}
		}
	}
	return true;
}

bool NetworkManager::workerThread()
{
	while (1) 
	{
		overlappedEx	*overEx = nullptr;
		DWORD			transferSize;
		ULONG			id;

		BOOL result = GetQueuedCompletionStatus(iocp_, &transferSize, (PULONG_PTR)&id, (LPOVERLAPPED *)&overEx, INFINITE);

		if (result == false)
		{
			this->error_display("GQCS error!", WSAGetLastError());
			logOut(id);
			continue;
		}

		if (transferSize == 0)
		{
			logOut(id);
			continue;
		}

		switch (overEx->optype)
		{
		case OPERATION_TYPE::OP_RECV:
			processPacket(id, transferSize);
			break;
		case OPERATION_TYPE::OP_SEND:
			delete overEx;
			break;
		case OPERATION_TYPE::OP_PLAYER_MOVE:
			wakeupNpc(id);
			delete overEx;
			break;
		case OPERATION_TYPE::OP_NPC_AI_SELF:
			doNpcAi(id);
			delete overEx;
			break;
		case OPERATION_TYPE::OP_RECOVER:
			recoverPlayer(id);
			delete overEx;
			break;
		case OPERATION_TYPE::OP_HEARTBEAT_PLAYER:
			heartbeatPlayer(id);
			delete overEx;
			break;
		case OPERATION_TYPE::OP_STOP_BLUE_POTION:
			stopUsingItem(id, ITEM_BLUE_POTION);
			delete overEx;
			break;
		case OPERATION_TYPE::OP_STOP_GREEN_POTION:
			stopUsingItem(id, ITEM_GREEN_POTION);
			delete overEx;
			break;
		default:
			printf("Unknown overEX optype! \n");
			break;
		}

	}
	return true;
}

bool NetworkManager::timerThread()
{
	/*while (true)
	{
		Sleep(10);
		do {
			timerLock_.lock();
			if (0 == timer_priority_Queue.size()) {
				timerLock_.unlock();
				break;
			}
			event_node t = timer_priority_Queue.top();
			if (t.startTime > GetTickCount()) {
				timerLock_.unlock(); 
				break;
			}
			timer_priority_Queue.pop();
			timerLock_.unlock();
			overlappedEx *send_over = new overlappedEx;
			memset(send_over, 0, sizeof(overlappedEx));
			send_over->optype = t.type;
			PostQueuedCompletionStatus(iocp_, 1, t.id, &(send_over->overlapped));
		} while (true);
	}*/

	while (true)
	{
		Sleep(10);
		timerLock_.lock();
		while (false == timer_priority_Queue.empty())
		{
			event_node ev = timer_priority_Queue.top();
			timer_priority_Queue.pop();
			if (ev.startTime > GetTickCount()) {
				timer_priority_Queue.push(ev);
				break;
			}
			timerLock_.unlock();
			overlappedEx *send_over = new overlappedEx;
			memset(send_over, 0, sizeof(overlappedEx));
			send_over->optype = ev.type;
			PostQueuedCompletionStatus(iocp_, 1, ev.id, &(send_over->overlapped));
			timerLock_.lock();
		}
		timerLock_.unlock();
	}
}

bool NetworkManager::initializeServer()
{
	playerCount_ = 0;
	seedID_ = 1;

	mapLock_.lock();
	for (int y = 0; y < BOARD_ROW; ++y)
	{
		for (int x = 0; x < BOARD_COL; ++x)
		{
			collisionMap_[y][x] = 0;
		}
	}
	mapLock_.unlock();

	WSADATA	wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
	{
		printf(" Wsa startup fail \n");
		return false;
	}

	iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, THREAD_COUNT);
	if (iocp_ == NULL) 
	{
		printf(" ioCompletionPort create fail \n");
		return false;
	}

	if (false == loadGrassMap())
	{
		return false;
	}

	if (false == initializeNPC())
	{
		return false;
	}

	

	return true;
}

bool NetworkManager::initializeDB()
{
	SQLWCHAR* db_name = L"frogworld_odbc";

	SQLRETURN retcode;

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv_);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		retcode = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv_, &hdbc_);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLSetConnectAttr(hdbc_, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc_, db_name, SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_, &hstmt_);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						wprintf(L" initialize DB success! \n");
						return true;
					}
				}
				else
				{
					wprintf(L" initialize DB fail! db_name: %s \n", db_name);
					return false;
				}
			}
		}
	}

	return false;

}

void NetworkManager::setWalkabilityMap()
{
	for (int y = 0; y < BOARD_ROW; ++y)
	{
		for (int x = 0; x < BOARD_COL; ++x)
		{
			if (collisionMap_[y][x] == 0 && grassMap_[y][x] != GRASS_TYPE::HILL)
				walkability[x][y] = walkable;
			else
				walkability[x][y] = unwalkable;
		}
	}

	printf("");
}

int NetworkManager::getDistance(int id_1, int id_2)
{
	POINT pos1, pos2;
	if (id_1 < MAX_PLAYER_SEED)
	{
		if (clients_.find(id_1) == clients_.end()) return 1;
		pos1 = clients_[id_1]->player->pos_;
	}
	else
	{
		pos1 = npcs_[id_1]->pos_;
	}
	if (id_2 < MAX_PLAYER_SEED)
	{
		pos2 = clients_[id_2]->player->pos_;
	}
	else
	{
		pos2 = npcs_[id_2]->pos_;
	}

	int distanceX = abs(pos1.x - pos2.x);
	int distanceY = abs(pos1.y - pos2.y);
	return distanceX + distanceY;
}

bool NetworkManager::loadGrassMap()
{
	ifstream fin("grassMap.txt");

	if (fin.is_open() == false) return false;

	for (int r = 0; r < BOARD_ROW; ++r)
	{
		for (int c = 0; c < BOARD_COL; ++c)
		{
			fin >> grassMap_[c][r];
		}
	}
	fin.close();
	printf(" load grassMap success! \n");

	return true;
}

bool NetworkManager::initializeNPC()
{
	for (auto i = MAX_PLAYER_SEED; i < MAX_NPC_SEED; ++i)
	{
		POINT pos;
		do
		{
			pos.x = rand() % BOARD_COL;
			pos.y = rand() % BOARD_ROW;
		} while (canEnterSpace(pos.x,pos.y) == false);

		Npc *npc = new Npc(i, pos, (rand() % 2) != 0 , (rand() % 2) != 0);
		mapLock_.lock();
		collisionMap_[pos.y][pos.x] = i;
		mapLock_.unlock();
		if (npc == NULL) return false;
		npc->isActived = false;

		npc->luaLock_.lock();
		lua_State *L = npc->Lua_ = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "AI_SCRIPT\\monster.lua");
		if (0 != lua_pcall(L, 0, 0, 0))
		{
			cout << "LUA error loading [AI_SCRIPT\\monster.lua] : " << (char *)lua_tostring(L, -1) << endl;
		}

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pushnumber(L, (int)npc->isAggro);
		lua_pushnumber(L, (int)npc->isRoaming);
		if (0 != lua_pcall(L, 3, 1, 0))
			cout << "LUA error calling [set_uid] : " << (char *)lua_tostring(L, -1) << endl;
		lua_pop(L, 1);

		npc->luaLock_.unlock();

		*static_cast<NetworkManager**>(lua_getextraspace(L)) = this; // 멤버함수 등록을 위한 인스턴스 등록
		lua_register(L, "API_GetX", &dispatch<&NetworkManager::API_GetX>);
		lua_register(L, "API_GetY", &dispatch<&NetworkManager::API_GetY>);
		lua_register(L, "API_GetTargetID", &dispatch<&NetworkManager::API_GetTargetID>);
		lua_register(L, "API_Ai_Start", &dispatch<&NetworkManager::API_Ai_Start>);
		lua_register(L, "API_Attack", &dispatch<&NetworkManager::API_Attack>);
		lua_register(L, "API_FindPath", &dispatch<&NetworkManager::API_FindPath>);
		lua_register(L, "API_Set_Actived", &dispatch<&NetworkManager::API_SetActived>);
		lua_register(L, "API_Move_Random", &dispatch<&NetworkManager::API_Move_Random>);
		lua_register(L, "API_Is_Visible", &dispatch<&NetworkManager::API_Is_Visible>);
		

		npcs_.insert(make_pair(i, npc));
		//recoverPlayer(i);
	}
	return true;
}

bool NetworkManager::createListenSocket()
{
	listenSocket_ = WSASocket(AF_INET, SOCK_STREAM, NULL, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (listenSocket_ == INVALID_SOCKET) 
	{
		printf(" ListenSocket Create Error! \n");
		return false;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	BOOL optval = TRUE;
	setsockopt(listenSocket_, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));

	int retval = ::bind(listenSocket_, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) 
	{
		this->error_display("Bind() Error!", WSAGetLastError());
		return false;
	}

	retval = ::listen(listenSocket_, SOMAXCONN);
	if (retval == SOCKET_ERROR) 
	{
		this->error_display("Listen() Error!", WSAGetLastError());
		return false;
	}

	printf(" createListenSocket Success. \n");
	return true;
}

bool NetworkManager::createThread()
{

	timerThread_ = new thread{ mem_fun(&NetworkManager::timerThread), this };

	acceptThread_ = new thread{ mem_fun(&NetworkManager::acceptThread), this };
	if (acceptThread_ == NULL)
	{
		printf(" acceptThread_ create error! \n");
	}
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		workerThread_[i] = new thread{ mem_fun(&NetworkManager::workerThread), this };
		if (workerThread_[i] == NULL)
		{
			printf(" workerThread_[%d] create error! \n", i);
		}
	};

	printf(" createThread success. \n");
	return true;
}


void NetworkManager::processPacket(int id, int recvByte)
{
	overlappedEx clientOverlaaped = clients_[id]->overEx;
	char *pRecvBuf = clientOverlaaped.IOCP_buf;
	
	int curSize = clients_[id]->iCurrPacketSize;
	int storedSize = clients_[id]->iStoredPacketSize;

	while (0 < recvByte)
	{
		if (0 == curSize)
		{
			if (recvByte + storedSize >= sizeof(HEADER))
			{
				int restHeaderSize = sizeof(HEADER) - storedSize;
				memcpy(clients_[id]->saveBuf + storedSize, pRecvBuf, restHeaderSize);
				pRecvBuf += restHeaderSize;
				storedSize += restHeaderSize;
				recvByte -= restHeaderSize;
				curSize = ((HEADER*)clients_[id]->saveBuf)->size;
				if ( 0 == curSize)
				{
					printf(" curSize is 0!! \n");
					break;
				}
			}
			else
			{
				memcpy(clients_[id]->saveBuf + storedSize, pRecvBuf, recvByte);
				storedSize += recvByte;
				recvByte = 0;
				break;
			}
		}

		int restSize = curSize - storedSize;

		if (restSize <= recvByte)
		{
			memcpy(clients_[id]->saveBuf + storedSize, pRecvBuf, restSize);

			this->processContents(id);

			curSize = storedSize = 0;

			pRecvBuf += restSize;
			recvByte -= restSize;
		}
		else
		{
			memcpy(clients_[id]->saveBuf + storedSize, pRecvBuf, recvByte);

			storedSize += recvByte;
			recvByte = 0;
		}
	}

	clients_[id]->iCurrPacketSize = curSize;
	clients_[id]->iStoredPacketSize = storedSize;

	DWORD flags = 0;
	int retval = WSARecv(clients_[id]->socket, &clients_[id]->overEx.wsabuf, 1, NULL, &flags, &clients_[id]->overEx.overlapped, NULL);

	if (retval == SOCKET_ERROR)
	{
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
		{
			this->error_display("Recv Error!", err_no);
		}
	}

}

void NetworkManager::processContents(int id)
{
	HEADER* header = (HEADER*)clients_[id]->saveBuf;

	switch (header->packetID)
	{
	case PAK_ID::PAK_LOGIN:
	{
		C_LOGIN *packet = ((C_LOGIN *)clients_[id]->saveBuf);

		// 접속 중인지 확인
		for (auto c : clients_)
		{
			if (c.second->isConnected == false) continue;
			if (c.second->dbID == packet->id)
			{
				login_fail(id, FAIL_REASON::CONNECTING);
				return;
			}
		}

		SQLWCHAR query[256] = L"EXEC dbo.select_player ";
		wchar_t str_id[256] = { 0 };
		_itow_s(packet->id, str_id, 10);
		wcscat_s(query, str_id);

		SQLRETURN retcode = SQLExecDirect(hstmt_, query, SQL_NTS);

		int sql_id, sql_lv, sql_exp, sql_hp, sql_posX, sql_posY, sql_red, sql_blue, sql_green;

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLBindCol(hstmt_, 1, SQL_C_LONG, &sql_id, sizeof(sql_id), NULL);
			SQLBindCol(hstmt_, 2, SQL_C_LONG, &sql_lv, sizeof(sql_lv), NULL);
			SQLBindCol(hstmt_, 3, SQL_C_LONG, &sql_exp, sizeof(sql_exp), NULL);
			SQLBindCol(hstmt_, 4, SQL_C_LONG, &sql_hp, sizeof(sql_hp), NULL);
			SQLBindCol(hstmt_, 5, SQL_C_LONG, &sql_posX, sizeof(sql_posX), NULL);
			SQLBindCol(hstmt_, 6, SQL_C_LONG, &sql_posY, sizeof(sql_posY), NULL);
			SQLBindCol(hstmt_, 7, SQL_C_LONG, &sql_red, sizeof(sql_red), NULL);
			SQLBindCol(hstmt_, 8, SQL_C_LONG, &sql_blue, sizeof(sql_blue), NULL);
			SQLBindCol(hstmt_, 9, SQL_C_LONG, &sql_green, sizeof(sql_green), NULL);

			// Fetch and print each row of data. On an error, display a message and exit.  
			retcode = SQLFetch(hstmt_);
			if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
			{
				printf("SQLFetch Error! \n");
			}
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				wprintf(L"sql_id: %d load data success! \n", sql_id);
			}
		}

		SQLCancel(hstmt_);

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			clients_[id]->dbID = sql_id;
			clients_[id]->player->setLv(sql_lv);
			int additional_exp = sql_exp - clients_[id]->player->getExp();
			clients_[id]->player->addExp(additional_exp);
			clients_[id]->player->setHp(sql_hp);
			clients_[id]->player->pos_ = POINT{ sql_posX, sql_posY };
			clients_[id]->player->items_[ITEM_RED_POTION] = sql_red;
			clients_[id]->player->items_[ITEM_BLUE_POTION] = sql_blue;
			clients_[id]->player->items_[ITEM_GREEN_POTION] = sql_green;
			login_ok(id);
		}
		else // 없는 아이디
		{
			//login_fail(id, FAIL_REASON::WRONG_ID);

			wcscpy_s(query, L"EXEC dbo.insert_player ");
			wcscat_s(query, str_id);

			retcode = SQLExecDirect(hstmt_, query, SQL_NTS);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				clients_[id]->dbID = packet->id;
				login_ok(id);
			}
			else
			{
				printf("insert player fail! \n");
				login_fail(id, FAIL_REASON::WRONG_ID);
			}
			SQLCancel(hstmt_);
		}
		break;
	}
	case PAK_ID::PAK_REQ_MOVE:
	{
		C_MOVE *packet = ((C_MOVE *)clients_[id]->saveBuf); 
		clients_[id]->player->push(action_node{ (ACTION_TYPE)(packet->dir), GetTickCount() });
		break;
	}
	case PAK_ID::PAK_REQ_ATTACK:
	{
		clients_[id]->player->push(action_node{ ACTION_TYPE::ATTACK, GetTickCount() });
		break;
	}
	case PAK_ID::PAK_USE_ITEM:
	{
		char chatBuf[MAX_CHAT_BUF] = { 0 };
		C_USE_ITEM *packet = ((C_USE_ITEM *)clients_[id]->saveBuf);
		switch (packet->item)
		{
		case ITEM_RED_POTION:
			sprintf_s(chatBuf, " 빨간");
			break;
		case ITEM_BLUE_POTION:
			sprintf_s(chatBuf, " 파란");
			break;
		case ITEM_GREEN_POTION:
			sprintf_s(chatBuf, " 초록");
			break;
		}
		// 사용불가
		if (false == clients_[id]->player->useItem(packet->item))
		{
			strcat_s(chatBuf, " 물약을 현재 사용할 수 없습니다!");
			this->sendChat(id, chatBuf);
			break;
		}

		// 사용
		switch (packet->item)
		{
		case ITEM_RED_POTION:
			strcat_s(chatBuf, " 물약을 사용했습니다! 최대 체력의 10%를 회복했습니다. ");
			break;
		case ITEM_BLUE_POTION:
			timerLock_.lock();
			timer_priority_Queue.push(event_node{ id, GetTickCount() + 5000,  OPERATION_TYPE::OP_STOP_BLUE_POTION });
			timerLock_.unlock();
			strcat_s(chatBuf, " 물약을 사용했습니다! 공격력이 잠시동안 2배 증가합니다. ");
			break;
		case ITEM_GREEN_POTION:
			timerLock_.lock();
			timer_priority_Queue.push(event_node{ id, GetTickCount() + 5000,  OPERATION_TYPE::OP_STOP_GREEN_POTION });
			timerLock_.unlock();
			strcat_s(chatBuf, " 물약을 사용했습니다! 속도가 잠시동안 2배 빨라집니다. ");
			break;
		}
		

		sendUseItem(id, packet->item);
		sendChat(id, chatBuf);

		sendStat(id, id);
		clients_[id]->vl_lock.lock();
		auto v_list = clients_[id]->view_list;
		clients_[id]->vl_lock.unlock();
		for (auto v : v_list)
		{
			if (v >= MAX_PLAYER_SEED) continue;
			if (clients_[v]->isConnected == false) continue;
			sendStat(v, id);
		}
		clients_[id]->bl_lock.lock();
		v_list = clients_[id]->bush_list;
		clients_[id]->bl_lock.unlock();
		for (auto v : v_list)
		{
			if (v >= MAX_PLAYER_SEED) continue;
			if (clients_[v]->isConnected == false) continue;
			sendStat(v, id);
		}
		break;
	}
	case PAK_ID::PAK_REQ_CHAT:
	{
		C_CHAT *packet = ((C_CHAT*)clients_[id]->saveBuf);
		char chatBuf[MAX_CHAT_BUF] = { 0 };
		sprintf_s(chatBuf, "  id[%2d] : ", clients_[id]->dbID);
		strncat_s(chatBuf, packet->buf, MAX_CHAT_BUF - 1 - strlen(chatBuf));
		for (auto cl : clients_)
		{
			if (cl.first >= MAX_PLAYER_SEED) continue;
			if (cl.second->isConnected == false) continue;
			this->sendChat(cl.first, chatBuf);
		}
		break;
	}
	default:
		printf(" undefined packet id error! \n");
	} 
}

void NetworkManager::sendBufData(int id, char* buf)
{
	overlappedEx *sendOver = new overlappedEx;
	ZeroMemory(sendOver, sizeof(overlappedEx));
	sendOver->optype = OPERATION_TYPE::OP_SEND;
	sendOver->wsabuf.buf = sendOver->IOCP_buf;
	sendOver->wsabuf.len = ((HEADER*)buf)->size;
	memcpy(sendOver->IOCP_buf, buf, sendOver->wsabuf.len);

	if (WSASend(clients_[id]->socket, &sendOver->wsabuf, 1, NULL, 0, &sendOver->overlapped, NULL) == SOCKET_ERROR) 
	{
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
		{
			printf("%d에게 보내는 센드 에러\n", id);
			this->error_display("Send Error!", err_no);
			logOut(id);
		}
	}
}

void NetworkManager::login_ok(int newID)
{
	// 초기위치 정하기
	POINT init_pos = clients_[newID]->player->pos_;

	while (canEnterSpace(init_pos.x, init_pos.y) == false)
	{
		init_pos.x = rand() % BOARD_COL; 
		init_pos.y = rand() % BOARD_ROW;
	}
	mapLock_.lock();
	collisionMap_[init_pos.y][init_pos.x] = newID;
	mapLock_.unlock();
	clients_[newID]->player->pos_ = init_pos;

	// 플레이어는 인덱스, client는 db아이디 보유
	clients_[newID]->player->id_ = newID;
	sendLogIn(newID);

	// 뷰리스트 업데이트
	updatePlayerViewList(newID);

	// 타이머 (HP회복, 이동)
	recoverPlayer(newID);
	heartbeatPlayer(newID);

}

void NetworkManager::login_fail(int newID, FAIL_REASON reason)
{
	sendLogIn_fail(newID, reason);
	logOut(newID);
}

void NetworkManager::sendLogIn_fail(int recv_id, FAIL_REASON reason)
{
	S_LOGIN_FAIL pPacket;
	pPacket.header.packetID = PAK_ID::PAK_LOGIN_FAIL;
	pPacket.header.size = sizeof(S_LOGIN_FAIL);
	pPacket.reason = reason;

	this->sendBufData(recv_id, (char*)&pPacket);

}

void NetworkManager::sendLogIn(int recv_id)
{
	S_LOGIN packet;
	packet.header.packetID = PAK_ID::PAK_LOGIN;
	packet.header.size = sizeof(packet);
	packet.id = recv_id;
	packet.position = clients_[recv_id]->player->pos_;
	packet.hp = clients_[recv_id]->player->getHp();
	packet.level = clients_[recv_id]->player->getLv();
	packet.exp = clients_[recv_id]->player->getExp();
	memcpy_s(packet.items, sizeof(packet.items), clients_[recv_id]->player->items_, sizeof(packet.items));

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::sendPutObject(int recv_id, int new_id)
{
	S_PUT_OBJECT packet;
	packet.header.size = sizeof(packet);
	packet.header.packetID = PAK_ID::PAK_PUT_OBJECT;
	packet.id = new_id;
	if (new_id < MAX_PLAYER_SEED)
	{
		packet.position = clients_[new_id]->player->pos_;
		packet.level = clients_[new_id]->player->level_;
		packet.hp = clients_[new_id]->player->getHp();
		packet.maxHp = clients_[new_id]->player->maxHp_;
		packet.look = clients_[new_id]->player->lookDir_;
		packet.npc_type = -1;
	}
	else
	{
		packet.position = npcs_[new_id]->pos_;
		packet.level = npcs_[new_id]->level_;
		packet.hp = npcs_[new_id]->getHp();
		packet.maxHp = npcs_[new_id]->maxHp_;
		packet.npc_type = npcs_[new_id]->npcType_;
		packet.look = npcs_[new_id]->lookDir_;
	}

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::sendRemoveObject(int recv_id, int rmv_id)
{
	S_REMOVE_OBJECT packet;
	packet.header.size = sizeof(packet);
	packet.header.packetID = PAK_ID::PAK_REMOVE_OBJECT;
	packet.id = rmv_id;

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::sendChat(int recv_id, char *buf)
{
	
	S_CHAT packet;
	packet.header.size = sizeof(packet);
	packet.header.packetID = PAK_ID::PAK_ANS_CHAT;
	time_t timer = time(NULL);    // 현재 시각을 초 단위로 얻기
	struct tm t;
	localtime_s(&t, &timer); // 초 단위의 시간을 분리하여 구조체에 넣기
	sprintf_s(packet.buf, "[%04d-%02d-%02d %02d:%02d:%02d] ::: ",
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec
	);
	strncat_s(packet.buf, buf, MAX_CHAT_BUF - 1);
	packet.buf[MAX_CHAT_BUF - 1] = NULL;

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::sendGetItem(int recv_id, ITEM_TYPE item)
{
	S_GET_ITEM packet;
	packet.header.size = sizeof(packet);
	packet.header.packetID = PAK_ID::PAK_GET_ITEM;
	packet.item = item;

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::sendUseItem(int recv_id, ITEM_TYPE item)
{
	S_USE_ITEM packet;
	packet.header.size = sizeof(packet);
	packet.header.packetID = PAK_ID::PAK_USE_ITEM;
	packet.item = item;

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::sendStat(int recv_id, int change_id)
{
	S_STAT_CHANGE packet;
	packet.header.size = sizeof(packet);
	packet.header.packetID = PAK_ID::PAK_STAT_CHANGE;
	packet.id = change_id;
	if (change_id < MAX_PLAYER_SEED)
	{
		packet.hp = clients_[change_id]->player->getHp();
		packet.level = clients_[change_id]->player->getLv();
		packet.exp = clients_[change_id]->player->getExp();
	}
	else
	{
		packet.hp = npcs_[change_id]->getHp();
		packet.level = npcs_[change_id]->getLv();
		packet.exp = 0;
	}

	this->sendBufData(recv_id, (char*)&packet);
}

void NetworkManager::logOut(int out_id)
{
	if (false == clients_[out_id]->isConnected) return;

	clients_[out_id]->isConnected = false;
	--playerCount_;
	printf(" id[%d] disconnect. \n", out_id);

	if (-1 == clients_[out_id]->player->id_) return;

	// 나갈 때 다른 플레이어들의 viwe_list 업데이트
	for (auto c = clients_.begin(); c != clients_.end(); ++c)
	{
		if (c->first >= MAX_PLAYER_SEED) continue;
		if (c->second->isConnected == false) continue;
		int ci = c->first;
		if (ci == out_id) continue;
		clients_[ci]->vl_lock.lock();
		if (0 != clients_[ci]->view_list.count(out_id))
		{
			clients_[ci]->view_list.erase(out_id);
			clients_[ci]->vl_lock.unlock();
			sendRemoveObject(ci, out_id);
		}
		else
		{
			clients_[ci]->vl_lock.unlock();
		}
	}

	POINT pos = clients_[out_id]->player->pos_;
	mapLock_.lock();
	collisionMap_[pos.y][pos.x] = 0;
	mapLock_.unlock();

	// 나갈 때 DB 업데이트
	updateDB(out_id);

}

void NetworkManager::sendMoveObject(int recv_id, int move_id)
{
	S_MOVE_OBJECT packet;
	packet.header.size = sizeof(S_MOVE_OBJECT);
	packet.header.packetID = PAK_ID::PAK_MOVE_OBJECT;
	packet.id = move_id;
	if (move_id < MAX_PLAYER_SEED)
	{
		packet.position = clients_[move_id]->player->pos_;
		packet.look = clients_[move_id]->player->lookDir_;
	}
	else
	{
		packet.position = npcs_[move_id]->pos_;
		packet.look = npcs_[move_id]->lookDir_;
	}

	this->sendBufData(recv_id, (char*)&packet);

}

void NetworkManager::proceedAttack(int atk_id)
{
	POINT pos;
	POINT atk_pos;
	int damage;

	if (atk_id < MAX_PLAYER_SEED)
	{
		damage = clients_[atk_id]->player->damage_;
		pos = clients_[atk_id]->player->pos_;
		atk_pos = pos;
		switch (clients_[atk_id]->player->lookDir_)
		{
		case DIR_LEFT:
			--(atk_pos.x);
			if (atk_pos.x < 0) return;
			break;
		case DIR_RIGHT:
			++(atk_pos.x);
			if (atk_pos.x >= BOARD_COL) return;
			break;
		case DIR_UP:
			--(atk_pos.y);
			if (atk_pos.y < 0) return;
			break;
		case DIR_DOWN:
			++(atk_pos.y);
			if (atk_pos.y >= BOARD_ROW) return;
			break;
		}
	}
	else
	{
		damage = npcs_[atk_id]->damage_;
		pos = npcs_[atk_id]->pos_;
		atk_pos = pos;

		switch (npcs_[atk_id]->lookDir_)
		{
		case DIR_LEFT:
			--(atk_pos.x);
			if (atk_pos.x < 0) return;
			break;
		case DIR_RIGHT:
			++(atk_pos.x);
			if (atk_pos.x >= BOARD_COL) return;
			break;
		case DIR_UP:
			--(atk_pos.y);
			if (atk_pos.y < 0) return;
			break;
		case DIR_DOWN:
			++(atk_pos.y);
			if (atk_pos.y >= BOARD_ROW) return;
			break;
		}
	}

	mapLock_.lock();
	unsigned int hit_id = collisionMap_[atk_pos.y][atk_pos.x];
	mapLock_.unlock();

	if (hit_id == 0) return;

	char chatBuf[MAX_CHAT_BUF] = { 0 };
	unordered_set<int> view_list;

	if (hit_id < MAX_PLAYER_SEED)
	{
		clients_[hit_id]->player->suffer(damage);

		// 시야처리
		clients_[hit_id]->vl_lock.lock();
		if (clients_[hit_id]->view_list.count(atk_id) == 0)
		{
			clients_[hit_id]->view_list.insert(atk_id);
			sendPutObject(hit_id, atk_id);
			sendStat(hit_id, atk_id);
		}
		clients_[hit_id]->vl_lock.unlock();

		// 메세지
		if (atk_id < MAX_PLAYER_SEED)
		{
			sprintf_s(chatBuf, " 상대 개구리[%d]에게 [%d] 대미지를 입혔다!", clients_[hit_id]->dbID, damage);
			sendChat(atk_id, chatBuf);
			sprintf_s(chatBuf, " 상대 개구리[%d]로부터 [%d] 대미지를 입었다!", clients_[atk_id]->dbID, damage);
			sendChat(hit_id, chatBuf);
		}
		else
		{
			sprintf_s(chatBuf, " 몬스터[%d]로부터 [%d] 대미지를 입었다!", atk_id, damage);
			sendChat(hit_id, chatBuf);
		}

		// 플레이어 사망
		if (clients_[hit_id]->player->getHp() <= 0)
		{
			// 메세지
			if (atk_id < MAX_PLAYER_SEED)
			{
				sprintf_s(chatBuf, " 상대 개구리[%d]를 사냥했다!", clients_[hit_id]->dbID);
				sendChat(atk_id, chatBuf);
				sprintf_s(chatBuf, " 상대 개구리[%d]로부터 사냥당했다!", clients_[atk_id]->dbID);
				sendChat(hit_id, chatBuf);
			}
			else
			{
				sprintf_s(chatBuf, " 몬스터[%d]의 공격으로 사망했다!", atk_id);
				sendChat(hit_id, chatBuf);
			}

			int lost_exp = clients_[hit_id]->player->getExp() / 2;
			clients_[hit_id]->player->addExp(-lost_exp);
			clients_[hit_id]->player->setHp(clients_[hit_id]->player->maxHp_);

			// 시작 위치로 돌아가기
			POINT init_pos = clients_[hit_id]->player->pos_;
			mapLock_.lock();
			collisionMap_[init_pos.y][init_pos.x] = 0;
			mapLock_.unlock();
			init_pos = POINT{ 0,1 };
			while (canEnterSpace(init_pos.x, init_pos.y) == false)
			{
				init_pos.x = rand() % BOARD_COL;
				init_pos.y = rand() % BOARD_ROW;
			}
			mapLock_.lock();
			collisionMap_[init_pos.y][init_pos.x] = hit_id;
			mapLock_.unlock();
			clients_[hit_id]->player->pos_ = init_pos;
			sendMoveObject(hit_id, hit_id);
		}

		updatePlayerViewList(hit_id);
		clients_[hit_id]->vl_lock.lock();
		view_list = clients_[hit_id]->view_list;
		clients_[hit_id]->vl_lock.unlock();

		sendStat(hit_id, hit_id);
		for (auto vi : view_list)
		{
			if (vi >= MAX_PLAYER_SEED) continue;
			sendStat(vi, hit_id);
		}

		clients_[hit_id]->bl_lock.lock();
		view_list = clients_[hit_id]->bush_list;
		clients_[hit_id]->bl_lock.unlock();
		for (auto vi : view_list)
		{
			if (vi >= MAX_PLAYER_SEED) continue;
			sendStat(vi, hit_id);
		}

	}
	else
	{
		// npc끼리는 안 싸움
		if (atk_id >= MAX_PLAYER_SEED) return;

		npcs_[hit_id]->luaLock_.lock();
		lua_State *L = npcs_[hit_id]->Lua_;
		lua_getglobal(L, "set_combat");
		lua_pushnumber(L, atk_id);
		if (0 != lua_pcall(L, 1, 1, 0))
			cout << "LUA error calling [set_combat] : " << (char *)lua_tostring(L, -1) << endl;
		lua_pop(L, 1);
		npcs_[hit_id]->luaLock_.unlock();

		//wakeupNpc(hit_id);

		npcs_[hit_id]->suffer(damage);
		view_list = getNpcViewList(hit_id);
		view_list.insert(atk_id);

		for (auto vi : view_list)
		{
			if (vi >= MAX_PLAYER_SEED) continue;
			sendStat(vi, hit_id);
		}

		// 메세지
		sprintf_s(chatBuf, " 몬스터[%d]에게 [%d] 대미지를 입혔다!", hit_id, damage);
		sendChat(atk_id, chatBuf);

		// 몬스터 사망 시
		if (npcs_[hit_id]->getHp() <= 0)
		{
			// 메세지
			int get_exp = giveExp(atk_id, hit_id);
			sprintf_s(chatBuf, " 몬스터[%d]를 사냥하여 [%d]의 경험치를 얻었다!", hit_id, get_exp);
			sendChat(atk_id, chatBuf);


			// 뷰리스트 업데이트
			clients_[atk_id]->vl_lock.lock();
			auto atk_view_list = clients_[atk_id]->view_list;
			clients_[atk_id]->vl_lock.unlock();

			for (auto vi : view_list)
			{
				if (vi >= MAX_PLAYER_SEED) continue;
				sendStat(vi, atk_id);
			}

			npcs_[hit_id]->isActived = false;
			npcs_[hit_id]->isAlive = false;
			mapLock_.lock();
			collisionMap_[atk_pos.y][atk_pos.x] = 0;
			mapLock_.unlock();

			for (auto vi : view_list)
			{
				if (vi >= MAX_PLAYER_SEED) continue;
				sendRemoveObject(vi, hit_id);
			}

			// 아이템 획득
			int random_result = rand() % 100 + 1;
			if (random_result > 0)
			{
				ITEM_TYPE item = (ITEM_TYPE)(rand() % MAX_ITEM_TYPE);
				sendGetItem(atk_id, item);
				clients_[atk_id]->player->items_[item]++;
				switch (item)
				{
				case ITEM_TYPE::ITEM_RED_POTION:
					sprintf_s(chatBuf, " 몬스터[%d]를 사냥하여 빨간 물약을 획득했다!", hit_id);
					break;
				case ITEM_TYPE::ITEM_BLUE_POTION:
					sprintf_s(chatBuf, " 몬스터[%d]를 사냥하여 파란 물약을 획득했다!", hit_id);
					break;
				case ITEM_TYPE::ITEM_GREEN_POTION:
					sprintf_s(chatBuf, " 몬스터[%d]를 사냥하여 초록 물약을 획득했다!", hit_id);
					break;
				}
				sendChat(atk_id, chatBuf);
			}
		}
	}
}

void NetworkManager::wakeupNpc(int id)
{
	if (id < MAX_PLAYER_SEED) return;
	if (false == npcs_[id]->isAlive) return;

	npcs_.find(id)->second->luaLock_.lock();
	lua_State *L = npcs_.find(id)->second->Lua_;
	lua_getglobal(L, "player_move_notify");
	if (0 != lua_pcall(L, 0, 1, 0))
	{
		cout << "LUA error calling [player_move_notify] : " << (char *)lua_tostring(L, -1) << endl;
	}
	lua_pop(L, 1);
	npcs_.find(id)->second->luaLock_.unlock();

	return;
}

void NetworkManager::doNpcAi(int npc_id)
{
	npcs_.find(npc_id)->second->luaLock_.lock();
	lua_State *L = npcs_.find(npc_id)->second->Lua_;
	lua_getglobal(L, "move_self");

	if (0 != lua_pcall(L, 0, 1, 0))
	{
		cout << "LUA error calling [move_self] : " << (char *)lua_tostring(L, -1) << endl;
	}
	lua_pop(L, 1);
	npcs_.find(npc_id)->second->luaLock_.unlock();
}

void NetworkManager::heartbeatPlayer(int id)
{
	if (id >= MAX_PLAYER_SEED) return;
	if (clients_[id]->isConnected == false) return;

	timerLock_.lock();
	timer_priority_Queue.push(event_node{ id, GetTickCount() + clients_[id]->player->heartBeatTick_,  OPERATION_TYPE::OP_HEARTBEAT_PLAYER });
	timerLock_.unlock();

	POINT myPos = clients_[id]->player->pos_;

	ACTION_TYPE result = clients_[id]->player->HeartBeat(collisionMap_, &mapLock_, grassMap_);

	if (ACTION_TYPE::DIR_STOP <= result && result < ACTION_TYPE::ATTACK)
	{
		sendMoveObject(id, id);
		if (result == DIR_STOP)
		{
			clients_[id]->vl_lock.lock();
			auto view_list = clients_[id]->view_list;
			clients_[id]->vl_lock.unlock();
			for (auto vi : view_list)
			{
				if (vi >= MAX_PLAYER_SEED) continue;
				if (clients_[vi]->isConnected == false) continue;
				sendMoveObject(vi, id);
			}
			clients_[id]->bl_lock.lock();
			view_list = clients_[id]->bush_list;
			clients_[id]->bl_lock.unlock();
			for (auto vi : view_list)
			{
				if (vi >= MAX_PLAYER_SEED) continue;
				if (clients_[vi]->isConnected == false) continue;
				sendMoveObject(vi, id);
			}
		}
		else
		{
			updatePlayerViewList(id);
		}
	}
	else if (result == ACTION_TYPE::ATTACK)
	{
		proceedAttack(id);
	}
}


void NetworkManager::recoverPlayer(int id)
{
	if (clients_[id]->isConnected == false) return;

	if (true == clients_[id]->player->Recover())
	{
		sendStat(id, id);
		clients_[id]->vl_lock.lock();
		auto view_list = clients_[id]->view_list;
		clients_[id]->vl_lock.unlock();
		for (auto vi : view_list)
		{
			if (vi >= MAX_PLAYER_SEED) continue;
			if (clients_[vi]->isConnected == false) continue;
			sendStat(vi, id);
		}
		clients_[id]->bl_lock.lock();
		view_list = clients_[id]->bush_list;
		clients_[id]->bl_lock.unlock();
		for (auto vi : view_list)
		{
			if (vi >= MAX_PLAYER_SEED) continue;
			if (clients_[vi]->isConnected == false) continue;
			sendStat(vi, id);
		}
	}

	timerLock_.lock();
	timer_priority_Queue.push(event_node{ id, GetTickCount() + 5000,  OPERATION_TYPE::OP_RECOVER });
	timerLock_.unlock();
}

void NetworkManager::stopUsingItem(int id, ITEM_TYPE item)
{
	clients_[id]->player->stopItem(item);

	if (clients_[id]->isConnected == false) return;

	char chatBuf[MAX_CHAT_BUF] = { 0 };
	switch (item)
	{
	case ITEM_BLUE_POTION:
		sprintf_s(chatBuf, " 파란");
		break;
	case ITEM_GREEN_POTION:
		sprintf_s(chatBuf, " 초록");
		break;
	}
	strcat_s(chatBuf, " 물약 효과 지속시간이 끝났습니다!");
	sendChat(id, chatBuf);
}

int NetworkManager::giveExp(int get_id, int npc_id)
{
	int exp = npcs_[npc_id]->getLv() * 5;

	if (npcs_[npc_id]->isAggro) exp *= 2;
	if (npcs_[npc_id]->isRoaming) exp *= 2;

	clients_[get_id]->player->addExp(exp);

	return exp;
}

bool NetworkManager::canEnterSpace(int x, int y)
{
	mapLock_.lock();
	if (collisionMap_[y][x] != 0)
	{
		mapLock_.unlock();
		return false;
	}
	mapLock_.unlock();

	if (grassMap_[y][x] == GRASS_TYPE::HILL) return false;

	return true;
}

void NetworkManager::updatePlayerViewList(int id)
{
	if (id >= MAX_PLAYER_SEED) return;

	POINT myPos = clients_[id]->player->pos_;

	// 시야처리 테스트//
	int min_y = clients_[id]->player->pos_.y - VIEW_SPACE;
	int min_x = clients_[id]->player->pos_.x - VIEW_SPACE;
	if (min_y < 0) min_y = 0;
	if (min_x < 0) min_x = 0;
	int max_y = clients_[id]->player->pos_.y + VIEW_SPACE;
	int max_x = clients_[id]->player->pos_.x + VIEW_SPACE;
	if (max_y > BOARD_ROW) max_y = BOARD_ROW;
	if (max_x > BOARD_COL) max_x = BOARD_COL;

	// new_list 생성
	unordered_set <int> new_list;
	unordered_set <int> new_bush_list;
	mapLock_.lock();
	for (int y = min_y; y < max_y; ++y)
	{
		for (int x = min_x; x < max_x; ++x)
		{
			if (collisionMap_[y][x] == id) continue;
			if (collisionMap_[y][x] > 0) 
			{
				int other_id = collisionMap_[y][x];
				// 거리가 1이거나, 부쉬에 숨지 않은 객체를 리스트에 넣음
				if (getDistance(id,other_id) == 1 || grassMap_[y][x] != GRASS_TYPE::BUSH)
				{
					new_list.insert(other_id);
				}
				else
				{
					new_bush_list.insert(other_id);
				}
			}
		}
	}
	mapLock_.unlock();

	// npc 깨우기
	for (auto npc_id : new_list) {
		if (npc_id < MAX_PLAYER_SEED) continue;
		if (npcs_[npc_id]->isAlive == false) return;
		if (npcs_[npc_id]->isActived == true) continue;
		npcs_[npc_id]->isActived = true;
		timer_priority_Queue.push(event_node{ npc_id, GetTickCount() + 1000, OPERATION_TYPE::OP_PLAYER_MOVE });
	}

	// new_list와 view_list 의 비교를 통해 뷰에 들어오는 객체 처리
	for (auto ni : new_list)
	{
		// 움직인 플레이어(id)의 new_list에 있는 객체가 view_list에 없다면 [ 즉, 새로 추가된 객체 ]
		clients_[id]->vl_lock.lock();
		if (clients_[id]->view_list.count(ni) == 0)
		{
			clients_[id]->view_list.insert(ni);
			clients_[id]->vl_lock.unlock();
			sendPutObject(id, ni);
		}
		else  // new_list 객체가 이미 view_list에 있다면
		{
			clients_[id]->vl_lock.unlock();
		}

		// NPC는 아래의 뷰리스트 업데이트가 필요 없음
		if (ni >= MAX_PLAYER_SEED) continue;

		// 새로운 플레이어(ni)의 view_list에 움직인 플레이어(id)가 없다면 
		clients_[ni]->vl_lock.lock();
		if (clients_[ni]->view_list.count(id) == 0)
		{
			if (grassMap_[myPos.y][myPos.x] == GRASS_TYPE::BUSH && getDistance(id, ni) > 1 )
			{
				clients_[ni]->vl_lock.unlock();

				clients_[ni]->bl_lock.lock();
				clients_[ni]->bush_list.insert(id);
				clients_[ni]->bl_lock.unlock();
			}
			else
			{
				clients_[ni]->view_list.insert(id);
				clients_[ni]->vl_lock.unlock();
				sendPutObject(ni, id);
			}
		}
		else										// 있다면 이동
		{
			if (grassMap_[myPos.y][myPos.x] == GRASS_TYPE::BUSH && getDistance(id, ni) > 1)
			{
				clients_[ni]->view_list.erase(id);
				clients_[ni]->vl_lock.unlock();
				clients_[ni]->bl_lock.lock();
				clients_[ni]->bush_list.insert(id);
				clients_[ni]->bl_lock.unlock();
				sendRemoveObject(ni, id);
			}
			else
			{
				clients_[ni]->view_list.insert(id);
				clients_[ni]->vl_lock.unlock();
				sendMoveObject(ni, id);
			}
		}
	}

	// 부쉬리스트 업데이트
	for (auto bi : new_bush_list)
	{
		// 움직인 플레이어(id)의 new_list에 있는 객체가 view_list에 없다면 [ 즉, 새로 추가된 객체 ]
		clients_[id]->bl_lock.lock();
		if (clients_[id]->bush_list.count(bi) == 0)
		{
			clients_[id]->bush_list.insert(bi);
			clients_[id]->bl_lock.unlock();
		}
		else  // new_list 객체가 이미 view_list에 있다면
		{
			clients_[id]->bl_lock.unlock();
		}

		// NPC는 아래의 부쉬리스트 업데이트가 필요 없음
		if (bi >= MAX_PLAYER_SEED) continue;

		// 부쉬리스트에 있는 클라들 업데이트
		clients_[bi]->vl_lock.lock();
		if (clients_[bi]->view_list.count(id) == 0)
		{
			if (grassMap_[myPos.y][myPos.x] == GRASS_TYPE::BUSH &&getDistance(id, bi) > 1)
			{
				clients_[bi]->vl_lock.unlock();

				clients_[bi]->bl_lock.lock();
				clients_[bi]->bush_list.insert(id);
				clients_[bi]->bl_lock.unlock();
			}
			else
			{
				clients_[bi]->view_list.insert(id);
				clients_[bi]->vl_lock.unlock();
				sendPutObject(bi, id);
			}
		}
		else										// 있다면 이동
		{
			if (grassMap_[myPos.y][myPos.x] == GRASS_TYPE::BUSH && getDistance(id, bi) > 1)
			{
				clients_[bi]->view_list.erase(id);
				clients_[bi]->vl_lock.unlock();
				clients_[bi]->bl_lock.lock();
				clients_[bi]->bush_list.insert(id);
				clients_[bi]->bl_lock.unlock();
				sendRemoveObject(bi, id);
			}
			else
			{
				clients_[bi]->vl_lock.unlock();
				sendMoveObject(bi, id);
			}
		}
	}

	// 부쉬 리스트에서 나가는 객체 처리
	vector <int> remove_bush_list;
	clients_[id]->bl_lock.lock();
	for (auto vi : clients_[id]->bush_list)
	{
		// view_list에 있는 객체가 new_list에 없다면 remove_list에 추가
		if (new_bush_list.count(vi) == 0)
		{
			remove_bush_list.push_back(vi);
		}
	}
	//remove_list 객체들을 bush_list에서 제외
	for (auto ri : remove_bush_list)
	{
		clients_[id]->bush_list.erase(ri);
	}
	clients_[id]->bl_lock.unlock();
	for (auto ri : remove_bush_list)
	{
		// NPC는 아래의 뷰리스트 업데이트가 필요 없음
		if (ri >= MAX_PLAYER_SEED) continue;

		if (clients_.find(ri) == clients_.end()) continue;
		clients_[ri]->vl_lock.lock();
		if (clients_[ri]->view_list.count(id) != 0 && getDistance(ri,id) > 1)
		{
			clients_[ri]->view_list.erase(id);
			clients_[ri]->vl_lock.unlock();
			sendRemoveObject(ri, id);
		}
		else 	clients_[ri]->vl_lock.unlock();
	}


	// 뷰 리스트에서 나가는 객체 처리
	vector <int> remove_list;
	clients_[id]->vl_lock.lock();
	for (auto vi : clients_[id]->view_list)
	{
		// view_list에 있는 객체가 new_list에 없다면 remove_list에 추가
		if (new_list.count(vi) == 0)
		{
			remove_list.push_back(vi);
		}
	}
	//remove_list 객체들을 view_list에서 제외
	for (auto ri : remove_list)
	{
		clients_[id]->view_list.erase(ri);
		sendRemoveObject(id, ri);
	}
	clients_[id]->vl_lock.unlock();

	for (auto ri : remove_list)
	{
		// NPC는 아래의 뷰리스트 업데이트가 필요 없음
		if (ri >= MAX_PLAYER_SEED) continue;
		clients_[id]->bl_lock.lock();
		if (clients_[id]->bush_list.count(ri) != 0)
		{
			clients_[id]->bl_lock.unlock();
			continue;
		}
		clients_[id]->bl_lock.unlock();

		if (clients_.find(ri) == clients_.end()) continue;
		clients_[ri]->vl_lock.lock();
		if (clients_[ri]->view_list.count(id) != 0)
		{
			clients_[ri]->view_list.erase(id);
			clients_[ri]->vl_lock.unlock();
			sendRemoveObject(ri, id);
		}
		else 	clients_[ri]->vl_lock.unlock();
	}

}

unordered_set<int> NetworkManager::getNpcViewList(int id)
{
	if (id < MAX_PLAYER_SEED) return unordered_set<int>();

	int min_y = npcs_[id]->pos_.y - VIEW_SPACE;
	int min_x = npcs_[id]->pos_.x - VIEW_SPACE;
	if (min_y < 0) min_y = 0;
	if (min_x < 0) min_x = 0;
	int max_y = npcs_[id]->pos_.y + VIEW_SPACE;
	int max_x = npcs_[id]->pos_.x + VIEW_SPACE;
	if (max_y > BOARD_ROW) max_y = BOARD_ROW;
	if (max_x > BOARD_COL) max_x = BOARD_COL;

	POINT myPos = npcs_[id]->pos_;
	unordered_set <int> view_list;
	mapLock_.lock();
	for (int y = min_y; y < max_y; ++y)
	{
		for (int x = min_x; x < max_x; ++x)
		{
			if (collisionMap_[y][x] >= MAX_PLAYER_SEED) continue;
			if (collisionMap_[y][x] > 0) 
			{
				int p_id = collisionMap_[y][x];
				if (grassMap_[myPos.y][myPos.x] != GRASS_TYPE::BUSH) //npc는 부쉬밖
				{
					view_list.insert(p_id);
				}
				else // npc는 부쉬안
				{
					if (getDistance(p_id,id) == 1)
					{
						view_list.insert(p_id);
					}
				}
			}
		}
	}
	mapLock_.unlock();
	return view_list;
}




void NetworkManager::startServer()
{
	this->createListenSocket();
	this->createThread();

}

void NetworkManager::updateDB(int update_id)
{
	SQLWCHAR query[256] = L"EXEC dbo.update_player ";
	wchar_t strTemp[256];
	// id
	_itow_s(clients_[update_id]->dbID, strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// lv
	_itow_s(clients_[update_id]->player->getLv(), strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// exp
	_itow_s(clients_[update_id]->player->getExp(), strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// hp
	_itow_s(clients_[update_id]->player->getHp(), strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// x
	_itow_s(clients_[update_id]->player->pos_.x, strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// y
	_itow_s(clients_[update_id]->player->pos_.y, strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// item_red
	_itow_s(clients_[update_id]->player->items_[ITEM_RED_POTION], strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// item_blue
	_itow_s(clients_[update_id]->player->items_[ITEM_BLUE_POTION], strTemp, 10);
	wcscat_s(query, strTemp);
	wcscat_s(query, (wchar_t*)L", ");
	// item_green
	_itow_s(clients_[update_id]->player->items_[ITEM_GREEN_POTION], strTemp, 10);
	wcscat_s(query, strTemp);
	//wcscat_s(query, (wchar_t*)L", ");

	SQLRETURN retcode = SQLExecDirect(hstmt_, query, SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		printf(" id[%d] db_id[%d] data update! \n", update_id, clients_[update_id]->dbID);
	}
	else
	{
		//HandleDiagnosticRecord(hdbc_, SQL_HANDLE_STMT, retcode);
		printf(" sql position update error! \n");
	}
	SQLCancel(hstmt_);
}

void NetworkManager::disconnectAllClients()
{
	for (auto c : clients_)
	{
		if (c.first >= MAX_PLAYER_SEED) continue;
		if (c.second->isConnected == true) logOut(c.first);
	}
}

