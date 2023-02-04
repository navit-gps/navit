#include "navit/coord.h"
#include <gtest/gtest.h>

TEST(CoordsTestSuite, none_dec) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300,6.334000";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, mg_dec) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300,6.334000";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, 704308);
    ASSERT_EQ(result->y, -3938147);
}

TEST(CoordsTestSuite, garmin_dec) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300,6.334000";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, 295185);
    ASSERT_EQ(result->y, -1554469);
}

TEST(CoordsTestSuite, utm_dec) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300,6.334000";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, none_google_maps) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "52.678595, 13.572752";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 20);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, mg_google_maps) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "52.678595, 13.572752";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 20);

    ASSERT_EQ(result->x, 1509221);
    ASSERT_EQ(result->y, 6916019);
}

TEST(CoordsTestSuite, garmin_google_maps) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "52.678595, 13.572752";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 20);

    ASSERT_EQ(result->x, 632536);
    ASSERT_EQ(result->y, 2455000);
}

TEST(CoordsTestSuite, utm_google_maps) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "52.678595, 13.572752";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 20);

    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, none_deg_min) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "4808.2356 N 1134.5252 E";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, mg_deg_min) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "4808.2356 N 1134.5252 E";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 1287127);
    ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, garmin_deg_min) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "4808.2356 N 1134.5252 E";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 539453);
    ASSERT_EQ(result->y, 2243358);
}

TEST(CoordsTestSuite, utm_deg_min) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "4808.2356 N 1134.5252 E";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, none_mercator) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 1287127); // TODO: should be null?
    ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, mg_mercator) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 1287127);
    ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, garmin_mercator) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 539453);
    ASSERT_EQ(result->y, 2243358);
}

TEST(CoordsTestSuite, utm_mercator) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 1287127);
    ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, none_mg_prefix) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "mg: 0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 21);
    ASSERT_EQ(result->x, 1287127);
	ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, mg_mg_prefix) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "mg: 0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 21);
    ASSERT_EQ(result->x, 1287127);
	ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, garmin_mg_prefix) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "mg: 0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 21);
    ASSERT_EQ(result->x, 539453);
	ASSERT_EQ(result->y, 2243358);
}

TEST(CoordsTestSuite, utm_mg_prefix) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "mg: 0x13a3d7 0x5d6d6d";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 21);
    ASSERT_EQ(result->x, 1287127);
	ASSERT_EQ(result->y, 6122861);
}

TEST(CoordsTestSuite, none_utm) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utm32N: 674661 	5328114";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 32674661);
	ASSERT_EQ(result->y, 5328114);
}

TEST(CoordsTestSuite, mg_utm) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utm32N: 674661 	5328114";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 1261533);
	ASSERT_EQ(result->y, 6113717);
}

TEST(CoordsTestSuite, garmin_utm) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utm32N: 674661 	5328114";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 528726);
	ASSERT_EQ(result->y, 2240800);
}

TEST(CoordsTestSuite, utm_utm) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utm32N: 674661 	5328114";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 23);
    ASSERT_EQ(result->x, 32674661);
	ASSERT_EQ(result->y, 5328114);
}

TEST(CoordsTestSuite, none_utmref) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utmref32UMD:74 03";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 32400074);
	ASSERT_EQ(result->y, 5800003);
}

TEST(CoordsTestSuite, mg_utmref) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utmref32UMD:74 03";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 837654);
	ASSERT_EQ(result->y, 6854379);
}

TEST(CoordsTestSuite, garmin_utmref) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utmref32UMD:74 03";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 351072);
	ASSERT_EQ(result->y, 2439277);
}


TEST(CoordsTestSuite, utm_utmref) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "utmref32UMD:74 03";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 17);
    ASSERT_EQ(result->x, 32400074);
	ASSERT_EQ(result->y, 5800003);
}

TEST(CoordsTestSuite, none_dec_space) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300 6.334000";
    int out = coord_parse(coord_str, projection_none, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}

TEST(CoordsTestSuite, mg_dec_space) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300 6.334000";
    int out = coord_parse(coord_str, projection_mg, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, -3708940);
    ASSERT_EQ(result->y, 705747);
}

TEST(CoordsTestSuite, garmin_dec_space) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300 6.334000";
    int out = coord_parse(coord_str, projection_garmin, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, -1554469);
    ASSERT_EQ(result->y, 295185);
}

TEST(CoordsTestSuite, utm_dec_space) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300 6.334000";
    int out = coord_parse(coord_str, projection_utm, result);
    ASSERT_EQ(out, 19);
    ASSERT_EQ(result->x, 0);
    ASSERT_EQ(result->y, 0);
}
