#pragma once

#include <QString>
#include "Interval.h"


class SurfaceRadiosityEvaluation
{
private:
	float m_val;
	float m_radius;
	int m_photons;
	Interval m_interval;
	bool m_isMaxQuality;
	bool m_isValid;
public:
	SurfaceRadiosityEvaluation(float val, float radius, int photons, bool isMaxQuality, bool isValid);

	enum ComparisonResult {
		BETTER,
		WORSE,
		INDISTINGUISHABLE
	};

	bool valid() const;
	float val() const;
	float radius() const;
	int photons() const;
	Interval interval () const;
	bool isMaxQuality() const;
	virtual QString info() const;
	virtual QString infoShort() const;

	ComparisonResult compareWith(const SurfaceRadiosityEvaluation* other) const;
};