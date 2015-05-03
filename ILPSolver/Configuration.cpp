#include "Configuration.h"

Configuration::Configuration():
	m_evaluation(NULL)
{
}

Configuration::Configuration(SurfaceRadiosityEvaluation *evaluation, const QVector<ConditionPosition *> &positions):
		m_evaluation(evaluation),
		m_positions(positions)
{
}

SurfaceRadiosityEvaluation *Configuration::evaluation() const
{
	return m_evaluation;
}

SurfaceRadiosityEvaluation *Configuration::setEvaluation(SurfaceRadiosityEvaluation *evaluation) 
{
	auto old = m_evaluation;
	m_evaluation = evaluation;
	return old;
}


QVector<ConditionPosition *> Configuration::positions() const
{
	return m_positions;
}

Configuration::~Configuration()
{
}
