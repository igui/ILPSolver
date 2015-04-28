#pragma once

#include "Evaluation.h"

class SurfaceRadiosityEvaluation: public Evaluation
{
private:
	float m_val;
	float m_radius;
	int m_photons;
public:
	SurfaceRadiosityEvaluation(float val, float radius, int photons);

	float val() const;
	float radius() const;

	virtual EvaluationResult::CompareResult compare(const Evaluation* other) const;
	virtual QString info() const;
	virtual QString infoShort() const;
};