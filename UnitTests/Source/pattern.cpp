#include "doctest.h"

#include "Document/PatternData_new.h"
#include "Document/TrackData.h"

TEST_SUITE("Pattern data");

TEST_CASE("track data test") {
	FTExt::CTrackData t;
	SUBCASE("1") {
		REQUIRE(true);
		CHECK(false);
	}
}

TEST_SUITE_END();
