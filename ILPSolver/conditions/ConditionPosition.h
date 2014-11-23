#pragma once

class PMOptixRenderer;

class ConditionPosition
{
public:
	virtual void apply(PMOptixRenderer *) = 0;
	virtual ~ConditionPosition(void);
protected:
	ConditionPosition(void);
};

