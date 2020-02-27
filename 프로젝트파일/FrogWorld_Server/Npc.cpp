#include "Npc.h"



Npc::Npc(int id, POINT pos, bool aggro, bool roaming)
{
	id_ = id;
	pos_ = pos;
	isAggro = aggro;
	isRoaming = roaming;
	isCombat = aggro;

	isActived = false;
	level_ = 1;
	hp_ = maxHp_ = level_*100;
	lookDir_ = DIR_LEFT;
	damage_ = 5;
	isAlive = true;


	if (isRoaming)
	{
		if (isAggro) { npcType_ = NPC_CRO; }
		else { npcType_ = NPC_PIG; }
	}
	else
	{
		if (isAggro) { npcType_ = NPC_RED_TREE; }
		else { npcType_ = NPC_BLACK_TREE; }
	}
}


Npc::~Npc()
{
}

void Npc::setPos(int x, int y)
{
	posLock_.lock();
	pos_.x = x;
	pos_.y = y;
	posLock_.unlock();
}

void Npc::Move(ACTION_TYPE dir, unsigned int(*map)[BOARD_COL], mutex* lock, int(*grassMap)[BOARD_COL])
{
	posLock_.lock();
	switch (dir)
	{
	case ACTION_TYPE::DIR_UP:
		lookDir_ = DIR_UP;
		if (pos_.y > 0)
		{
			lock->lock();
			if (map[pos_.y - 1][pos_.x] == 0 && grassMap[pos_.y - 1][pos_.x] != GRASS_TYPE::HILL)
			{
				map[pos_.y][pos_.x] = 0;
				pos_.y--;
				map[pos_.y][pos_.x] = id_;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_DOWN:
		lookDir_ = DIR_DOWN;
		if (pos_.y < BOARD_ROW - 1)
		{
			lock->lock();
			if (map[pos_.y + 1][pos_.x] == 0 && grassMap[pos_.y + 1][pos_.x] != GRASS_TYPE::HILL)
			{
				map[pos_.y][pos_.x] = 0;
				pos_.y++;
				map[pos_.y][pos_.x] = id_;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_RIGHT:
		lookDir_ = DIR_RIGHT;
		if (pos_.x < BOARD_COL - 1)
		{
			lock->lock();
			if (map[pos_.y][pos_.x + 1] == 0 && grassMap[pos_.y][pos_.x + 1] != GRASS_TYPE::HILL)
			{
				map[pos_.y][pos_.x] = 0;
				pos_.x++;
				map[pos_.y][pos_.x] = id_;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_LEFT:
		lookDir_ = DIR_LEFT;
		if (pos_.x > 0)
		{
			lock->lock();
			if (map[pos_.y][pos_.x - 1] == 0 && grassMap[pos_.y][pos_.x - 1] != GRASS_TYPE::HILL)
			{
				map[pos_.y][pos_.x] = 0;
				pos_.x--;
				map[pos_.y][pos_.x] = id_;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_STOP:
		break;
	}
	posLock_.unlock();

}

int Npc::getHp()
{
	hpLock_.lock();
	int hp = hp_;
	hpLock_.unlock();
	return hp;
}

int Npc::getLv()
{
	lvLock_.lock();
	int lv = level_;
	lvLock_.unlock();
	return lv;
}

void Npc::suffer(int damage)
{
	hpLock_.lock();
	hp_ -= damage;
	if (hp_ < 0) hp_ = 0;
	hpLock_.unlock();
}

