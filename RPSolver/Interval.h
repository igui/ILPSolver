#pragma once
class Interval
{
public:
	Interval(float center, float radius);
	static Interval fromTwoPoints(float a, float b);
	float center() const;
	float radius() const;
	float bottom() const;
	float top() const;
	bool intersects(const Interval& other) const;
	Interval intersection(const Interval &other) const;
	Interval inclusion(const Interval& other) const;
	bool operator == (const Interval& other) const;

	// returns
	bool operator < (const Interval& other) const;
	bool operator > (const Interval& other) const;
	bool operator != (const Interval& other) const;
private:
	float m_center, m_radius;
};

