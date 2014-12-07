#pragma once

#include <QString>

namespace EvaluationResult
{
	enum CompareResult { BETTER, WORSE, EQUAL, SIMILAR };
};

QString toString(EvaluationResult::CompareResult compareResult);

class Evaluation
{
public:
	virtual EvaluationResult::CompareResult compare(const Evaluation* other) const = 0;
	virtual QString info() const = 0;
	virtual QString infoShort() const = 0;
	virtual ~Evaluation(void);
protected:
	Evaluation(void);
};

