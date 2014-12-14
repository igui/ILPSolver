#include "SurfaceRadiosityEvaluation.h"
#include <QLocale>

SurfaceRadiosityEvaluation::SurfaceRadiosityEvaluation(float val, float radius):
 m_val(val),
 m_radius(radius)
{
}

EvaluationResult::CompareResult SurfaceRadiosityEvaluation::compare(const Evaluation* eval) const
{
	auto other = dynamic_cast<const SurfaceRadiosityEvaluation *>(eval);
	if(other == NULL){
		throw std::invalid_argument("other");
	}

	if((m_val - m_radius) > (other->m_val + other->m_radius))
		return EvaluationResult::BETTER;
	else if((m_val + m_radius) < (other->m_val - other->m_radius))
		return EvaluationResult::WORSE;
	else if(m_radius == other->m_radius && m_val == other->m_val)
		return EvaluationResult::EQUAL;
	else
		return EvaluationResult::SIMILAR;
}

float SurfaceRadiosityEvaluation::val() const
{
	return m_val;
}

float SurfaceRadiosityEvaluation::radius() const
{
	return m_radius;
}

QString SurfaceRadiosityEvaluation:: info() const
{
	QLocale locale;
	return locale.toString(m_val-m_radius, 'f', 2) + ";" + locale.toString(m_val, 'f', 2) + ";" + locale.toString(m_val+m_radius, 'f', 2);
}

QString SurfaceRadiosityEvaluation:: infoShort() const
{
	QLocale locale;
	return locale.toString(m_val, 'f', 2);
}

