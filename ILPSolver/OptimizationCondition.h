/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "Math/AAB.h"
#include <QString>

class OptimizationCondition
{
public:
	enum Type { AABB, SURFACE, MESH  };

	OptimizationCondition(const QString& variable, Type type, const AAB& aabb);
	OptimizationCondition(const QString& variable, Type type, const QString& m_enclosingObject);

	Type type() const;
	QString variable() const;
	AAB aabb() const;
	QString enclosingObject() const;
	
	virtual ~OptimizationCondition();
private:
	Type m_type;
	QString m_variable;
	AAB m_aabb;
	QString m_enclosingObject;
};

