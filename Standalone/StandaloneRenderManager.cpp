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
#include <algorithm>

StandaloneRenderManager::StandaloneRenderManager(QApplication & qApplication, Application & application, const ComputeDevice& device) :
    m_device(device),
    m_renderer(NULL), 
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
	try {
		//m_renderer->initialize(m_device, &m_logger);
	} catch(std::exception& ex)
	{
		emit renderManagerError(ex.what());
	}
}

void StandaloneRenderManager::reinitRenderer(OptixRenderer *newRenderer)
{
	if(m_renderer)
	{
		delete m_renderer;
	}
	m_renderer = newRenderer;
	m_application.setRendererStatus(RendererStatus::INITIALIZING_ENGINE);
	m_renderer->initialize(m_device, &m_logger);
	m_compileScene = true;
}

void StandaloneRenderManager::onContinueRayTracing()
{
    renderNextIteration();
    continueRayTracingIfRunningAsync();
}

void StandaloneRenderManager::logHitCount()
{
	auto pmOptixRenderer = dynamic_cast<PMOptixRenderer *>(m_renderer);
	if(!pmOptixRenderer)
		return;
	
	auto objectIdToName = pmOptixRenderer->objectToNameMapping();
	auto radiances = pmOptixRenderer->getRadiance();
	auto hitCounts = pmOptixRenderer->getHitCount();
	m_logger.log("Hits \n");
	
	int sumHits = 0;
	for(int i = 0; i < hitCounts.size(); ++i)
	{
		auto hitCount = hitCounts.at(i);
		auto radiance = radiances.at(i) / m_currentScene->getObjectArea(i);
		auto average = m_hitCumulativeSum.at(i) / m_nextIterationNumber;
		auto stddev = sqrtf(m_hitCumulativeSum.at(i) / (m_nextIterationNumber+1));

		m_logger.log("\t%20s: %10d\tAvg: %10f\tStddev: %10f\tRadiance: %10f\n",
			objectIdToName.at(i).c_str(),
			hitCount,
			average,
			stddev,
			radiance
		);
		sumHits += hitCount;
	}

	m_logger.log("Hit ratio: %0.2f\n", (float)sumHits / pmOptixRenderer->totalPhotons());
	m_logger.log("Emitted power: %0.2f\n", pmOptixRenderer->getEmittedPower());

}

void StandaloneRenderManager::calculateAvgStdDevHits()
{
	auto pmOptixRenderer = dynamic_cast<PMOptixRenderer *>(m_renderer);
	if (!pmOptixRenderer)
		return;

	auto hitCounts = pmOptixRenderer->getHitCount();

	// https://math.stackexchange.com/questions/374881/recursive-formula-for-variance
	if (m_nextIterationNumber == 1)
	{
		// M_1 = x_1
		m_hitCumulativeSum.resize(hitCounts.size());
		std::copy(hitCounts.cbegin(), hitCounts.cend(), m_hitCumulativeSum.begin());
		// S_1 = 0
		m_hitCumulativeErrorSquared.resize(hitCounts.size(), 0.0f);
	}
	else
	{
		for (int i = 0; i < hitCounts.size(); ++i)
		{
			m_hitCumulativeSum[i] += hitCounts[i];
		}
		for (int i = 0; i < hitCounts.size(); ++i)
		{
			m_hitCumulativeErrorSquared[i] +=
				pow(m_nextIterationNumber*hitCounts[i] - m_hitCumulativeSum[i], 2) /
				m_nextIterationNumber * (m_nextIterationNumber - 1);
		}
	}
}

void StandaloneRenderManager::renderNextIteration()
{
    try
    {
        if(m_application.getRunningStatus() == RunningStatus::RUNNING && m_currentScene != NULL)
        {
            m_noEmittedSignals = true;

			if(m_application.getRenderMethod() == RenderMethod::PHOTON_MAPPING && !dynamic_cast<PMOptixRenderer *>(m_renderer))
			{
				reinitRenderer(new PMOptixRenderer());
			}
			else if(m_application.getRenderMethod() != RenderMethod::PHOTON_MAPPING && !dynamic_cast<PPMOptixRenderer *>(m_renderer))
			{
				reinitRenderer(new PPMOptixRenderer());
			}

            if(m_compileScene)
            {
                m_application.setRendererStatus(RendererStatus::INITIALIZING_SCENE);
                m_renderer->initScene(*m_currentScene);
                m_compileScene = false;
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

			calculateAvgStdDevHits();
			logHitCount();
        }
    }
    catch(const std::exception & E)
    {
        m_application.setRunningStatus(RunningStatus::PAUSE);
        QString error = QString("%1").arg(E.what());
        emit renderManagerError(error);
    }
}

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

