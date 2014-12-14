#pragma once

#include "Evaluation.h"

class SurfaceRadiosityEvaluation: public Evaluation
{
private:
	float m_val;
	float m_radius;
public:
	SurfaceRadiosityEvaluation(float val = 0.0f, float radius = 0.0f);

	float val() const;
	float radius() const;

	virtual EvaluationResult::CompareResult compare(const Evaluation* other) const;
	virtual QString info() const;
	virtual QString infoShort() const;
};