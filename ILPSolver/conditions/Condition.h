#pragma once

#include <QString>

class ConditionPosition;

class Condition
{
public:
	virtual ConditionPosition *pushMoveToNeighbourhood(float radius, unsigned int retries) = 0;
	virtual void popLastMovement() = 0;
	virtual  operator QString() = 0;
	virtual ~Condition(void);
protected:
	Condition(void);
};

