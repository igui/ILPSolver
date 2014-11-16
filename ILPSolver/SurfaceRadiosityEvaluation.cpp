#include "SurfaceRadiosityEvaluation.h"

SurfaceRadiosityEvaluation::SurfaceRadiosityEvaluation(float val):
 val(val){
}

bool SurfaceRadiosityEvaluation::better(const SurfaceRadiosityEvaluation& other) const
{
	return val > other.val;
}

bool SurfaceRadiosityEvaluation::better(const SurfaceRadiosityEvaluation* other) const
{
	return val > other->val;
}

SurfaceRadiosityEvaluation::operator QString() const
{
	return QString::number(val, 'f', 2);
}
