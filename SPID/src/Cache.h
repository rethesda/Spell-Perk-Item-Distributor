#pragma once

namespace FormType
{
	inline constexpr std::array whitelist{
		RE::FormType::Faction,
		RE::FormType::Class,
		RE::FormType::CombatStyle,
		RE::FormType::Race,
		RE::FormType::Outfit,
		RE::FormType::NPC,
		RE::FormType::ActorCharacter,
		RE::FormType::VoiceType,
		RE::FormType::FormList,
		RE::FormType::Spell,
		RE::FormType::Armor,
		RE::FormType::Location,
		RE::FormType::Perk,
		// Inventory-checked item types (see NPC::Data::has_item_in_inventory)
		RE::FormType::Weapon,
		RE::FormType::Ammo,
		RE::FormType::Ingredient,
		RE::FormType::AlchemyItem,
		RE::FormType::Book,
		RE::FormType::Misc,
		RE::FormType::SoulGem,
		RE::FormType::KeyMaster,
		RE::FormType::Scroll,
		RE::FormType::Note,
		RE::FormType::Apparatus,
		RE::FormType::Light
	};

	bool GetWhitelisted(RE::FormType a_type);
}
