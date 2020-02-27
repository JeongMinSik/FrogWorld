#pragma once

#include "stdafx.h"
#include "aStarLibrary.h"
#include "Player.h"
#include "Npc.h"

#define		VIEW_SPACE			4

#define		THREAD_COUNT		4


enum class OPERATION_TYPE
{
	OP_SEND = 0,
	OP_RECV,
	OP_PLAYER_MOVE,
	OP_RECOVER,
	OP_HEARTBEAT_PLAYER,
	OP_STOP_BLUE_POTION,
	OP_STOP_GREEN_POTION,
	OP_NPC_AI_SELF
};
struct overlappedEx
{
	OVERLAPPED			overlapped;
	WSABUF				wsabuf;
	char				IOCP_buf[SOCKET_BUF_SIZE];
	OPERATION_TYPE		optype;
};

struct clientInfo
{
	int						dbID;
	char					saveBuf[PACKET_BUF_SIZE];
	int						iCurrPacketSize;
	int						iStoredPacketSize;

	SOCKET					socket;
	overlappedEx			overEx;

	volatile bool					isConnected;
	Player*					player;
	unordered_set <int>		view_list;
	mutex					vl_lock;
	unordered_set <int>		bush_list;
	mutex					bl_lock;
};

struct event_node
{
	volatile int			id;
	volatile unsigned int	startTime;
	OPERATION_TYPE			type;
};

class cmp_event {
	bool reverse;
public:
	cmp_event() {}
	bool operator() (const event_node lhs, const event_node rhs) const
	{
		return (lhs.startTime > rhs.startTime);
	}
};

class NetworkManager 
{
private:
	SOCKET						listenSocket_;
	HANDLE						iocp_;
	thread						*acceptThread_;
	thread						*workerThread_[THREAD_COUNT];
	thread						*timerThread_;
	map<UINT, clientInfo*>		clients_;
	map<UINT, Npc*>				npcs_;
	UINT						playerCount_;
	UINT						seedID_;
	priority_queue<event_node,vector<event_node>, cmp_event>  timer_priority_Queue;
	mutex						timerLock_;

	// DB
	SQLHENV						henv_;
	SQLHDBC						hdbc_;
	SQLHSTMT					hstmt_;

	unsigned int				collisionMap_[BOARD_ROW][BOARD_COL];
	int							grassMap_[BOARD_COL][BOARD_ROW];
	mutex						mapLock_;
	mutex						pathLock_;

private:
	void					error_display(char *msg, int err_no);

	bool					initializeServer();
	bool					initializeNPC();
	bool					initializeDB();
	bool					loadGrassMap();
	bool					createListenSocket();
	bool					createThread();
	bool					acceptThread();
	bool					workerThread();
	bool					timerThread();
	void					processPacket(int id, int recvByte);
	void					processContents(int id);

	void					sendBufData(int id, char* buf);

	void					login_ok(int in_id);
	void					login_fail(int in_id, FAIL_REASON reason);
	void					sendLogIn_fail(int recv_id, FAIL_REASON reason);
	void					logOut(int out_id);
	void					sendLogIn(int recv_id);
	void					sendPutObject(int recv_id, int new_id);
	void					sendMoveObject(int recv_id, int move_id);
	void					sendRemoveObject(int recv_id, int rmv_id);
	void					sendChat(int recv_id, char* buf);
	void					sendGetItem(int recv_id, ITEM_TYPE item);
	void					sendUseItem(int recv_id, ITEM_TYPE item);
	void					sendStat(int recv_id, int hp_id);
	
	void					proceedAttack(int id);
	void					wakeupNpc(int id);
	void					doNpcAi(int npc_id);
	void					heartbeatPlayer(int id);
	void					recoverPlayer(int id);
	void					stopUsingItem(int id, ITEM_TYPE item);

	int						giveExp(int get_id, int npc_id);
	bool					canEnterSpace(int x, int y);

	void					updatePlayerViewList(int id);
	unordered_set<int>		getNpcViewList(int id);
	void					setWalkabilityMap();
	int						getDistance(int id_1, int id_2);

	int						API_SetActived(lua_State * L);
	int						API_Is_Visible(lua_State * L);
	int						API_Move_Random(lua_State * L);
	int						API_GetX(lua_State * L);
	int						API_GetY(lua_State * L);
	int						API_GetTargetID(lua_State * L);
	int						API_Ai_Start(lua_State * L);
	int						API_Attack(lua_State * L);
	int						API_FindPath(lua_State * L);

public:
	NetworkManager();
	~NetworkManager();

	void		startServer();
	void		updateDB(int id);
	void		disconnectAllClients();
};