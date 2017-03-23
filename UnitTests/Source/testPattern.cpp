#include "doctest.h"

#include "Document/PatternData_new.h"
#include "Document/TrackData.h"

using namespace FTExt;

TEST_SUITE("Pattern data");

SCENARIO("Track data test") {
	GIVEN("A default-constructed track") {
		CTrackData t;
		{
			bool id = true;
			for (size_t i = 0; i < MAX_FRAMES; ++i)
				if (t.GetPatternIndex(i)) {
					id = false; break;
				}
			REQUIRE(id);
		}

		WHEN("Pattern is accessed") {
			THEN("Pointer to pattern should be returned") {
				REQUIRE(t.GetPattern(0) != nullptr);
				REQUIRE(t.GetPattern(1) != nullptr);
				REQUIRE(t.GetPattern(MAX_FRAMES - 1) != nullptr);
			}
			AND_THEN("Patterns should be unique") {
				REQUIRE(t.GetPattern(0) != t.GetPattern(1));
				REQUIRE(t.GetPattern(0) != t.GetPattern(MAX_FRAMES - 1));
			}
		}

		WHEN("Out-of-bound pattern is accessed") {
			THEN("Track should return nullptr") {
				REQUIRE(t.GetPattern(MAX_FRAMES) == nullptr);
				REQUIRE(t.GetPattern((size_t)-1) == nullptr);
			}
		}
	}
}

TEST_SUITE_END;
