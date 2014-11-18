#pragma once

#include <QString>

namespace EvaluationResult
{
	enum CompareResult { BETTER, WORSE, EQUAL, SIMILAR };
};

class SurfaceRadiosityEvaluation
{
private:
	float val;
	float radius;
public:
	SurfaceRadiosityEvaluation(float val = 0.0f, float radius = 0.0f);

	EvaluationResult::CompareResult compare(const SurfaceRadiosityEvaluation* other) const;
	bool better(const SurfaceRadiosityEvaluation* other) const;
	operator QString() const;
};