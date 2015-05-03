#include "Interval.h"
#include <cmath>
#include <stdexcept>

Interval::Interval(float center, float radius):
	m_center(center),
	m_radius(radius)
{
	if(m_radius < 0)
		throw std::invalid_argument("radius can't be negative");
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

float Interval::top() const
{
	return m_center + m_radius;
}

float Interval::bottom() const 
{
	return m_center - m_radius;
}

bool Interval::intersects(const Interval& other) const
{
	return fabsf(m_center - other.m_center) < m_radius + other.m_radius;
}

Interval Interval::intersection(const Interval &other) const 
{
	auto a = std::max(m_center - m_radius, other.m_center - other.m_radius);
	auto b = std::min(m_center + m_radius, other.m_center + other.m_radius);

	auto center = (a + b) / 2.0f;
	// a > b implies a non-intersection between intervals. An Interval with radius 0 is returned in that case
	auto radius  = std::max((a - b) / 2.0f, 0.0f); 

	return Interval(center, radius);
}

bool Interval::operator == (const Interval& other) const
{
	return m_center == other.m_center && m_radius == other.m_radius;
}

bool Interval::operator < (const Interval& other) const
{
	return top() < other.bottom();
}


bool Interval::operator > (const Interval& other) const	
{
	return bottom() > other.top();
}

bool Interval::operator != (const Interval& other) const
{
	return m_center != other.m_center || m_radius != other.m_radius; 
}
