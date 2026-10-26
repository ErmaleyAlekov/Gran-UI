#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"

namespace
{
    void testParseTracksAutoFraction()
    {
        auto tracks = gran::style::parseTracks("auto, 1fr");
        TEST_ASSERT(tracks.size() == 2);
        TEST_ASSERT(tracks[0].min.kind == gran::style::TrackBoundKind::AUTO);
        TEST_ASSERT(tracks[0].max.kind == gran::style::TrackBoundKind::AUTO);
        TEST_ASSERT(tracks[1].min.kind == gran::style::TrackBoundKind::AUTO);
        TEST_ASSERT(tracks[1].max.kind == gran::style::TrackBoundKind::FRACTION);
        TEST_ASSERT_NEAR(tracks[1].max.value, 1.0f, 0.001f);
    }

    void testParseTracksPixelsFraction()
    {
        auto tracks = gran::style::parseTracks("48 2fr 120px");
        TEST_ASSERT(tracks.size() == 3);
        TEST_ASSERT(tracks[0].min.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT(tracks[0].max.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT_NEAR(tracks[0].min.value, 48.0f, 0.001f);
        TEST_ASSERT_NEAR(tracks[0].max.value, 48.0f, 0.001f);
        TEST_ASSERT(tracks[2].min.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT(tracks[2].max.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT_NEAR(tracks[2].min.value, 120.0f, 0.001f);
        TEST_ASSERT_NEAR(tracks[2].max.value, 120.0f, 0.001f);
    }

    void testParseTracksMinMax()
    {
        auto tracks = gran::style::parseTracks("minmax(100, 1fr)");
        TEST_ASSERT(tracks.size() == 1);
        TEST_ASSERT(tracks[0].min.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT(tracks[0].max.kind == gran::style::TrackBoundKind::FRACTION);
        TEST_ASSERT_NEAR(tracks[0].min.value, 100.0f, 0.001f);
        TEST_ASSERT_NEAR(tracks[0].max.value, 1.0f, 0.001f);
    }

    void testParseTracksErrors()
    {
        TEST_ASSERT_THROWS(gran::style::parseTracks(""), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseTracks("minmax(1fr, 100)"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseTracks("-5"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseTracks("12em"), gran::GranException);
    }
}

namespace suites
{
    void registerStyleTrackTests(test::TestRunner& runner)
    {
        runner.add("style: parseTracks auto, 1fr", testParseTracksAutoFraction);
        runner.add("style: parseTracks 48 2fr 120px", testParseTracksPixelsFraction);
        runner.add("style: parseTracks minmax(100, 1fr)", testParseTracksMinMax);
        runner.add("style: parseTracks errors", testParseTracksErrors);
    }
}
