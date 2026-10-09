#pragma region Includes

#include "Interface/Widgets/FlatToolBar.h"

#include "Interface/Style/Icons.h"
#include "Interface/Style/Theme.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/menu.h>

#include <algorithm>
#include <memory>

#pragma endregion

namespace
{
	// Sizes in DIPs
	constexpr int BAR_HEIGHT = 56;
	constexpr int ITEM_HEIGHT = 34;
	constexpr int ICON_SIZE = 18;
	constexpr int CHEVRON_SIZE = 14;
	constexpr int SPLIT_WIDTH = 28;
	constexpr int ITEM_PADDING_X = 12;
	constexpr int ICON_TEXT_GAP = 7;
	constexpr int EDGE_PADDING = 16;
	constexpr int TITLE_GAP = 28;
	constexpr int SEPARATOR_WIDTH = 17;
	constexpr int CORNER_RADIUS = 8;
	constexpr int TRACK_INSET = 3;
	constexpr int TRACK_RADIUS = 10;

	// Gaps between items in DIPs
	constexpr int GAP_INSIDE_RADIO_GROUP = 2;
	constexpr int GAP_AROUND_RADIO_GROUP = 14;
	constexpr int GAP_BETWEEN_BUTTONS = 8;

	// Menu id for the first dropdown choice, the rest follow
	constexpr int FIRST_CHOICE_MENU_ID = 1000;

	wxFont LabelFont(const wxWindow* _window)
	{
		wxFont font = _window->GetFont();
		font.SetWeight(wxFONTWEIGHT_MEDIUM);
		return font;
	}

	wxFont TitleFont(const wxWindow* _window)
	{
		return _window->GetFont().Bold().Scaled(1.25f);
	}

	int TextWidth(const wxWindow* _window, const wxString& _text, const wxFont& _font)
	{
		int width = 0;
		int height = 0;
		_window->GetTextExtent(_text, &width, &height, nullptr, nullptr, &_font);
		return width;
	}
}

FlatToolBar::FlatToolBar(wxWindow* _parent) : wxPanel(_parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(-1, FromDIP(BAR_HEIGHT)));

	Bind(wxEVT_PAINT, &FlatToolBar::OnPaint, this);
	Bind(wxEVT_SIZE, &FlatToolBar::OnSize, this);
	Bind(wxEVT_MOTION, &FlatToolBar::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &FlatToolBar::OnLeave, this);
	Bind(wxEVT_LEFT_DOWN, &FlatToolBar::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &FlatToolBar::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &FlatToolBar::OnLeftUp, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &FlatToolBar::OnCaptureLost, this);
	Bind(wxEVT_DPI_CHANGED, &FlatToolBar::OnDpiChanged, this);
}

#pragma region Building

void FlatToolBar::SetTitle(const wxString& _title)
{
	title = _title;
	LayoutItems();
	Refresh();
}

void FlatToolBar::AddRadio(int _id, const wxString& _label, const char* _icon)
{
	// first radio item in a group starts checked
	const bool startsGroup = items.empty() || items.back().kind != Kind::Radio;
	AddItem({ .id = _id, .kind = Kind::Radio, .label = _label, .icon = _icon, .checked = startsGroup });
}

void FlatToolBar::AddButton(int _id, const wxString& _label, const char* _icon, bool _primary)
{
	const Kind kind = _primary ? Kind::PrimaryButton : Kind::Button;
	AddItem({ .id = _id, .kind = kind, .label = _label, .icon = _icon });
}

void FlatToolBar::AddMenuButton(int _id, const wxString& _label, const char* _icon)
{
	AddItem({ .id = _id, .kind = Kind::MenuButton, .label = _label, .icon = _icon });
}

void FlatToolBar::AddSplitButton(int _id, int _menuId, const wxString& _label, const char* _icon)
{
	AddItem({ .id = _id, .kind = Kind::SplitButton, .label = _label, .icon = _icon, .menuId = _menuId });
}

void FlatToolBar::AddDropdown(int _id, const char* _icon, const std::vector<wxString>& _choices)
{
	const wxString firstChoice = _choices.empty() ? wxString() : _choices.front();
	Item item{ .id = _id, .kind = Kind::Dropdown, .label = firstChoice, .icon = _icon, .choices = _choices };

	// size it for the longest choice
	const wxFont font = LabelFont(this);

	for (const wxString& choice : _choices)
	{
		if (TextWidth(this, choice, font) > TextWidth(this, item.reservedLabel, font))
		{
			item.reservedLabel = choice;
		}
	}

	AddItem(item);
}

void FlatToolBar::AddSeparator()
{
	AddItem({ .kind = Kind::Separator });
}

void FlatToolBar::AddStretch()
{
	AddItem({ .kind = Kind::Stretch });
}

void FlatToolBar::AddItem(const Item& _item)
{
	items.push_back(_item);
	LayoutItems();
	Refresh();
}

#pragma endregion

#pragma region Changing Items

void FlatToolBar::SetChecked(int _id)
{
	const int index = FindItem(_id);

	if (index == -1 || items[index].kind != Kind::Radio)
	{
		return;
	}

	// find the start and end of this radio group
	size_t first = index;
	size_t last = index;

	while (first > 0 && items[first - 1].kind == Kind::Radio)
	{
		--first;
	}

	while (last + 1 < items.size() && items[last + 1].kind == Kind::Radio)
	{
		++last;
	}

	for (size_t i = first; i <= last; ++i)
	{
		items[i].checked = static_cast<int>(i) == index;
	}

	Refresh();
}

void FlatToolBar::SetItemEnabled(int _id, bool _enabled)
{
	const int index = FindItem(_id);

	if (index != -1 && items[index].enabled != _enabled)
	{
		items[index].enabled = _enabled;
		Refresh();
	}
}

void FlatToolBar::SetItemContent(int _id, const wxString& _label, const char* _icon)
{
	const int index = FindItem(_id);

	// called every animation frame, so skip if nothing changed
	if (index == -1 || (items[index].label == _label && items[index].icon == _icon))
	{
		return;
	}

	items[index].label = _label;
	items[index].icon = _icon;
	LayoutItems();
	Refresh();
}

void FlatToolBar::ReserveLabel(int _id, const wxString& _label)
{
	const int index = FindItem(_id);

	if (index == -1)
	{
		return;
	}

	const wxFont font = LabelFont(this);

	if (TextWidth(this, _label, font) > TextWidth(this, items[index].reservedLabel, font))
	{
		items[index].reservedLabel = _label;
		LayoutItems();
		Refresh();
	}
}

void FlatToolBar::SetDropdownSelection(int _id, int _choice)
{
	const int index = FindItem(_id);

	if (index == -1 || items[index].kind != Kind::Dropdown)
	{
		return;
	}

	Item& item = items[index];

	if (_choice < 0 || _choice >= static_cast<int>(item.choices.size()))
	{
		return;
	}

	item.selectedChoice = _choice;
	item.label = item.choices[_choice];
	Refresh();
}

void FlatToolBar::SetDropdownLabel(int _id, const wxString& _label)
{
	const int index = FindItem(_id);

	if (index == -1 || items[index].kind != Kind::Dropdown)
	{
		return;
	}

	// no choice checked while a custom label shows
	items[index].selectedChoice = -1;
	items[index].label = _label;
	ReserveLabel(_id, _label);
	Refresh();
}

#pragma endregion

#pragma region Layout

wxSize FlatToolBar::ItemSize(const Item& _item) const
{
	const int height = FromDIP(ITEM_HEIGHT);

	switch (_item.kind)
	{
	case Kind::Separator:
		return wxSize(FromDIP(SEPARATOR_WIDTH), height);
	case Kind::Stretch:
		return wxSize(0, height);
	default:
		break;
	}

	// icon-only buttons are square
	if (_item.label.empty())
	{
		return wxSize(height, height);
	}

	const wxFont font = LabelFont(this);
	const int textWidth = std::max(TextWidth(this, _item.label, font), TextWidth(this, _item.reservedLabel, font));
	int width = FromDIP(ITEM_PADDING_X) * 2 + FromDIP(ICON_SIZE) + FromDIP(ICON_TEXT_GAP) + textWidth;

	if (_item.kind == Kind::SplitButton)
	{
		width += FromDIP(SPLIT_WIDTH);
	}
	else if (HasChevron(_item))
	{
		width += FromDIP(ICON_TEXT_GAP) + FromDIP(CHEVRON_SIZE);
	}

	return wxSize(width, height);
}

int FlatToolBar::GapAfter(size_t _index) const
{
	if (_index + 1 >= items.size())
	{
		return 0;
	}

	const bool thisIsRadio = items[_index].kind == Kind::Radio;
	const bool nextIsRadio = items[_index + 1].kind == Kind::Radio;

	if (thisIsRadio && nextIsRadio)
	{
		return FromDIP(GAP_INSIDE_RADIO_GROUP);
	}

	if (thisIsRadio || nextIsRadio)
	{
		return FromDIP(GAP_AROUND_RADIO_GROUP);
	}

	return FromDIP(GAP_BETWEEN_BUTTONS);
}

wxRect FlatToolBar::SplitRect(const Item& _item) const
{
	const int splitWidth = FromDIP(SPLIT_WIDTH);
	return wxRect(_item.rect.GetRight() + 1 - splitWidth, _item.rect.y, splitWidth, _item.rect.height);
}

int FlatToolBar::MinimumWidth() const
{
	int width = FromDIP(EDGE_PADDING) * 2;

	for (size_t i = 0; i < items.size(); ++i)
	{
		// stretch has no size
		if (items[i].kind != Kind::Stretch)
		{
			width += ItemSize(items[i]).x + GapAfter(i);
		}
	}

	return width;
}

void FlatToolBar::LayoutItems()
{
	const wxSize client = GetClientSize();
	const int top = (client.y - FromDIP(ITEM_HEIGHT)) / 2;
	int x = FromDIP(EDGE_PADDING);

	// only show the title if everything fits
	const wxFont titleFont = TitleFont(this);
	int titleWidth = 0;
	int titleHeight = 0;
	GetTextExtent(title, &titleWidth, &titleHeight, nullptr, nullptr, &titleFont);
	titleVisible = !title.empty() && MinimumWidth() + titleWidth + FromDIP(TITLE_GAP) <= client.x;

	if (titleVisible)
	{
		titlePosition = wxPoint(x, (client.y - titleHeight) / 2);
		x += titleWidth + FromDIP(TITLE_GAP);
	}

	// items before the stretch go from the left
	size_t stretchIndex = items.size();

	for (size_t i = 0; i < items.size(); ++i)
	{
		if (items[i].kind == Kind::Stretch)
		{
			stretchIndex = i;
			break;
		}
	}

	for (size_t i = 0; i < stretchIndex; ++i)
	{
		const wxSize size = ItemSize(items[i]);
		items[i].rect = wxRect(x, top, size.x, size.y);
		x += size.x + GapAfter(i);
	}

	if (stretchIndex == items.size())
	{
		return;
	}

	// items after it line up to the right edge
	int rightWidth = 0;

	for (size_t i = stretchIndex + 1; i < items.size(); ++i)
	{
		rightWidth += ItemSize(items[i]).x + GapAfter(i);
	}

	x = std::max(x, client.x - FromDIP(EDGE_PADDING) - rightWidth);

	for (size_t i = stretchIndex + 1; i < items.size(); ++i)
	{
		const wxSize size = ItemSize(items[i]);
		items[i].rect = wxRect(x, top, size.x, size.y);
		x += size.x + GapAfter(i);
	}
}

#pragma endregion

#pragma region Lookup

int FlatToolBar::HitTest(wxPoint _point) const
{
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (IsClickable(items[i]) && items[i].rect.Contains(_point))
		{
			return static_cast<int>(i);
		}
	}

	return -1;
}

bool FlatToolBar::IsOverSplit(int _index, wxPoint _point) const
{
	return _index != -1 && items[_index].kind == Kind::SplitButton && SplitRect(items[_index]).Contains(_point);
}

int FlatToolBar::FindItem(int _id) const
{
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (items[i].id == _id && IsClickable(items[i]))
		{
			return static_cast<int>(i);
		}
	}

	return -1;
}

bool FlatToolBar::IsClickable(const Item& _item) const
{
	return _item.kind != Kind::Separator && _item.kind != Kind::Stretch;
}

bool FlatToolBar::HasChevron(const Item& _item) const
{
	return _item.kind == Kind::Dropdown || _item.kind == Kind::MenuButton || _item.kind == Kind::SplitButton;
}

#pragma endregion

#pragma region Painting

wxColour FlatToolBar::ForegroundOf(size_t _index) const
{
	const Item& item = items[_index];
	const bool isHovered = static_cast<int>(_index) == hoveredIndex;

	if (item.kind == Kind::PrimaryButton)
	{
		return *wxWHITE;
	}

	if (!item.enabled)
	{
		return Theme::TEXT_DISABLED;
	}

	if (item.kind == Kind::Radio)
	{
		if (item.checked)
		{
			return Theme::ACCENT;
		}

		return isHovered ? Theme::TEXT : Theme::TEXT_MUTED;
	}

	// accent on hover, split buttons only when over the main part
	const bool mainPartHovered = isHovered && !(item.kind == Kind::SplitButton && hoveredOnSplit);
	return mainPartHovered ? Theme::ACCENT : Theme::TEXT;
}

wxColour FlatToolBar::ChevronColourOf(size_t _index) const
{
	const Item& item = items[_index];
	const bool isHovered = static_cast<int>(_index) == hoveredIndex && item.enabled;

	// split button chevrons only light up when hovered directly
	const bool chevronHovered = isHovered && (item.kind != Kind::SplitButton || hoveredOnSplit);
	return chevronHovered ? Theme::ACCENT : Theme::TEXT_MUTED;
}

const wxBitmap& FlatToolBar::Icon(const char* _icon, const wxColour& _colour, int _sizeDip)
{
	const int sizePx = FromDIP(_sizeDip);
	const std::string colour = _colour.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
	const std::string key = std::string(_icon) + colour + std::to_string(sizePx);

	const std::map<std::string, wxBitmap>::const_iterator cached = iconCache.find(key);

	if (cached != iconCache.end())
	{
		return cached->second;
	}

	return iconCache.emplace(key, Icons::Render(_icon, _colour, sizePx)).first->second;
}

void FlatToolBar::DrawRadioTracks(wxGraphicsContext& _gc) const
{
	_gc.SetPen(*wxTRANSPARENT_PEN);
	_gc.SetBrush(wxBrush(Theme::SURFACE_ALT));

	for (size_t i = 0; i < items.size(); ++i)
	{
		const bool startsGroup = items[i].kind == Kind::Radio && (i == 0 || items[i - 1].kind != Kind::Radio);

		if (!startsGroup)
		{
			continue;
		}

		wxRect track = items[i].rect;

		for (size_t j = i + 1; j < items.size() && items[j].kind == Kind::Radio; ++j)
		{
			track.Union(items[j].rect);
		}

		track.Inflate(FromDIP(TRACK_INSET));
		_gc.DrawRoundedRectangle(track.x, track.y, track.width, track.height, FromDIP(TRACK_RADIUS));
	}
}

void FlatToolBar::DrawItemBackground(wxGraphicsContext& _gc, size_t _index) const
{
	const Item& item = items[_index];
	const bool isHovered = static_cast<int>(_index) == hoveredIndex && item.enabled;
	const bool isPressed = isHovered && static_cast<int>(_index) == pressedIndex && pressedOnSplit == hoveredOnSplit;
	const wxRect& r = item.rect;
	const double radius = FromDIP(CORNER_RADIUS);

	switch (item.kind)
	{
	case Kind::Radio:
		if (item.checked)
		{
			// checked item is a raised pill on the track
			_gc.SetPen(wxPen(Theme::BORDER, 1));
			_gc.SetBrush(wxBrush(Theme::SURFACE));
			_gc.DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, radius);
		}
		else if (isHovered)
		{
			_gc.SetPen(*wxTRANSPARENT_PEN);
			_gc.SetBrush(wxBrush(Theme::SURFACE_HOVER));
			_gc.DrawRoundedRectangle(r.x, r.y, r.width, r.height, radius);
		}
		break;
	case Kind::PrimaryButton:
	{
		wxColour fill = isHovered ? Theme::ACCENT_HOVER : Theme::ACCENT;

		if (!item.enabled)
		{
			fill = Theme::ACCENT_DISABLED;
		}
		else if (isPressed)
		{
			fill = Theme::ACCENT_HOVER.ChangeLightness(85);
		}

		_gc.SetPen(*wxTRANSPARENT_PEN);
		_gc.SetBrush(wxBrush(fill));
		_gc.DrawRoundedRectangle(r.x, r.y, r.width, r.height, radius);
		break;
	}
	case Kind::Button:
	case Kind::MenuButton:
	case Kind::SplitButton:
	case Kind::Dropdown:
	{
		_gc.SetPen(*wxTRANSPARENT_PEN);
		_gc.SetBrush(wxBrush(Theme::SURFACE));
		_gc.DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, radius);

		// soft accent fill on hover, split buttons only fill the hovered part
		if (isHovered)
		{
			wxRect hoveredPart = r;

			if (item.kind == Kind::SplitButton)
			{
				const wxRect split = SplitRect(item);
				hoveredPart = hoveredOnSplit ? split : wxRect(r.x, r.y, split.x - r.x, r.height);
			}

			const wxColour hoverFill = isPressed ? Theme::ACCENT_SOFT.ChangeLightness(94) : Theme::ACCENT_SOFT;
			_gc.Clip(hoveredPart.x, hoveredPart.y, hoveredPart.width, hoveredPart.height);
			_gc.SetBrush(wxBrush(hoverFill));
			_gc.DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, radius);
			_gc.ResetClip();
		}

		const wxColour outline = isHovered ? Theme::ACCENT : Theme::BORDER;
		_gc.SetPen(wxPen(outline, 1));
		_gc.SetBrush(*wxTRANSPARENT_BRUSH);
		_gc.DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, radius);

		// line between the two parts of a split button
		if (item.kind == Kind::SplitButton)
		{
			const double lineX = SplitRect(item).x + 0.5;
			_gc.StrokeLine(lineX, r.y + 1, lineX, r.GetBottom());
		}
		break;
	}
	case Kind::Separator:
	{
		const double centerX = r.x + r.width / 2 + 0.5;
		_gc.SetPen(wxPen(Theme::BORDER, 1));
		_gc.StrokeLine(centerX, r.y + FromDIP(6), centerX, r.GetBottom() - FromDIP(6));
		break;
	}
	case Kind::Stretch:
		break;
	}
}

void FlatToolBar::DrawItemContent(wxDC& _dc, size_t _index)
{
	const Item& item = items[_index];

	if (!item.icon)
	{
		return;
	}

	const wxColour foreground = ForegroundOf(_index);
	const wxBitmap& icon = Icon(item.icon, foreground, ICON_SIZE);
	const int iconSize = icon.GetWidth();
	const int iconY = item.rect.y + (item.rect.height - iconSize) / 2;

	// split buttons centre their content left of the chevron
	wxRect content = item.rect;

	if (item.kind == Kind::SplitButton)
	{
		const wxRect split = SplitRect(item);
		content.width = split.x - item.rect.x;

		const wxBitmap& chevron = Icon(Icons::CHEVRON_DOWN, ChevronColourOf(_index), CHEVRON_SIZE);
		_dc.DrawBitmap(chevron, split.x + (split.width - chevron.GetWidth()) / 2, split.y + (split.height - chevron.GetHeight()) / 2, true);
	}

	if (item.label.empty())
	{
		_dc.DrawBitmap(icon, content.x + (content.width - iconSize) / 2, iconY, true);
		return;
	}

	const wxSize text = _dc.GetTextExtent(item.label);
	const int textY = item.rect.y + (item.rect.height - text.y) / 2;
	_dc.SetTextForeground(foreground);

	// dropdowns and menu buttons put the chevron on the right
	if (item.kind == Kind::Dropdown || item.kind == Kind::MenuButton)
	{
		const int iconX = item.rect.x + FromDIP(ITEM_PADDING_X);
		const wxBitmap& chevron = Icon(Icons::CHEVRON_DOWN, ChevronColourOf(_index), CHEVRON_SIZE);
		const int chevronX = item.rect.GetRight() - FromDIP(ITEM_PADDING_X) - chevron.GetWidth() + 1;

		_dc.DrawBitmap(icon, iconX, iconY, true);
		_dc.DrawText(item.label, iconX + iconSize + FromDIP(ICON_TEXT_GAP), textY);
		_dc.DrawBitmap(chevron, chevronX, item.rect.y + (item.rect.height - chevron.GetHeight()) / 2, true);
		return;
	}

	// centre the icon and label
	const int contentWidth = iconSize + FromDIP(ICON_TEXT_GAP) + text.x;
	const int iconX = content.x + (content.width - contentWidth) / 2;

	_dc.DrawBitmap(icon, iconX, iconY, true);
	_dc.DrawText(item.label, iconX + iconSize + FromDIP(ICON_TEXT_GAP), textY);
}

#pragma endregion

#pragma region Clicking

void FlatToolBar::ActivateItem(size_t _index, bool _onSplit)
{
	Item& item = items[_index];
	int sentId = item.id;

	switch (item.kind)
	{
	case Kind::Dropdown:
		ShowDropdownMenu(_index);
		return;
	case Kind::Radio:
		if (item.checked)
		{
			return;
		}

		SetChecked(item.id);
		break;
	case Kind::SplitButton:
		sentId = _onSplit ? item.menuId : item.id;
		break;
	default:
		break;
	}

	wxCommandEvent event(wxEVT_BUTTON, sentId);
	event.SetEventObject(this);
	ProcessWindowEvent(event);
}

int FlatToolBar::PopupMenuBelow(int _id, wxMenu& _menu)
{
	const int index = FindItem(_id);

	if (index == -1)
	{
		return wxID_NONE;
	}

	// open under the item
	const wxPoint menuPosition = items[index].rect.GetBottomLeft() + wxPoint(0, FromDIP(4));
	return GetPopupMenuSelectionFromUser(_menu, menuPosition);
}

void FlatToolBar::ShowDropdownMenu(size_t _index)
{
	const Item& item = items[_index];
	wxMenu menu;

	for (size_t i = 0; i < item.choices.size(); ++i)
	{
		wxMenuItem* menuItem = menu.AppendCheckItem(FIRST_CHOICE_MENU_ID + static_cast<int>(i), item.choices[i]);
		menuItem->Check(static_cast<int>(i) == item.selectedChoice);
	}

	const int pickedId = PopupMenuBelow(item.id, menu);

	if (pickedId == wxID_NONE)
	{
		return;
	}

	const int pickedChoice = pickedId - FIRST_CHOICE_MENU_ID;
	SetDropdownSelection(item.id, pickedChoice);

	wxCommandEvent event(wxEVT_CHOICE, item.id);
	event.SetEventObject(this);
	event.SetInt(pickedChoice);
	ProcessWindowEvent(event);
}

#pragma endregion

#pragma region Events

void FlatToolBar::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	const wxSize client = GetClientSize();

	dc.SetBackground(wxBrush(Theme::SURFACE));
	dc.Clear();

	// shapes use a graphics context for smooth corners
	{
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(dc));

		if (gc)
		{
			gc->SetPen(wxPen(Theme::BORDER, 1));
			gc->StrokeLine(0, client.y - 0.5, client.x, client.y - 0.5);

			DrawRadioTracks(*gc);

			for (size_t i = 0; i < items.size(); ++i)
			{
				DrawItemBackground(*gc, i);
			}
		}
	}

	// text and icons use the DC to keep ClearType
	if (titleVisible)
	{
		dc.SetFont(TitleFont(this));
		dc.SetTextForeground(Theme::TEXT);
		dc.DrawText(title, titlePosition);
	}

	dc.SetFont(LabelFont(this));

	for (size_t i = 0; i < items.size(); ++i)
	{
		DrawItemContent(dc, i);
	}
}

void FlatToolBar::OnSize(wxSizeEvent& _event)
{
	LayoutItems();
	Refresh();
	_event.Skip();
}

void FlatToolBar::OnMotion(wxMouseEvent& _event)
{
	const int hit = HitTest(_event.GetPosition());
	const bool onSplit = IsOverSplit(hit, _event.GetPosition());

	if (hit == hoveredIndex && onSplit == hoveredOnSplit)
	{
		return;
	}

	hoveredIndex = hit;
	hoveredOnSplit = onSplit;
	const bool overEnabledItem = hit != -1 && items[hit].enabled;
	SetCursor(overEnabledItem ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
	Refresh();
}

void FlatToolBar::OnLeave(wxMouseEvent&)
{
	if (hoveredIndex != -1 && !HasCapture())
	{
		hoveredIndex = -1;
		hoveredOnSplit = false;
		SetCursor(wxNullCursor);
		Refresh();
	}
}

void FlatToolBar::OnLeftDown(wxMouseEvent& _event)
{
	pressedIndex = HitTest(_event.GetPosition());
	pressedOnSplit = IsOverSplit(pressedIndex, _event.GetPosition());

	if (pressedIndex == -1 || !items[pressedIndex].enabled)
	{
		return;
	}

	if (!HasCapture())
	{
		CaptureMouse();
	}

	Refresh();
}

void FlatToolBar::OnLeftUp(wxMouseEvent& _event)
{
	if (HasCapture())
	{
		ReleaseMouse();
	}

	// only a click if released over the same item and part
	const int hit = HitTest(_event.GetPosition());
	const bool onSplit = IsOverSplit(hit, _event.GetPosition());
	const bool isClick = hit != -1 && hit == pressedIndex && onSplit == pressedOnSplit && items[hit].enabled;
	pressedIndex = -1;
	Refresh();

	if (isClick)
	{
		ActivateItem(hit, onSplit);
	}
}

void FlatToolBar::OnCaptureLost(wxMouseCaptureLostEvent&)
{
	pressedIndex = -1;
	Refresh();
}

void FlatToolBar::OnDpiChanged(wxDPIChangedEvent& _event)
{
	// icons were made for the old DPI
	iconCache.clear();
	SetMinSize(wxSize(-1, FromDIP(BAR_HEIGHT)));
	LayoutItems();
	Refresh();
	_event.Skip();
}

#pragma endregion
