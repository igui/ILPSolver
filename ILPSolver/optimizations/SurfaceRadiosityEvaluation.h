#pragma once

#include "Evaluation.h"

class SurfaceRadiosityEvaluation: public Evaluation
{
private:
	float val;
	float radius;
public:
	SurfaceRadiosityEvaluation(float val = 0.0f, float radius = 0.0f);

	virtual EvaluationResult::CompareResult compare(const Evaluation* other) const;
	virtual QString info() const;
	virtual QString infoShort() const;
};