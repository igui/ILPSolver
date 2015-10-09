#pragma once

struct RendererStatistics {
	RendererStatistics():
		photonTracingTime(0),
		buildPhotonMapTime(0),
		resizeBufferTime(0),
		hitCountCalculationTime(0),
		transferDataTime(0),
		raytracePassTime(0),
		directRadiancePassTime(0),
		indirectRadiancePassTime(0),
		outputPassTime(0)
	{

	}

	double photonTracingTime;
	double buildPhotonMapTime;
	double resizeBufferTime;
	double hitCountCalculationTime;
	double transferDataTime;
	double raytracePassTime;
	double directRadiancePassTime;
	double indirectRadiancePassTime;
	double outputPassTime;
};