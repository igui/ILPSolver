#pragma once
class Interval
{
public:
	Interval(float center, float radius);
	static Interval fromTwoPoints(float a, float b);
	float center() const;
	float radius() const;
	bool intersects(const Interval& other) const;
	bool operator == (const Interval& other) const;
	bool operator < (const Interval& other) const;
	bool operator > (const Interval& other) const;
	bool operator != (const Interval& other) const;
private:
	float m_center, m_radius;
};

