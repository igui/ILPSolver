#pragma once

#include <QVector>

class Evaluation;
class ConditionPosition;


class Configuration
{
public:
	Configuration();
	Configuration(Evaluation *evaluation, const QVector<ConditionPosition *> &positions);
	Evaluation *evaluation();
	QVector<ConditionPosition *> positions();
	~Configuration();
private:
	Evaluation *m_evaluation;
	QVector<ConditionPosition *> m_positions;

};

