#pragma once

#include <QStringList>

class PMOptixRenderer;

class ConditionPosition
{
public:
	virtual void apply(PMOptixRenderer *) const = 0;
	virtual QStringList info() const = 0;
	virtual ~ConditionPosition(void) { };
};

