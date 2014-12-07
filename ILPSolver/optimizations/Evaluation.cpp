#include "Evaluation.h"


Evaluation::Evaluation(void)
{
}


Evaluation::~Evaluation(void)
{
}

QString toString(EvaluationResult::CompareResult compareResult)
{
	switch(compareResult){
	case EvaluationResult::BETTER:
		return "Better";
	case EvaluationResult::WORSE:
		return "Worse";
	case EvaluationResult::SIMILAR:
		return "Similar";
	case EvaluationResult::EQUAL:
		return "Equal";
	default:
		throw std::invalid_argument("Unknown argument");
	}
}