#ifndef PCH_HPP
#define PCH_HPP

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <synchapi.h>

#elif defined( __unix__ )
	#include <X11/Xlib.h>
	#include <GL/gl.h>
	#include <GL/glx.h>
	#include <sys/time.h>
    
#elif defined( __APPLE__ )

#endif


#endif