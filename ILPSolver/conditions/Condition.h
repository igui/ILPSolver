#pragma once

#include <QStringList>
class ConditionPosition;

class Condition
{
public:
	virtual ConditionPosition *findNeighbour(ConditionPosition *from, float radius, unsigned int retries) const = 0;
	virtual ConditionPosition *initial() const = 0;
	virtual QStringList header() const = 0;
	virtual ~Condition(void) {};
};

