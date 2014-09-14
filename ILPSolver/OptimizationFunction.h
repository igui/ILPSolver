/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include <QString>

class OptimizationFunction
{
public:
	enum Type { MINIMIZE_RADIANCE, MAXIMIZE_RADIANCE };

	OptimizationFunction(Type type, const QString& object);
	Type type() const;
	QString object() const; 
	virtual ~OptimizationFunction(void);
private:
	Type m_type;
	QString m_object;
};

