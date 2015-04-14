#pragma once

#include <QStringList>
#include <QVector>

class PMOptixRenderer;

class ConditionPosition
{
public:
	virtual void apply(PMOptixRenderer *) const = 0;
	virtual QVector<float> normalizedPosition() const = 0;
	virtual QStringList info() const = 0;
	virtual ~ConditionPosition(void) { };
};

