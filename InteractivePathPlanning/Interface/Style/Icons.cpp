#pragma region Includes

#include "Interface/Style/Icons.h"

#include "Interface/Style/EmbeddedFiles.h"

#include <wx/bmpbndl.h>
#include <wx/mstream.h>

#include <cstring>
#include <string>

#pragma endregion

namespace
{
	// Colour the svg files are drawn in
	const std::string FILE_COLOUR = "#000000";

	// Embedded file called _name, or nullptr if there isn't one
	const EmbeddedFile* FindEmbeddedFile(const char* _name)
	{
		for (int i = 0; i < EMBEDDED_FILE_COUNT; ++i)
		{
			if (std::strcmp(EMBEDDED_FILES[i].name, _name) == 0)
			{
				return &EMBEDDED_FILES[i];
			}
		}

		return nullptr;
	}
}

wxBitmap Icons::Render(const char* _name, const wxColour& _colour, int _sizePx)
{
	const EmbeddedFile* file = FindEmbeddedFile(_name);

	if (!file)
	{
		wxFAIL_MSG(wxString("No embedded icon called ") + _name);
		return wxBitmap(_sizePx, _sizePx);
	}

	std::string svg(reinterpret_cast<const char*>(file->data), file->size);

	// swap black for the wanted colour
	const std::string colour = _colour.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();

	for (size_t at = svg.find(FILE_COLOUR); at != std::string::npos; at = svg.find(FILE_COLOUR, at + colour.size()))
	{
		svg.replace(at, FILE_COLOUR.size(), colour);
	}

	const wxBitmapBundle bundle = wxBitmapBundle::FromSVG(svg.c_str(), wxSize(_sizePx, _sizePx));
	return bundle.GetBitmap(wxSize(_sizePx, _sizePx));
}

wxIconBundle Icons::AppIcon()
{
	const EmbeddedFile* file = FindEmbeddedFile(APP);

	if (!file)
	{
		return wxIconBundle();
	}

	// every size in the .ico becomes one icon in the bundle
	wxMemoryInputStream stream(file->data, file->size);
	return wxIconBundle(stream, wxBITMAP_TYPE_ICO);
}
