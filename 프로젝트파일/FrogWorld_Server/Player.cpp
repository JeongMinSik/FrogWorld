#include "Player.h"

Player::Player(int id)
{
	initialize(id, POINT{ 0,0 }, 1, 100, 0);
}

Player::Player(int id, POINT pos, int lv, int hp, int exp)
{
	initialize(id, pos, lv, hp, exp);
}


Player::~Player()
{
}

void Player::initialize(int id, POINT pos, int lv, int hp, int exp)
{
	for (int& item : items_) item = 0;
	id_ = id;
	pos_ = pos;
	hp_ = hp;
	exp_ = exp;
	level_ = lv;
	maxHp_ = lv * 100;
	maxExp_ = lv * 100;
	action_ = DIR_STOP;
	lookDir_ = DIR_LEFT;
	damage_ = 5;
	usingBluePotion_ = usingGreenPotion_ = false;
	heartBeatTick_ = 100;
}

ACTION_TYPE Player::HeartBeat(unsigned int (*map)[BOARD_COL], mutex* lock, int (*grassMap)[BOARD_COL])
{
	actionLock_.lock();
	if(action_priority_Queue.empty() == false)
	{
		action_node m = action_priority_Queue.top();
		action_priority_Queue.pop();
		actionLock_.unlock();

		if (m.startTime + 3000 > GetTickCount())
		{
			action_ = m.action;
		}
		else
		{
			actionLock_.lock();
			int size = (int)action_priority_Queue.size();
			for (int i = 0; i < size; ++i)
			{
				action_priority_Queue.pop();
			}
			actionLock_.unlock();
			action_ = DIR_STOP;
		}
		actionLock_.lock();
	}
	actionLock_.unlock();

	posLock_.lock();
	switch (action_)
	{
	case ACTION_TYPE::DIR_UP:
		lookDir_ = DIR_UP;
		if ( (pos_.y > 0))
		{
			lock->lock();
			if (map[pos_.y - 1][pos_.x] == 0)
			{
				if (grassMap[pos_.y - 1][pos_.x] != GRASS_TYPE::HILL)
				{
					map[pos_.y][pos_.x] = 0;
					pos_.y--;
					map[pos_.y][pos_.x] = id_;
				}
			}
			else
			{
				action_ = ACTION_TYPE::DIR_STOP;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_DOWN:
		lookDir_ = DIR_DOWN;
		if (pos_.y < BOARD_ROW - 1)
		{
			lock->lock();
			if (map[pos_.y + 1][pos_.x] == 0)
			{
				if (grassMap[pos_.y + 1][pos_.x] != GRASS_TYPE::HILL)
				{
					map[pos_.y][pos_.x] = 0;
					pos_.y++;
					map[pos_.y][pos_.x] = id_;
				}
			}
			else
			{
				action_ = ACTION_TYPE::DIR_STOP;
			}
			lock->unlock();
		}
		break; 
	case ACTION_TYPE::DIR_RIGHT:
		lookDir_ = DIR_RIGHT;
		if (pos_.x < BOARD_COL - 1)
		{
			lock->lock();
			if (map[pos_.y][pos_.x + 1] == 0)
			{
				if (grassMap[pos_.y][pos_.x + 1] != GRASS_TYPE::HILL)
				{
					map[pos_.y][pos_.x] = 0;
					pos_.x++;
					map[pos_.y][pos_.x] = id_;
				}
			}
			else
			{
				action_ = ACTION_TYPE::DIR_STOP;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_LEFT:
		lookDir_ = DIR_LEFT;
		if (pos_.x > 0)
		{
			lock->lock();
			if (map[pos_.y][pos_.x - 1] == 0)
			{
				if (grassMap[pos_.y][pos_.x - 1] != GRASS_TYPE::HILL)
				{
					map[pos_.y][pos_.x] = 0;
					pos_.x--;
					map[pos_.y][pos_.x] = id_;
				}
			}
			else
			{
				action_ = ACTION_TYPE::DIR_STOP;
			}
			lock->unlock();
		}
		break;
	case ACTION_TYPE::DIR_STOP:
		break;
	case ACTION_TYPE::ATTACK:
		break;
	}
	posLock_.unlock();

	return action_;

}

bool Player::Recover()
{
	//printf("debug: hp:%d \n", hp_);

	if (hp_ == maxHp_) return false;

	hpLock_.lock();
	hp_ += int(maxHp_ * 0.1f);
	if (hp_ > maxHp_) { hp_ = maxHp_; };
	hpLock_.unlock();
	return true;

}

int Player::getHp()
{
	hpLock_.lock();
	int hp = hp_;
	hpLock_.unlock();
	return hp;
}

int Player::getLv()
{
	lvLock_.lock();
	int lv = level_;
	lvLock_.unlock();
	return lv;
}

int Player::getExp()
{
	expLock_.lock();
	int exp = exp_;
	expLock_.unlock();
	return exp;
}

void Player::setHp(int hp)
{
	hpLock_.lock();
	hp_ = hp;
	if (hp_ > maxHp_) hp_ = maxHp_;
	hpLock_.unlock();
}

void Player::setLv(int lv)
{
	lvLock_.lock();
	if (level_ == lv)
	{
		lvLock_.unlock();
		return;
	}
	level_ = lv;
	lvLock_.unlock();

	maxHp_ = lv * 100;
	maxExp_ = lv * 100;
	
	hpLock_.lock();
	if (hp_ > maxHp_) hp_ = maxHp_;
	hpLock_.unlock();

	expLock_.lock();
	if (exp_ > maxExp_) exp_ = maxExp_-1;
	expLock_.unlock();
	
}

void Player::push(action_node moveNode)
{
	actionLock_.lock();
	action_priority_Queue.push(moveNode);
	actionLock_.unlock();
}

void Player::suffer(unsigned int damage)
{
	hpLock_.lock();
	hp_ -= damage;
	if (hp_ < 0) hp_ = 0;
	hpLock_.unlock();
}

void Player::addExp(int exp)
{
	expLock_.lock();
	exp_ += exp;
	if (exp_ < 0) exp_ = 0;
	expLock_.unlock();

	expLock_.lock();
	if (exp_ >= maxExp_)
	{
		exp_ -= maxExp_;
		expLock_.unlock();
		setLv(getLv() + 1);
	}
	else
	{
		expLock_.unlock();
	}
}

bool Player::useItem(ITEM_TYPE item)
{
	if (items_[item] <= 0) return false;

	switch (item)
	{
	case ITEM_RED_POTION:
		if (false == Recover()) return false;
		break;
	case ITEM_BLUE_POTION:
		if (true == usingBluePotion_) return false;
		usingBluePotion_ = true;
		damage_ *= 2;
		break;
	case ITEM_GREEN_POTION:
		if (true == usingGreenPotion_) return false;
		usingGreenPotion_ = true;
		heartBeatTick_ /= 2;
		break;
	}
	items_[item]--;

	return true;

}

void Player::stopItem(ITEM_TYPE item)
{
	switch (item)
	{
	case ITEM_BLUE_POTION:
		if (false == usingBluePotion_) return;
		usingBluePotion_ = false;
		damage_ /= 2;
		break;
	case ITEM_GREEN_POTION:
		if (false == usingGreenPotion_) return;
		usingGreenPotion_ = false;
		heartBeatTick_ *= 2;
		break;
	}
}


