#pragma once

// Defined as IMGUI_USER_CONFIG in Libraries.cmake

#if !GBC_WITH_UI
#	error "Including UI config header, but GBC_WITH_UI is not set!"
#endif // !GBC_WITH_UI

#include "Core/Assert.h"

//---- Define assertion handler. Defaults to calling assert().
#define IM_ASSERT(_EXPR) ASSERT(_EXPR)

//---- Don't define obsolete functions/enums names. Consider enabling from time to time after updating to avoid using soon-to-be obsolete function/names.
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
#if !GBC_DEBUG
#  define IMGUI_DISABLE_DEMO_WINDOWS
#  define IMGUI_DISABLE_METRICS_WINDOW
#endif // !GBC_DEBUG

// Avoid unnecessary reinterpret_cast calls
#define ImTextureID unsigned int
