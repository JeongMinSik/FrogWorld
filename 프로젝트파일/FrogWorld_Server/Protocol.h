#pragma once

#define		SERVER_PORT			4000
#define		SOCKET_BUF_SIZE		1024*10
#define		PACKET_BUF_SIZE		1024
#define		MAX_PLAYER_COUNT	2500
#define		MAX_PLAYER_SEED		4000
#define		MAX_NPC_COUNT		1000
#define		MAX_NPC_SEED		(MAX_PLAYER_SEED + MAX_NPC_COUNT)

#define		MAX_CHAT_BUF		128

#define		BOARD_COL			100
#define		BOARD_ROW			100

typedef enum
{
	PAK_LOGIN,
	PAK_PUT_OBJECT,
	PAK_REMOVE_OBJECT,
	PAK_REQ_MOVE,
	PAK_REQ_ATTACK,
	PAK_MOVE_OBJECT,
	PAK_REQ_CHAT,
	PAK_ANS_CHAT,
	PAK_STAT_CHANGE,
	PAK_LOGIN_FAIL,
	PAK_GET_ITEM,
	PAK_USE_ITEM
} PAK_ID;

typedef enum
{
	DIR_STOP,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN,
	ATTACK
}ACTION_TYPE;
#define MAX_MOVE_DIR (5)

typedef enum
{
	NPC_BLACK_TREE,
	NPC_RED_TREE,
	NPC_PIG,
	NPC_CRO
}NPC_TYPE;
#define MAX_NPC_TYPE (4)

typedef enum
{
	ITEM_RED_POTION,
	ITEM_BLUE_POTION,
	ITEM_GREEN_POTION
}ITEM_TYPE;
#define MAX_ITEM_TYPE (3)

typedef enum
{
	WRONG_ID,
	CONNECTING
}FAIL_REASON;


///////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)

struct HEADER
{
	USHORT		size;
	PAK_ID		packetID;
};

struct S_LOGIN_FAIL
{
	HEADER			header;
	FAIL_REASON		reason;
};

struct S_LOGIN
{
	HEADER			header;
	int				id;
	POINT			position;
	int				level;
	int				hp;
	int				exp;
	int				items[MAX_ITEM_TYPE];
};

struct S_PUT_OBJECT
{
	HEADER			header;
	int				id;
	POINT			position;
	int				hp;
	int				maxHp;
	int				level;
	int				npc_type;
	ACTION_TYPE		look;
};

struct S_MOVE_OBJECT
{
	HEADER			header;
	int				id;
	POINT			position;
	ACTION_TYPE		look;
};


struct S_REMOVE_OBJECT
{
	HEADER		header;
	int			id;
};

struct S_CHAT
{
	HEADER		header;
	char		buf[MAX_CHAT_BUF];
};

struct S_STAT_CHANGE
{
	HEADER		header;
	int			id;
	int			level;
	int			hp;
	int			exp;
};

struct S_GET_ITEM
{
	HEADER		header;
	ITEM_TYPE	item;
};

struct S_USE_ITEM
{
	HEADER		header;
	ITEM_TYPE	item;
};

struct C_LOGIN
{
	HEADER		header;
	int			id;
};

struct C_MOVE
{
	HEADER		header;
	BYTE		dir;
};

struct C_CHAT
{
	HEADER		header;
	char		buf[MAX_CHAT_BUF];
};

struct C_USE_ITEM
{
	HEADER		header;
	ITEM_TYPE	item;
};

#pragma pack(pop)
////////////////////////////////////////////////////////////////////////////////////////