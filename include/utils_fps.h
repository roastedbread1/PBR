#pragma once

#include <assert.h>
#include <stdio.h>

struct FramePerSecondCounter
{
	float avgInterval = 0.5f;
	unsigned int numFrames = 0;
	double accumulatedTime = 0;
	float currentFPS = 0.0f;
	bool printFPS = true;
};




inline void init_fps(FramePerSecondCounter* fps, float avgInterval=0.5)
{
	assert(avgInterval > 0.0f);
	fps->avgInterval = avgInterval;
}

inline bool tick_fps(FramePerSecondCounter* fps, float deltaSeconds, bool frameRendered = true)
{
	if (frameRendered) { fps->numFrames++; }

	fps->accumulatedTime += deltaSeconds;

	if (fps->accumulatedTime > fps->avgInterval)
	{
		fps->currentFPS = static_cast<float>(fps->numFrames / fps->accumulatedTime);

		if (fps->printFPS)
		{
			printf("FPS: %.1f\n", fps->currentFPS);
		}
		fps->numFrames = 0;
		fps->accumulatedTime = 0;
		return true;
	}
	return false;
}

inline float get_fps(FramePerSecondCounter* fps)
{
	return fps->currentFPS;
}