/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "StandaloneRenderManager.hxx"
#include "renderer/PPMOptixRenderer.h"
#include "renderer/PMOptixRenderer.h"
#include <QThread>
#include "renderer/Camera.h"
#include <QTime>
#include "scene/Scene.h"
#include "clientserver/RenderServerRenderRequest.h"
#include <QCoreApplication>
#include <QApplication>
#include "Application.hxx"

StandaloneRenderManager::StandaloneRenderManager(QApplication & qApplication, Application & application, const ComputeDevice& device) :
    m_device(device),
    m_renderer(new PPMOptixRenderer()), 
    m_nextIterationNumber(0),
    m_outputBuffer(NULL),
    m_currentScene(NULL),
    m_compileScene(false),
    m_application(application),
    m_noEmittedSignals(true),
	m_logger()
{
    connect(&application, SIGNAL(sequenceNumberIncremented()), this, SLOT(onSequenceNumberIncremented()));
    connect(&application, SIGNAL(runningStatusChanged()), this, SLOT(onRunningStatusChanged()));
    connect(&application.getSceneManager(), SIGNAL(sceneLoadingNew()), this, SLOT(onSceneLoadingNew()));
    connect(&application.getSceneManager(), SIGNAL(sceneUpdated()), this, SLOT(onSceneUpdated()));

    onSceneUpdated();

    connect(this, SIGNAL(continueRayTracing()), 
            this, SLOT(onContinueRayTracing()), 
            Qt::QueuedConnection);
}

SignalLogger& StandaloneRenderManager::logger()
{
	return m_logger;
}

StandaloneRenderManager::~StandaloneRenderManager()
{
    if(m_outputBuffer != NULL)
    {
        delete[] m_outputBuffer;
        m_outputBuffer = NULL;
    }
}

void StandaloneRenderManager::start()
{
    m_application.setRendererStatus(RendererStatus::INITIALIZING_ENGINE);
	m_renderer->initialize(m_device, &m_logger);
}

void StandaloneRenderManager::onContinueRayTracing()
{
    renderNextIteration();
    continueRayTracingIfRunningAsync();
}

void StandaloneRenderManager::renderNextIteration()
{
    try
    {
        if(m_application.getRunningStatus() == RunningStatus::RUNNING && m_currentScene != NULL)
        {
            m_noEmittedSignals = true;

            if(m_compileScene)
            {
                m_application.setRendererStatus(RendererStatus::INITIALIZING_SCENE);
                m_renderer->initScene(*m_currentScene);
                m_compileScene = false;

				calculateOptimalNumEmmitedPhotons();

                m_application.setRendererStatus(RendererStatus::RENDERING);
            }


			const double PPMAlpha = 2.0/3.0;
            QVector<unsigned long long> iterationNumbers;
            QVector<double> ppmRadii;

            RenderServerRenderRequestDetails details (m_camera, QByteArray(m_currentScene->getSceneName()), 
                m_application.getRenderMethod(), m_application.getWidth(), m_application.getHeight(), PPMAlpha);

            RenderServerRenderRequest renderRequest (m_application.getSequenceNumber(), iterationNumbers, ppmRadii, details);

            m_renderer->renderNextIteration(m_nextIterationNumber, m_nextIterationNumber, m_PPMRadius, renderRequest.getDetails());
            const double ppmRadiusSquared = m_PPMRadius*m_PPMRadius;
            const double ppmRadiusSquaredNew = ppmRadiusSquared*(m_nextIterationNumber+PPMAlpha)/double(m_nextIterationNumber+1);
            m_PPMRadius = sqrt(ppmRadiusSquaredNew);

            // Transfer the output buffer to CPU and signal ready for display
            if(m_outputBuffer == NULL)
            {
                m_outputBuffer = new float[2000*2000*3];
            }
            m_renderer->getOutputBuffer(m_outputBuffer);
            emit newFrameReadyForDisplay(m_outputBuffer, m_nextIterationNumber);

            fillRenderStatistics();
            m_nextIterationNumber++;
			//m_application.setRunningStatus(RunningStatus::PAUSE);
        }
    }
    catch(const std::exception & E)
    {
        m_application.setRunningStatus(RunningStatus::PAUSE);
        QString error = QString("%1").arg(E.what());
        emit renderManagerError(error);
    }
}

/*
unsigned long long StandaloneRenderManager::getIterationNumber() const
{
    return m_nextIterationNumber-1;
}*/

void StandaloneRenderManager::fillRenderStatistics()
{
    m_application.getRenderStatisticsModel().setNumIterations(m_nextIterationNumber);
    m_application.getRenderStatisticsModel().setCurrentPPMRadius(m_PPMRadius);

    if(m_application.getRenderMethod() == RenderMethod::PROGRESSIVE_PHOTON_MAPPING)
    {
        m_application.getRenderStatisticsModel().setNumEmittedPhotonsPerIteration(PPMOptixRenderer::EMITTED_PHOTONS_PER_ITERATION);
        m_application.getRenderStatisticsModel().setNumEmittedPhotons(PPMOptixRenderer::EMITTED_PHOTONS_PER_ITERATION*m_nextIterationNumber);
    }
    else
    {
        m_application.getRenderStatisticsModel().setNumEmittedPhotonsPerIteration(0);
        m_application.getRenderStatisticsModel().setNumEmittedPhotons(0);

    }

}

// TODO this may be called very often for rapid camera changes.
void StandaloneRenderManager::onSequenceNumberIncremented()
{
    m_nextIterationNumber = 0;
    m_PPMRadius = m_application.getPPMSettingsModel().getPPMInitialRadius();
    m_camera = m_application.getCamera();
    continueRayTracingIfRunningAsync();
}

void StandaloneRenderManager::onSceneLoadingNew()
{
    //m_currentScene = NULL;
}

void StandaloneRenderManager::onSceneUpdated()
{
    Scene* scene = m_application.getSceneManager().getScene();
    if(scene != m_currentScene)
    {
        m_compileScene = true;
        m_currentScene = scene;
    }
}

void StandaloneRenderManager::wait()
{
    printf("StandaloneRenderManager::wait\n");
    /*m_thread->exit();
    m_thread->wait();*/
}

void StandaloneRenderManager::continueRayTracingIfRunningAsync()
{
    if(m_application.getRunningStatus() == RunningStatus::RUNNING && m_noEmittedSignals)
    {
        m_noEmittedSignals = false;
        emit continueRayTracing();
    }
}

void StandaloneRenderManager::onRunningStatusChanged()
{
    continueRayTracingIfRunningAsync();
}

template <class T> class MinMaxAvg
{
public:
	T min, max, avg;
	
	static MinMaxAvg<T> forVector(std::vector<T> col)
	{
		MinMaxAvg<T> res;
		if(col.empty())
			throw std::range_error("Col is empty");
		res.min = res.max = col.at(0);
		res.avg = 0;
		for(unsigned int i = 0; i < col.size(); ++i)
		{
			res.min = std::min(res.min, col.at(i));
			res.max = std::max(res.max, col.at(i));
			res.avg += col.at(i);
		}

		res.avg /= col.size();

		return res;
	}
};

void StandaloneRenderManager::calculateOptimalNumEmmitedPhotons()
{
	static const unsigned int photonWidthSizes[] = { 32, 64, 128, 256, 512, 1024, 2048 };
	static const unsigned int numPhotonWidthSizes = sizeof(photonWidthSizes) / sizeof(photonWidthSizes[0]);
	static const unsigned int nPasses = 30;
	static const float acceptableAvgError = 0.05f;

	PMOptixRenderer *renderer = dynamic_cast<PMOptixRenderer *>(m_renderer);

	if(renderer == NULL) // not a PMOptixRenderer reference
	{
		return;
	}

	m_logger.log("Calculating optimal photons emmited per iteration\n");

	const int nMeshes = renderer->getHitCount().size();

	for(int i = 0; i < nMeshes; ++i)
	{
		m_logger.log("%d -> %s ", i, renderer->idToObjectName(i).c_str()); 
	}
	m_logger.log("\n");

	int optimalPhotonSize = photonWidthSizes[numPhotonWidthSizes-1];

	for(unsigned int i = 0; i < numPhotonWidthSizes; ++i)
	{
		std::vector<std::vector<unsigned int>> hitCounts(nPasses, std::vector<unsigned int>(nMeshes, 0));
		std::vector<float> averages(nMeshes, 0);

		float totalHits = 0.0f;

		// gather hit count and calculate average hits per mesh
		for(unsigned int pass = 0; pass < nPasses; ++pass)
		{
			renderer->genPhotonMap(photonWidthSizes[i]);
			hitCounts[pass] = renderer->getHitCount();
			for(int mesh = 0; mesh < nMeshes; ++mesh)
			{
				averages.at(mesh) += hitCounts.at(pass).at(mesh) / (float)nPasses;
			}
		}

		// calculate average absolute error (average - observed hit count)
		std::vector<float> averageError(nMeshes, 0);
		for(unsigned int pass = 0; pass < nPasses; ++pass)
		{
			for(int mesh = 0; mesh < nMeshes; ++mesh)
			{
				if(averages.at(mesh) != 0.0f)
					averageError.at(mesh) += fabsf(hitCounts.at(pass).at(mesh) - averages.at(mesh)) / (averages.at(mesh) * nPasses);
			}
		}

		if(averageError.empty())
		{
			break;
		}

		auto minMaxAvg = MinMaxAvg<float>::forVector(averageError);

		if(minMaxAvg.avg <= acceptableAvgError && photonWidthSizes[i] < optimalPhotonSize)
		{
			optimalPhotonSize = photonWidthSizes[i];
		}

		m_logger.log("Results for photonWidth = %d: min %0.1f%%, max %0.1f%%, avg %0.1f%%\n",
			photonWidthSizes[i], minMaxAvg.min * 100.0f, minMaxAvg.max * 100.0f, minMaxAvg.avg * 100.0f);
				
		m_logger.log("Average hit count(error%%): ");
		for(int mesh = 0; mesh < nMeshes; ++mesh)
		{
			m_logger.log("%0.0f(%0.1f%%) ", averages.at(mesh), averageError.at(mesh) * 100.f);
		}
		m_logger.log("\n");
	}

	m_logger.log("Optimal photon width size for average error of %0.0f%%: %d\n", acceptableAvgError * 100.0f, optimalPhotonSize);
}