#include "Interval.h"
#include <cmath>
#include <stdexcept>

Interval::Interval(float center, float radius):
	m_center(center),
	m_radius(radius)
{
	if(m_radius <= 0)
		throw std::invalid_argument("radius must be positive");
}

Interval Interval::fromTwoPoints(float a, float b)
{
	float radius = fabsf(a - b) / 2.0f;
	float center = (a + b) / 2.0f;
	return Interval(center, radius);
}

float Interval::center() const
{
	return m_center;
}

float Interval::radius() const
{
	return m_radius;
}

bool Interval::intersects(const Interval& other) const
{
	return fabsf(m_center - other.m_center) < m_radius + other.m_radius;
}

bool Interval::operator == (const Interval& other) const
{
	return m_center == other.m_center && m_radius == other.m_radius;
}

bool Interval::operator < (const Interval& other) const
{
	return m_center < other.m_center && (other.m_center - m_center < m_radius + other.m_radius);
}


bool Interval::operator > (const Interval& other) const	
{
	return m_center > other.m_center && (m_center - other.m_center < m_radius + other.m_radius);
}

bool Interval::operator != (const Interval& other) const
{
	return m_center != other.m_center || m_radius != other.m_radius; 
}
