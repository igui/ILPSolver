#pragma once

class Evaluation;
class QString;

class OptimizationFunction
{
public:
	virtual Evaluation *evaluateFast() = 0;
	virtual Evaluation *evaluateRadiosity() = 0;
	virtual void saveImage(const QString &fileName) = 0;
	virtual ~OptimizationFunction(void);
protected:
	OptimizationFunction(void);
};

