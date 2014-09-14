/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "OptimizationCondition.h"

OptimizationCondition::OptimizationCondition(const QString& objectName, Type type, const AAB& aabb):
	m_variable(objectName),
	m_type(type),
	m_aabb(aabb)
{
}

OptimizationCondition::OptimizationCondition(const QString& objectName, Type type, const QString& enclosingObject):
	m_variable(objectName),
	m_type(type),
	m_enclosingObject(enclosingObject)
{
}

OptimizationCondition::Type OptimizationCondition::type() const
{
	return m_type;
}

QString OptimizationCondition::variable() const
{
	return m_variable;
}

QString OptimizationCondition::enclosingObject() const
{
	return m_enclosingObject;
}


AAB OptimizationCondition::aabb() const
{
	return m_aabb;
}

OptimizationCondition::~OptimizationCondition(void)
{
}
