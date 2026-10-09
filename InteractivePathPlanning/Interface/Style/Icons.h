#pragma once

#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/iconbndl.h>

// Icon names from the embedded files list in CMakeLists.txt
// Each icon is a black (#000000) .svg in Resources/Icons that gets packed into the app
// To change an icon, replace its .svg and rebuild
namespace Icons
{
	// Tools
	inline constexpr const char* TILE_STATE = "ICON_TILE_STATE";
	inline constexpr const char* COST_PAINTER = "ICON_COST_PAINTER";
	inline constexpr const char* ANNOTATE = "ICON_ANNOTATE";

	// Menus
	inline constexpr const char* MAP = "ICON_MAP";
	inline constexpr const char* VIEW = "ICON_VIEW";
	inline constexpr const char* CHEVRON_DOWN = "ICON_CHEVRON_DOWN";
	inline constexpr const char* CHEVRON_LEFT = "ICON_CHEVRON_LEFT";

	// Search
	inline constexpr const char* FIND_PATH = "ICON_FIND_PATH";
	inline constexpr const char* PLAY = "ICON_PLAY";
	inline constexpr const char* PAUSE = "ICON_PAUSE";
	inline constexpr const char* STEP = "ICON_STEP";
	inline constexpr const char* CLEAR = "ICON_CLEAR";

	// App
	inline constexpr const char* RESET = "ICON_RESET";
	inline constexpr const char* HELP = "ICON_HELP";
	inline constexpr const char* APP = "APP_ICON";

	// Same triangle as Play
	inline constexpr const char* CHEVRON_RIGHT = PLAY;

	// Loads an icon as a square bitmap in _colour
	wxBitmap Render(const char* _name, const wxColour& _colour, int _sizePx);
	// Window icon from Resources/app.ico
	wxIconBundle AppIcon();
}
