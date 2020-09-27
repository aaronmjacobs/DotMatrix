#pragma once

#if GBC_WITH_UI
class UI;
#  define DECLARE_UI_FRIEND friend UI;
#else
#  define DECLARE_UI_FRIEND
#endif
