#pragma once

#include <QStringList>

class Evaluation;
class PMOptixRenderer;
class QImage;
class Logger;
class Scene;
class Camera;

class OptimizationFunction
{
public:
	virtual QStringList header() = 0;
	Evaluation *evaluateRadiosity();
	Evaluation *evaluateFast(float quality);
	void saveImage(const QString &fileName);	
	virtual ~OptimizationFunction();
protected:
	OptimizationFunction(PMOptixRenderer *renderer, Scene *scene, Logger *logger);
	PMOptixRenderer *renderer();
	virtual Evaluation *genEvaluation(int nPhotons) = 0;
private:
	void saveImageAsync(const QString& fileName, QImage* image);


private:
	static const unsigned int sampleImageWidth;
	static const unsigned int sampleImageHeight;
	static const unsigned int minPhotonWidth;
	unsigned int maxPhotonWidth;
	static const float gammaCorrection;

	PMOptixRenderer *m_renderer;
	Scene *scene;
	Logger *logger;
	// for sampling images
	Camera *sampleCamera; 
};

