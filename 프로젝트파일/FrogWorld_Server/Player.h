#pragma once
#include "stdafx.h"

struct action_node
{
	ACTION_TYPE				action;
	unsigned int			startTime;
};

class cmp_move {
public:
	bool operator() (const action_node& lhs, const action_node& rhs) const
	{
		return (lhs.startTime > rhs.startTime);
	}
};


class Player
{
private:
	mutex			posLock_;
	mutex			hpLock_;
	mutex			lvLock_;
	mutex			expLock_;
	mutex			actionLock_;
	priority_queue<action_node, vector<action_node>, cmp_move>  action_priority_Queue;
public:
	int				id_;
	int				items_[MAX_ITEM_TYPE];
	POINT			pos_;
	int				hp_;
	int				maxHp_;
	int				exp_;
	int				maxExp_;
	int				level_;
	ACTION_TYPE		lookDir_;
	ACTION_TYPE		action_;
	int				damage_;
	bool			usingBluePotion_;
	bool			usingGreenPotion_;
	int				heartBeatTick_;

public:
	Player(int id);
	Player(int id, POINT pos, int lv, int hp, int exp);
	~Player();
	void			initialize(int id, POINT pos, int lv, int hp, int exp);
	ACTION_TYPE		HeartBeat(unsigned int(*map)[BOARD_COL], mutex* lock, int(*grassMap)[BOARD_COL]);
	bool			Recover();
	int				getHp();
	int				getLv();
	int				getExp();
	void			setHp(int hp);
	void			setLv(int lv);
	void			push(action_node moveNode);
	void			suffer(unsigned int damage);
	void			addExp(int exp);
	bool			useItem(ITEM_TYPE item);
	void			stopItem(ITEM_TYPE item);

};

