#include "bin/Rendering/Drawer.h"

#include "WheelItems/WheelItem.h"
#include "WheelItems/WheelItemFactory.h"
#include "WheelItems/WheelItemMutable.h"
#include "WheelEntry.h"

void WheelEntry::DrawBackGround(
	const ImVec2 wheelCenter, float innerSpacingRad, 
	float entryInnerAngleMin, float entryInnerAngleMax,
	float entryOuterAngleMin, float entryOuterAngleMax, 
	const ImVec2 itemCenter, bool hovered, 
	int numArcSegments, RE::TESObjectREFR::InventoryItemMap& inv, DrawArgs a_drawARGS)
{
	using namespace Config::Styling::Wheel;

	float mainArcOuterBoundRadius = OuterCircleRadius;
	mainArcOuterBoundRadius += _arcRadiusIncInterpolator.GetValue();
	mainArcOuterBoundRadius += _arcRadiusBounceInterpolator.GetValue();
	
	float entryInnerAngleMinUpdated = entryInnerAngleMin - _arcInnerAngleIncInterpolator.GetValue() * 2;
	float entryInnerAngleMaxUpdated = entryInnerAngleMax + _arcInnerAngleIncInterpolator.GetValue() * 2;
	float entryOuterAngleMinUpdated = entryOuterAngleMin - _arcOuterAngleIncInterpolator.GetValue() * 2;
	float entryOuterAngleMaxUpdated = entryOuterAngleMax + _arcOuterAngleIncInterpolator.GetValue() * 2;
	
	Drawer::draw_arc_gradient(wheelCenter,
		InnerCircleRadius,
		mainArcOuterBoundRadius,
		entryInnerAngleMinUpdated, entryInnerAngleMaxUpdated,
		entryOuterAngleMinUpdated, entryOuterAngleMaxUpdated,
		hovered ? HoveredColorBegin : UnhoveredColorBegin,
		hovered ? HoveredColorEnd : UnhoveredColorEnd,
		numArcSegments, a_drawARGS);

	bool active = this->IsActive(inv);
	ImU32 arcColorBegin = active ? ActiveArcColorBegin : InActiveArcColorBegin;
	ImU32 arcColorEnd = active ? ActiveArcColorEnd : InActiveArcColorEnd;

	Drawer::draw_arc_gradient(wheelCenter,
		mainArcOuterBoundRadius,
		mainArcOuterBoundRadius + ActiveArcWidth,
		entryOuterAngleMinUpdated,
		entryOuterAngleMaxUpdated,
		entryOuterAngleMinUpdated,
		entryOuterAngleMaxUpdated,
		arcColorBegin,
		arcColorEnd,
		numArcSegments, a_drawARGS);

	if (hovered) {
		if (!_prevHovered) {
			float expandSize = Config::Animation::EntryHighlightExpandScale * (OuterCircleRadius - InnerCircleRadius);
			_arcRadiusIncInterpolator.InterpolateTo(expandSize, Config::Animation::EntryHighlightExpandTime);
			_arcOuterAngleIncInterpolator.InterpolateTo(innerSpacingRad * (InnerCircleRadius / OuterCircleRadius), Config::Animation::EntryHighlightExpandTime);
			_arcInnerAngleIncInterpolator.InterpolateTo(innerSpacingRad, Config::Animation::EntryHighlightExpandTime);
			_prevHovered = true;
		}
	} else {
		if (_prevHovered != false) {
			_prevHovered = false;
			_arcRadiusIncInterpolator.InterpolateTo(0, Config::Animation::EntryHighlightRetractTime);
			_arcOuterAngleIncInterpolator.InterpolateTo(0, Config::Animation::EntryHighlightRetractTime);
			_arcInnerAngleIncInterpolator.InterpolateTo(0, Config::Animation::EntryHighlightRetractTime);
		}
	}
}

void WheelEntry::DrawSlotAndHighlight(ImVec2 a_wheelCenter, ImVec2 a_entryCenter, bool a_hovered, RE::TESObjectREFR::InventoryItemMap& a_imap, DrawArgs a_drawArgs)
{
	if (a_hovered) {
		this->drawHighlight(a_wheelCenter, a_imap, a_drawArgs);
	}
	this->drawSlot(a_entryCenter, a_hovered, a_imap, a_drawArgs);
}

void WheelEntry::drawSlot(ImVec2 a_center, bool a_hovered, RE::TESObjectREFR::InventoryItemMap& a_imap, DrawArgs a_drawArgs)
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);

	if (_items.size() == 0) {
		return; // nothing to draw
	}
	_items[_selectedItem]->DrawSlot(a_center, a_hovered, a_imap, a_drawArgs);
}

void WheelEntry::drawHighlight(ImVec2 a_center, RE::TESObjectREFR::InventoryItemMap& a_imap, DrawArgs a_drawArgs)
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);

	if (_items.size() == 0) {
		return;  // nothing to draw
	}
	_items[_selectedItem]->DrawHighlight(a_center, a_imap, a_drawArgs);
	if (_items.size() > 1) {
		Drawer::draw_text(
			a_center.x + Config::Styling::Entry::Highlight::Text::OffsetX,
			a_center.y + Config::Styling::Entry::Highlight::Text::OffsetY,
			fmt::format("{} / {}", _selectedItem + 1, _items.size()).data(),
			C_SKYRIMWHITE,
			Config::Styling::Entry::Highlight::Text::Size,
			a_drawArgs
			);
	}
}

bool WheelEntry::IsActive(RE::TESObjectREFR::InventoryItemMap& a_inv)
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);

	if (_items.size() == 0) {
		return false;  // nothing to draw
	}
	return _items[_selectedItem]->IsActive(a_inv);
}

bool WheelEntry::IsAvailable(RE::TESObjectREFR::InventoryItemMap& a_inv)
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);

	if (_items.size() == 0) {
		return false;  // nothing to draw
	}
	return _items[_selectedItem]->IsAvailable(a_inv);
}

void WheelEntry::ActivateItemSecondary(bool editMode)
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);

	if (_items.size() == 0) {
		return;
	}
	if (!editMode) {
		_items[_selectedItem]->ActivateItemSecondary();
		_arcRadiusBounceInterpolator.InterpolateTo(-10, 0.1f);
	} else {
		// remove selected item
		std::shared_ptr<WheelItem> itemToDelete = _items[_selectedItem];
		_items.erase(_items.begin() + _selectedItem);
		// move _selecteditem to the item immediately before the erased item
		if (_selectedItem > 0) {
			_selectedItem--;
		}
	}
}

void WheelEntry::ActivateItemPrimary(bool editMode)
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);

	if (!editMode) { 
		if (_items.size() == 0) {
			return;  // nothing to erase
		}
		_arcRadiusBounceInterpolator.InterpolateTo(-10, 0.1f);
		_items[_selectedItem]->ActivateItemPrimary();
	} else {// append item to after _selectedItem index
		std::shared_ptr<WheelItem> newItem = WheelItemFactory::MakeWheelItemFromMenuHovered();
		if (newItem) {
			_items.insert(_items.begin() + _selectedItem, newItem);
		}
	}
}

void WheelEntry::ActivateItemSpecial(bool editMode)
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);
	if (editMode || _items.size() == 0) {
		return; // nothing to do
	}
	_items[_selectedItem]->ActivateItemSpecial();
}

void WheelEntry::PrevItem()
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);

	_selectedItem--;
	if (_selectedItem < 0) {
		if (_items.size() > 1) {
			_selectedItem = _items.size() - 1;
		} else {
			_selectedItem = 0;
		}
	}
	if (_items.size() > 1) {
		RE::PlaySoundRE(Config::Sound::SD_ITEMSWITCH);
	}
}

void WheelEntry::NextItem()
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);

	_selectedItem++;
	if (_selectedItem >= _items.size()) {
		_selectedItem = 0;
	}
	if (_items.size() > 1) {
		RE::PlaySoundRE(Config::Sound::SD_ITEMSWITCH);
	}
}

bool WheelEntry::IsEmpty()
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);
	return this->_items.empty();
}

std::vector<std::shared_ptr<WheelItem>>& WheelEntry::GetItems()
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);
	return this->_items;
}

WheelEntry::WheelEntry()
{
	_selectedItem = 0;
}

void WheelEntry::PushItem(std::shared_ptr<WheelItem> item)
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);
	this->_items.push_back(item);
}

WheelEntry::~WheelEntry()
{
	_items.clear();
}

int WheelEntry::GetSelectedItem()
{
	std::shared_lock<std::shared_mutex> lock(this->_lock);
	return this->_selectedItem;
}

void WheelEntry::SerializeIntoJsonObj(nlohmann::json& j_entry)
{
	// setup for entry
	j_entry["items"] = nlohmann::json::array();
	for (std::shared_ptr<WheelItem> item : this->_items) {
		nlohmann::json j_item;
		item->SerializeIntoJsonObj(j_item);
		j_entry["items"].push_back(j_item);
	}
	j_entry["selecteditem"] = this->_selectedItem;
}

std::unique_ptr<WheelEntry> WheelEntry::SerializeFromJsonObj(const nlohmann::json& j_entry, SKSE::SerializationInterface* a_intfc)
{
	std::unique_ptr<WheelEntry> entry = std::make_unique<WheelEntry>();
	entry->SetSelectedItem(j_entry["selecteditem"]);

	nlohmann::json j_items = j_entry["items"];
	for (const auto& j_item : j_items) {
		std::shared_ptr<WheelItem> item = WheelItemFactory::MakeWheelItemFromJsonObject(j_item, a_intfc);
		if (item) {
			entry->PushItem(std::move(item));
		}
	}
	return std::move(entry);
}

void WheelEntry::ResetAnimation()
{
	_arcInnerAngleIncInterpolator.ForceValue(0);
	_arcOuterAngleIncInterpolator.ForceValue(0);
	_arcRadiusIncInterpolator.ForceValue(0);
	_arcRadiusBounceInterpolator.ForceValue(0);
}


void WheelEntry::SetSelectedItem(int a_selected)
{
	std::unique_lock<std::shared_mutex> lock(this->_lock);
	this->_selectedItem = a_selected;
}

