#pragma once
#include "LookupConfigs.h"
#include "LookupFilters.h"
#include "LookupNPC.h"
#include "Testing.h"
#include "TestsHelpers.h"

namespace DeterministicChance
{
	using namespace Testing;

	namespace Testing
	{
		namespace Parsing
		{
			constexpr static const char* moduleName = "DeterministicChance.Parsing";

			struct ChanceData
			{
				Chance chance{};
			};

			TEST(ParsesPercentChanceAsDecimal)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("50", data);
				EXPECT(data.chance.value == 0.5, fmt::format("Expected value 0.5 but got {}", data.chance.value));
			}

			TEST(DeterministicSuffixSetsFlag)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("50!", data);
				EXPECT(data.chance.deterministic, "Expected '50!' to set deterministic flag");
			}

			TEST(NoSuffixLeavesFlagFalse)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("50", data);
				EXPECT(!data.chance.deterministic, "Expected '50' to leave deterministic flag false");
			}

			TEST(DeterministicEntryParsesValueCorrectly)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("75!", data);
				EXPECT(data.chance.value == 0.75, fmt::format("Expected value 0.75 but got {}", data.chance.value));
			}

			TEST(ParsesFullChance)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("100", data);
				EXPECT(data.chance.value == 1.0, fmt::format("Expected value 1.0 but got {}", data.chance.value));
			}

			TEST(ParsesZeroChance)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("0", data);
				EXPECT(data.chance.value == 0.0, fmt::format("Expected value 0.0 but got {}", data.chance.value));
			}

			TEST(ParsesDecimalPercentChance)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("50.5", data);
				EXPECT(std::abs(data.chance.value - 0.505) < 1e-9, fmt::format("Expected value 0.505 but got {}", data.chance.value));
			}

			TEST(ParsesDeterministicDecimalChance)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("50.5!", data);
				ASSERT(data.chance.deterministic, "Expected '50.5!' to set deterministic flag");
				EXPECT(std::abs(data.chance.value - 0.505) < 1e-9, fmt::format("Expected value 0.505 but got {}", data.chance.value));
			}

			TEST(ParsesDecimalNearFullChance)
			{
				ChanceData data;
				Distribution::INI::ChanceComponentParser{}("99.9!", data);
				ASSERT(data.chance.deterministic, "Expected '99.9!' to set deterministic flag");
				EXPECT(std::abs(data.chance.value - 0.999) < 1e-9, fmt::format("Expected value 0.999 but got {}", data.chance.value));
			}

			TEST(SameEntryLineProducesSameLineSeed)
			{
				const std::string key = "Spell";
				const std::string value = "0x10F7F7||||||50!";

				Distribution::INI::configs.clear();
				Distribution::INI::TryParse(key, value, "test.ini");
				Distribution::INI::TryParse(key, value, "test.ini");

				auto& entries = Distribution::INI::configs[RECORD::kSpell];
				ASSERT(entries.size() == 2, fmt::format("Expected 2 parsed entries but got {}", entries.size()));
				EXPECT(entries[0].chance.lineSeed == entries[1].chance.lineSeed, "Expected identical entry lines to produce the same lineSeed");
			}

			TEST(SlightlyDifferentEntryLineProducesDifferentLineSeed)
			{
				const std::string key = "Spell";
				const std::string value1 = "0x10F7F7||||||50!";
				const std::string value2 = "0x10F7F7||||||51!";

				Distribution::INI::configs.clear();
				Distribution::INI::TryParse(key, value1, "test.ini");
				Distribution::INI::TryParse(key, value2, "test.ini");

				auto& entries = Distribution::INI::configs[RECORD::kSpell];
				ASSERT(entries.size() == 2, fmt::format("Expected 2 parsed entries but got {}", entries.size()));
				EXPECT(entries[0].chance.lineSeed != entries[1].chance.lineSeed, "Expected entry lines differing only in chance to produce different lineSeeds");
			}
		}

		namespace DeterministicFilters
		{
			constexpr static const char* moduleName = "DeterministicChance.Filters.Deterministic";

			TEST(SameCallGivesSameResult)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				Chance deterministicChance(0.5, true);
				deterministicChance.lineSeed = std::hash<std::string>{}("Spell = 0x10F7F7||||||50!");
				::FilterData filterData{ {}, {}, {}, {}, deterministicChance };

				auto expected = filterData.PassedFilters(npcData);
				for (int i = 1; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == expected, fmt::format("Expected deterministic 50% chance to give the same result on call {}/{}", i + 1, N));
				}
				PASS;
			}

			TEST(ZeroChanceAlwaysFails)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				Chance zeroChance(0.0, true);
				zeroChance.lineSeed = std::hash<std::string>{}("Spell = 0x10F7F7||||||0!");
				::FilterData filterData{ {}, {}, {}, {}, zeroChance };

				for (int i = 0; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == Filter::Result::kFailRNG, fmt::format("Expected 0% chance to fail RNG on call {}/{}", i + 1, N));
				}
				PASS;
			}

			TEST(FullChanceAlwaysPasses)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				::FilterData filterData{ {}, {}, {}, {}, Chance(1.0, true) };

				for (int i = 0; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == Filter::Result::kPass, fmt::format("Expected 100% chance to pass on call {}/{}", i + 1, N));
				}
				PASS;
			}

			TEST(LowChanceIsStable)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				Chance lowChance(0.1, true);
				lowChance.lineSeed = std::hash<std::string>{}("Spell = 0x10F7F7||||||10!");
				::FilterData filterData{ {}, {}, {}, {}, lowChance };

				auto expected = filterData.PassedFilters(npcData);
				for (int i = 1; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == expected, fmt::format("Expected 10% chance to give the same result on call {}/{}", i + 1, N));
				}
				PASS;
			}

			TEST(HighChanceIsStable)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				Chance highChance(0.9, true);
				highChance.lineSeed = std::hash<std::string>{}("Spell = 0x10F7F7||||||90!");
				::FilterData filterData{ {}, {}, {}, {}, highChance };

				auto expected = filterData.PassedFilters(npcData);
				for (int i = 1; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == expected, fmt::format("Expected 90% chance to give the same result on call {}/{}", i + 1, N));
				}
				PASS;
			}
		}

		namespace NonDeterministicFilters
		{
			constexpr static const char* moduleName = "DeterministicChance.Filters.NonDeterministic";

			TEST(ProducesVariation)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				// 50% gives the lowest probability of all-same across 10 calls (~0.2%),
				// minimizing flake risk while still asserting that a true RNG path is in use.
				::FilterData filterData{ {}, {}, {}, {}, Chance(0.5, false) };

				auto first = filterData.PassedFilters(npcData);
				bool sawDifferent = false;
				for (int i = 1; i < N; ++i) {
					if (filterData.PassedFilters(npcData) != first) {
						sawDifferent = true;
					}
				}
				EXPECT(sawDifferent, "Expected 50% chance to produce different results across 10 calls");
			}

			TEST(IgnoresLineSeed)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				// lineSeed should be unused when deterministic is false; results must still vary.
				Chance nonDeterministicChance(0.5, false);
				nonDeterministicChance.lineSeed = std::hash<std::string>{}("Spell = 0x10F7F7||||||50");
				::FilterData filterData{ {}, {}, {}, {}, nonDeterministicChance };

				auto first = filterData.PassedFilters(npcData);
				bool sawDifferent = false;
				for (int i = 1; i < N; ++i) {
					if (filterData.PassedFilters(npcData) != first) {
						sawDifferent = true;
					}
				}
				EXPECT(sawDifferent, "Expected results to vary regardless of lineSeed");
			}

			TEST(ZeroChanceAlwaysFails)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				::FilterData filterData{ {}, {}, {}, {}, Chance(0.0, false) };

				for (int i = 0; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == Filter::Result::kFailRNG, fmt::format("Expected 0% chance to fail RNG on call {}/{}", i + 1, N));
				}
				PASS;
			}

			TEST(FullChanceAlwaysPasses)
			{
				constexpr int N = 10;

				auto    actor = ::Testing::Helper::Actor::GetActor();
				NPCData npcData(actor);

				// 100% chance bypasses RNG entirely (chance.value < 1 short-circuits).
				::FilterData filterData{ {}, {}, {}, {}, Chance(1.0, false) };

				for (int i = 0; i < N; ++i) {
					auto result = filterData.PassedFilters(npcData);
					ASSERT(result == Filter::Result::kPass, fmt::format("Expected 100% chance to pass on call {}/{}", i + 1, N));
				}
				PASS;
			}
		}
	}
}
