#pragma once

#include <wx/panel.h>

#include <map>
#include <string>
#include <vector>

class wxGraphicsContext;
class wxMenu;

// Custom drawn toolbar with a title, radio groups, buttons and dropdowns
// Buttons and radio items send wxEVT_BUTTON, dropdowns send wxEVT_CHOICE with the picked index
// Split buttons send their id for the main part and menuId for the chevron
class FlatToolBar : public wxPanel
{
private:
	enum class Kind
	{
		Radio,
		Button,
		PrimaryButton,
		// Button with a chevron that opens a menu
		MenuButton,
		// Button with a separate chevron part that opens a menu
		SplitButton,
		// Shows the picked choice from a list
		Dropdown,
		Separator,
		Stretch,
	};

	struct Item
	{
		int id = wxID_ANY;
		Kind kind = Kind::Button;
		wxString label;
		// Name from Icons.h
		const char* icon = nullptr;

		// Widest label the item will show, keeps the width fixed
		wxString reservedLabel;

		// Split buttons only, id sent by the chevron part
		int menuId = wxID_ANY;

		// Dropdowns only
		std::vector<wxString> choices;
		int selectedChoice = 0;

		bool enabled = true;
		bool checked = false;
		wxRect rect;
	};

	// Content
	std::vector<Item> items;
	wxString title;
	wxPoint titlePosition;
	// Hidden when there isn't room
	bool titleVisible = true;

	// Mouse state, OnSplit means over a split button's chevron
	int hoveredIndex = -1;
	bool hoveredOnSplit = false;
	int pressedIndex = -1;
	bool pressedOnSplit = false;

	// Icon cache, keyed by name, colour and size
	std::map<std::string, wxBitmap> iconCache;

#pragma region Layout

	// Adds an item and redoes the layout
	void AddItem(const Item& _item);
	// Positions items, anything after a Stretch goes on the right
	void LayoutItems();
	// How much room an item needs
	wxSize ItemSize(const Item& _item) const;
	// Gap after the item at _index
	int GapAfter(size_t _index) const;
	// Chevron part of a split button
	wxRect SplitRect(const Item& _item) const;

#pragma endregion

#pragma region Lookup

	// Clickable item under _point, or -1
	int HitTest(wxPoint _point) const;
	// True if _point is over a split button's chevron
	bool IsOverSplit(int _index, wxPoint _point) const;
	// Index of the item with _id, or -1
	int FindItem(int _id) const;
	bool IsClickable(const Item& _item) const;
	// Dropdowns, menu buttons and split buttons show a chevron
	bool HasChevron(const Item& _item) const;

#pragma endregion

#pragma region Painting

	// Icon and label colour
	wxColour ForegroundOf(size_t _index) const;
	// Chevron colour
	wxColour ChevronColourOf(size_t _index) const;
	// Gets a cached icon in a colour
	const wxBitmap& Icon(const char* _icon, const wxColour& _colour, int _sizeDip);

	// Drawn in this order
	void DrawRadioTracks(wxGraphicsContext& _gc) const;
	void DrawItemBackground(wxGraphicsContext& _gc, size_t _index) const;
	void DrawItemContent(wxDC& _dc, size_t _index);

#pragma endregion

	// Handles a click on an item or its chevron
	void ActivateItem(size_t _index, bool _onSplit);
	// Shows a dropdown's choices and sends the picked one
	void ShowDropdownMenu(size_t _index);

	// Events
	void OnPaint(wxPaintEvent& _event);
	void OnSize(wxSizeEvent& _event);
	void OnMotion(wxMouseEvent& _event);
	void OnLeave(wxMouseEvent& _event);
	void OnLeftDown(wxMouseEvent& _event);
	void OnLeftUp(wxMouseEvent& _event);
	void OnCaptureLost(wxMouseCaptureLostEvent& _event);
	void OnDpiChanged(wxDPIChangedEvent& _event);

public:
	explicit FlatToolBar(wxWindow* _parent);

	// Bold text on the far left
	void SetTitle(const wxString& _title);
	// Adds a radio item, items next to each other form a group and the first starts checked
	void AddRadio(int _id, const wxString& _label, const char* _icon);
	// Adds a button, an empty label makes it icon only
	void AddButton(int _id, const wxString& _label, const char* _icon, bool _primary = false);
	// Adds a button with a chevron, open its menu with PopupMenuBelow
	void AddMenuButton(int _id, const wxString& _label, const char* _icon);
	// Adds a button where the main part sends _id and the chevron sends _menuId
	void AddSplitButton(int _id, int _menuId, const wxString& _label, const char* _icon);
	// Adds a dropdown of _choices
	void AddDropdown(int _id, const char* _icon, const std::vector<wxString>& _choices);
	// Adds a divider line
	void AddSeparator();
	// Items added after this go on the right
	void AddStretch();

	// Checks a radio item and unchecks the rest of its group
	void SetChecked(int _id);
	// Enables or greys out an item
	void SetItemEnabled(int _id, bool _enabled);
	// Changes an item's label and icon
	void SetItemContent(int _id, const wxString& _label, const char* _icon);
	// Keeps an item wide enough for _label
	void ReserveLabel(int _id, const wxString& _label);
	// Sets a dropdown's choice
	void SetDropdownSelection(int _id, int _choice);
	// Shows _label on a dropdown with no choice picked, like "Random map"
	void SetDropdownLabel(int _id, const wxString& _label);

	// Shows _menu under an item, returns the picked id or wxID_NONE
	int PopupMenuBelow(int _id, wxMenu& _menu);
	// Narrowest width that fits every item, not counting the title
	int MinimumWidth() const;
};
