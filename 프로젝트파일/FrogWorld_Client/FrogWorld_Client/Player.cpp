#include "stdafx.h"
#include "Player.h"


Player::Player()
{
	initialize(POINT{ 0,0 }, 1, 0, 0, -1, ACTION_TYPE::DIR_LEFT);
}

Player::Player(POINT pos, int lv, int hp, int exp, int npc_type, ACTION_TYPE look)
{
	initialize(pos, lv, hp, exp, npc_type, look);
}


Player::~Player()
{
}

void Player::initialize(POINT pos, int lv, int hp, int exp, int npc_type, ACTION_TYPE look)
{
	for (int& item : items_) item = 0;
	pos_ = pos;
	hp_ = hp;
	exp_ = exp;
	level_ = lv;
	maxHp_ = lv * 100;
	maxExp_ = lv * 100;
	action_ = DIR_STOP;
	lookDir_ = look;
	npcType_ = npc_type;
	isSuffing_ = false;
	attackFrame_ = 0;
}

void Player::setPos(POINT p, ACTION_TYPE look)
{
	pos_ = p;
	lookDir_ = look;
}

void Player::setLv(int lv)
{
	if (level_ == lv) return;
	level_ = lv;
	maxExp_ = lv * 100;
	maxHp_ = lv * 100;
}
