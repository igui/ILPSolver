#include "SurfaceRadiosityEvaluation.h"

SurfaceRadiosityEvaluation::SurfaceRadiosityEvaluation(float val, float radius):
 val(val),
 radius(radius)
{
}
EvaluationResult::CompareResult SurfaceRadiosityEvaluation::compare(const SurfaceRadiosityEvaluation* other) const
{
	if((val - radius) > (other->val + other->radius))
		return EvaluationResult::BETTER;
	else if((val + radius) < (other->val - other->radius))
		return EvaluationResult::WORSE;
	else if(radius == other->radius && val == other->val)
		return EvaluationResult::EQUAL;
	else
		return EvaluationResult::SIMILAR;
}

bool SurfaceRadiosityEvaluation::better(const SurfaceRadiosityEvaluation* other) const
{
	return (val - radius) > (other->val + other->radius);
}

SurfaceRadiosityEvaluation::operator QString() const
{
	return QString::number(val-radius, 'f', 2) + ";" + QString::number(val, 'f', 2) + ";" + QString::number(val+radius, 'f', 2);
}
