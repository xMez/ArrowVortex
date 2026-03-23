#pragma once

#ifdef _WIN32
#include <windows.h>
#include <gl/gl.h>
#undef max
#undef min
#undef ERROR
#else
#ifdef __linux__
#include "SDL3/SDL_opengl.h"
#include "SDL3/SDL_opengl_glext.h"
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#endif
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
