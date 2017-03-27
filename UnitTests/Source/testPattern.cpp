#include "doctest.h"

#include "Document/PatternData_new.h"
#include "Document/TrackData.h"

using namespace FTExt;

TEST_SUITE("Pattern data");

SCENARIO("Pattern data test") {
	GIVEN("A default-constructed pattern") {
		CPatternData p;
		auto &&cp = std::as_const(p);

		
	}
}

SCENARIO("Track data test") {
	GIVEN("A track object") {
		CTrackData t;
		auto &&ct = std::as_const(t);

		AND_GIVEN("It is default-constructed") {
			WHEN("At the beginning") {
				THEN("All pattern indices should be initially 00") {
					bool id = true;
					for (size_t i = 0; i < MAX_FRAMES; ++i)
						if (t.GetPatternIndex(i)) {
							id = false; break;
						}
					REQUIRE(id);
					AND_THEN("Track should contain 1 effect column") {
						REQUIRE(t.GetEffectColumnCount() == 1);
					}
				}
			}

			AND_WHEN("Pattern is accessed as const") {
				auto ptr = ct.GetPattern(0);
				THEN("No patterns should be created") {
					REQUIRE(ptr == nullptr);
					REQUIRE(ct.GetPattern(MAX_FRAMES) == nullptr);
				}
			}

			AND_WHEN("Pattern is accessed") {
				auto ptr = t.GetPattern(0);
				THEN("Pointer to created pattern should be returned") {
					REQUIRE(ptr != nullptr);
					AND_THEN("Patterns should be unique") {
						REQUIRE(ptr != t.GetPattern(1));
						REQUIRE(ptr != t.GetPattern(MAX_FRAMES - 1));
					}
				}
				AND_WHEN("The same pattern index is used to access it again") {
					auto p2 = t.GetPattern(0);
					THEN("The patterns should be identical") {
						REQUIRE(ptr == p2);
					}
				}
			}

			AND_WHEN("Out-of-bound pattern indices are accessed") {
				THEN("nullptr should be returned") {
					REQUIRE(t.GetPattern(MAX_FRAMES) == nullptr);
					REQUIRE(t.GetPattern((size_t)-1) == nullptr);
					REQUIRE(ct.GetPattern(MAX_FRAMES) == nullptr);
					REQUIRE(ct.GetPattern((size_t)-1) == nullptr);
				}
			}

			AND_WHEN("Effect column count is changed") {
				THEN("Track should be able to hold 1 - 4 columns") {
					t.SetEffectColumnCount(2);
					REQUIRE(t.GetEffectColumnCount() == 2);
					t.SetEffectColumnCount(4);
					REQUIRE(t.GetEffectColumnCount() == 4);
					t.SetEffectColumnCount(1);
					REQUIRE(t.GetEffectColumnCount() == 1);
					t.SetEffectColumnCount(3);
					REQUIRE(t.GetEffectColumnCount() == 3);
					const int Count = t.GetEffectColumnCount();
					AND_THEN("Column count should not change otherwise") {
						try { t.SetEffectColumnCount(5); } catch (...) { }
						REQUIRE(t.GetEffectColumnCount() == Count);
						try { t.SetEffectColumnCount(0); } catch (...) { }
						REQUIRE(t.GetEffectColumnCount() == Count);
					}
				}
			}
		}
	}
}

TEST_SUITE_END;
