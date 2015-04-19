// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// ASIO
#define WINDOWS 1
#define ASIO_LITTLE_ENDIAN 1
#define ASIO_CPU_X86 1

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <timeapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <strsafe.h>

#include <math.h>

#include <thread>
#include <mutex>
#include <queue>
#include <vector>

// TODO: reference additional headers your program requires here

#include "dbgtrace.h"