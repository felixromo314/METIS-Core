#pragma once

#ifndef MEMORY_LEAKS_H
#define MEMORY_LEAKS_H

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#define MY_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )   // for debugging memory leaks
#else
#define MY_NEW new
#endif



#endif // DEBUG_MEMORY_H