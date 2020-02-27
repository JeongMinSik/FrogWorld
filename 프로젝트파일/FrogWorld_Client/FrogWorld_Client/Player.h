#pragma once

class Player
{
public:
	int			items_[MAX_ITEM_TYPE];
	POINT		pos_;
	int			hp_;
	int			maxHp_;
	int			exp_;
	int			maxExp_;
	int			level_;
	ACTION_TYPE	action_;
	ACTION_TYPE	lookDir_;
	int			npcType_;
	int			attackFrame_;
	bool		isSuffing_;
public:
	Player();
	Player(POINT pos, int lv, int hp, int exp, int npc_type, ACTION_TYPE look = DIR_LEFT);
	~Player();

	void initialize(POINT pos, int lv, int hp, int exp, int npc_type, ACTION_TYPE look);

	void setPos(POINT p, ACTION_TYPE look);
	void setLv(int lv);
};

