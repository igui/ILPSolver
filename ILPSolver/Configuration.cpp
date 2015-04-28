#include "Configuration.h"

Configuration::Configuration():
	m_evaluation(NULL)
{
}

Configuration::Configuration(Evaluation *evaluation, const QVector<ConditionPosition *> &positions):
		m_evaluation(evaluation),
		m_positions(positions)
{
}

Evaluation *Configuration::evaluation() const
{
	return m_evaluation;
}

Evaluation *Configuration::setEvaluation(Evaluation *evaluation) 
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
