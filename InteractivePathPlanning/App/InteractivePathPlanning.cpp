#pragma region Includes

#include "App/MainFrame.h"

#include <wx/app.h>
#include <wx/image.h>
#include <wx/imagbmp.h>
#include <wx/imagpng.h>

#pragma endregion

class InteractivePathPlanningApp : public wxApp
{
public:
	bool OnInit() override
	{
		if (!wxApp::OnInit())
		{
			return false;
		}

		SetAppDisplayName("Interactive Path Planning");

		// needed to save PNGs and load the window icon
		wxImage::AddHandler(new wxPNGHandler());
		wxImage::AddHandler(new wxICOHandler());

		MainFrame* frame = new MainFrame();
		frame->Show();
		return true;
	}
};

wxIMPLEMENT_APP(InteractivePathPlanningApp);
