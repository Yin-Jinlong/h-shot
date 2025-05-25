#pragma once

#define NOMINMAX

#include "type.h"

#include <iostream>
#include <vector>
#include <windows.h>
#include <windowsx.h>

#include "guard.h"

//

#include <include/core/SkBitmap.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkImage.h>
#include <include/core/SkPathEffect.h>
#include <include/core/SkRect.h>
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>

class State;
class View;

#if defined(UNICODE)
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

#define USE_RET [[nodiscard]]

tstring GetErrorMessage(DWORD code);

int ShowError(DWORD code = GetLastError());

int ShowErrorExit(int exitCode = -1, DWORD code = GetLastError());

int ShowError(const tstring& msg);

int ShowErrorExit(const tstring& msg, DWORD code = GetLastError());
