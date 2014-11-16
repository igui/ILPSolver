#pragma once

#include <QString>

class SurfaceRadiosityEvaluation
{
private:
	float val;
public:
	SurfaceRadiosityEvaluation(float val = 0.0f);

	bool better(const SurfaceRadiosityEvaluation& other) const;
	bool better(const SurfaceRadiosityEvaluation* other) const;
	operator QString() const;
};