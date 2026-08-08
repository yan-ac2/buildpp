#ifndef PCH_HPP
#define PCH_HPP

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#ifndef NOMINMAX
#define NOMINMAX 
#endif

    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <synchapi.h>

#elif __unix__ 
	#include <X11/Xlib.h>
	#include <GL/gl.h>
	#include <GL/glx.h>
	#include <sys/time.h>
    
#elif __APPLE__ 

#endif

#include "glHeader.hpp"

#endif