#pragma once

#include <QVector>

class SurfaceRadiosityEvaluation;
class ConditionPosition;


class Configuration
{
public:
	Configuration();
	Configuration(SurfaceRadiosityEvaluation *evaluation, const QVector<ConditionPosition *> &positions);
	SurfaceRadiosityEvaluation *evaluation() const;
	SurfaceRadiosityEvaluation *setEvaluation(SurfaceRadiosityEvaluation *evaluation);
	QVector<ConditionPosition *> positions() const;
	~Configuration();
private:
	SurfaceRadiosityEvaluation *m_evaluation;
	QVector<ConditionPosition *> m_positions;

};

