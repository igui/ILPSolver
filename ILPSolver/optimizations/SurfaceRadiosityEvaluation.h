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
public:
	SurfaceRadiosityEvaluation(float val, float radius, int photons, bool isMaxQuality);

	float val() const;
	float radius() const;
	int photons() const;
	Interval interval () const;
	bool isMaxQuality() const;
	virtual QString info() const;
	virtual QString infoShort() const;
};