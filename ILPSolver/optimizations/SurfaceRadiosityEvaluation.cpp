#include "SurfaceRadiosityEvaluation.h"
#include <QLocale>

SurfaceRadiosityEvaluation::SurfaceRadiosityEvaluation(float val, float radius):
 val(val),
 radius(radius)
{
}

EvaluationResult::CompareResult SurfaceRadiosityEvaluation::compare(const Evaluation* eval) const
{
	auto other = dynamic_cast<const SurfaceRadiosityEvaluation *>(eval);
	if(other == NULL){
		throw std::invalid_argument("other");
	}

	if((val - radius) > (other->val + other->radius))
		return EvaluationResult::BETTER;
	else if((val + radius) < (other->val - other->radius))
		return EvaluationResult::WORSE;
	else if(radius == other->radius && val == other->val)
		return EvaluationResult::EQUAL;
	else
		return EvaluationResult::SIMILAR;
}

SurfaceRadiosityEvaluation::operator QString() const
{
	QLocale locale;
	return locale.toString(val-radius, 'f', 2) + ";" + locale.toString(val, 'f', 2) + ";" + locale.toString(val+radius, 'f', 2);
}
