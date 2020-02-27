#pragma once
#include "stdafx.h"

class Npc
{
private:
	mutex				posLock_;
	mutex				hpLock_;
	mutex				lvLock_;
	mutex				expLock_;
public:
	int					id_;
	bool				isAlive;
	bool				isActived;
	bool				isAggro;
	bool				isRoaming;

	bool				isCombat;

	POINT				pos_;
	int					hp_;
	int					maxHp_;
	int					level_;
	NPC_TYPE			npcType_;
	ACTION_TYPE			lookDir_;
	int					damage_;
	lua_State*			Lua_;
	mutex				luaLock_;
	mutex				pathLock_;

public:
	Npc(int id, POINT pos,bool aggro, bool roaming);
	~Npc();
	void				setPos(int x, int y);
	void				Move(ACTION_TYPE dir, unsigned int(*map)[BOARD_COL], mutex* lock, int(*grassMap)[BOARD_COL]);
	int					getHp();
	int					getLv();
	void				suffer(int damage);
};

