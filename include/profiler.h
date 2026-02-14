#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#include <cstring> 


#define PROFILER_COLOR_WAIT     0xff0000 // blue 
#define PROFILER_COLOR_SUBMIT   0x0000ff // red
#define PROFILER_COLOR_PRESENT  0x00ff00 // green
#define PROFILER_COLOR_CREATE   0xff6600 // dark blue
#define PROFILER_COLOR_DESTROY  0xffa500 
#define PROFILER_COLOR_BARRIER  0xffffff 
#define PROFILER_FUNCTION() ZoneScoped
#define PROFILER_FUNCTION_COLOR(color) ZoneScopedC(color)
#define PROFILER_ZONE(name, color) { \
	ZoneScopedC(color); \
	ZoneName(name, strlen(name))
#define PROFILER_ZONE_END() }
#define PROFILER_FRAME(name) FrameMarkNamed(name)
#define PROFILER_THREAD(name) tracy::SetThreadName(name)

#else 

#define PROFILER_FUNCTION()
#define PROFILER_FUNCTION_COLOR(color)
#define PROFILER_ZONE(name, color) {
#define PROFILER_ZONE_END() }
#define PROFILER_THREAD(name)
#define PROFILER_FRAME(name)

#endif // TRACY_ENABLE