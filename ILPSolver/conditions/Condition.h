#pragma once

#include <QStringList>

class ConditionPosition;

class Condition
{
public:
	virtual ConditionPosition *pushMoveToNeighbourhood(float radius, unsigned int retries) = 0;
	virtual void popLastMovement() = 0;
	virtual QStringList info() = 0;
	virtual QStringList header() = 0;
	virtual ~Condition(void);
protected:
	Condition(void);
};

