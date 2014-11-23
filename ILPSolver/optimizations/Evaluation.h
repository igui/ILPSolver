#pragma once

#include <QString>

namespace EvaluationResult
{
	enum CompareResult { BETTER, WORSE, EQUAL, SIMILAR };
};

class Evaluation
{
public:
	virtual EvaluationResult::CompareResult compare(const Evaluation* other) const = 0;
	virtual operator QString() const = 0;
	virtual ~Evaluation(void);
protected:
	Evaluation(void);
};

