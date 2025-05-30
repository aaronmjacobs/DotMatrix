#pragma once

// Defined as IMGUI_USER_CONFIG in Libraries.cmake

#if !DM_WITH_UI
#	error "Including UI config header, but DM_WITH_UI is not set!"
#endif // !DM_WITH_UI

#include "Core/Assert.h"

//---- Define assertion handler. Defaults to calling DM_ASSERT().
#define IM_ASSERT(_EXPR) DM_ASSERT(_EXPR)

//---- Don't define obsolete functions/enums names. Consider enabling from time to time after updating to avoid using soon-to-be obsolete function/names.
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
#if !DM_WITH_DEBUG_UTILS
#  define IMGUI_DISABLE_DEMO_WINDOWS
#  define IMGUI_DISABLE_DEBUG_TOOLS
#endif // !DM_WITH_DEBUG_UTILS

// Avoid unnecessary reinterpret_cast calls
#define ImTextureID unsigned int
