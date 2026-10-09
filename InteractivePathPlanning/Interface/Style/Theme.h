#pragma once

#include <wx/colour.h>

// App colours for the toolbar, panels, text and canvas
namespace Theme
{
	// Accent
	inline const wxColour ACCENT{ 0x25, 0x63, 0xEB };
	inline const wxColour ACCENT_HOVER{ 0x1D, 0x4E, 0xD8 };
	inline const wxColour ACCENT_DISABLED{ 0xA9, 0xC1, 0xF5 };
	// Hovered button fill
	inline const wxColour ACCENT_SOFT{ 0xDB, 0xEA, 0xFE };

	// Text
	inline const wxColour TEXT{ 0x0F, 0x17, 0x2A };
	inline const wxColour TEXT_MUTED{ 0x47, 0x55, 0x69 };
	inline const wxColour TEXT_DISABLED{ 0xA8, 0xB3, 0xC2 };

	// Surfaces
	inline const wxColour BORDER{ 0xE2, 0xE8, 0xF0 };
	inline const wxColour SURFACE{ 0xFF, 0xFF, 0xFF };
	inline const wxColour SURFACE_ALT{ 0xF1, 0xF5, 0xF9 };
	inline const wxColour SURFACE_HOVER{ 0xE2, 0xE8, 0xF0 };
	inline const wxColour PANEL_BACKGROUND{ 0xF8, 0xFA, 0xFC };

	// Grid
	inline const wxColour CANVAS_BACKGROUND{ 0xEC, 0xF0, 0xF5 };
	inline const wxColour GUTTER{ 0xD5, 0xDC, 0xE5 };
	inline const wxColour EMPTY_CELL{ 0xFF, 0xFF, 0xFF };
}

// Grid colours, editable in the Colors card
struct Palette
{
	wxColour start{ 0x16, 0xA3, 0x4A };
	wxColour end{ 0xDC, 0x26, 0x26 };
	wxColour path{ 0xDB, 0x27, 0x77 };
	wxColour frontier{ 0x7D, 0xD3, 0xFC };
	wxColour visited{ 0xC4, 0xB5, 0xFD };
	wxColour wall{ 0x1E, 0x29, 0x3B };
	wxColour highCost{ 0xB4, 0x53, 0x09 };
	// Hovered cell outline and cost brush
	wxColour highlight{ 0x25, 0x63, 0xEB };
	// Annotate pen
	wxColour drawing{ 0xF9, 0x73, 0x16 };
};
