/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "Scene/Scene.h"
#include "OptimizationFunction.h"
#include "OptimizationCondition.h"

class Logger;

class ILP
{
public:
	static ILP fromFile(Logger *logger, const QString& filePath);
	const Scene *scene() const;
private:
	ILP();

	Scene *m_scene;
};