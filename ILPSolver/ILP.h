/* 
 * Copyright (c) 2014 Ignacio Avas
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/
#pragma once

#include "scene/Scene.h"
#include "renderer/PMOptixRenderer.h"
#include <QVector>

class Logger;
class QFile;
class QString;
class LightInSurface;
class SurfaceRadiosity;
class QDomDocument;

class ILP
{
public:
	static ILP fromFile(Logger *logger, const QString& filePath, PMOptixRenderer *renderer);
	void optimize();
private:
	ILP();
	void readScene(Logger *logger, QFile &file, const QString& fileName);
	void readConditions(QDomDocument& doc);
	void readOptimizationFunction(QDomDocument& doc);

	Scene *scene;
	QVector<LightInSurface *> conditions;
	SurfaceRadiosity *optimizationFunction;
	PMOptixRenderer *renderer;
};