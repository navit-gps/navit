extern "C" {
#include <math.h>
#include "sunriset.h"
}
#include <gtest/gtest.h>

TEST(SunTimeTestSuite, sunriset_equator) {
    double rise;
    double tset;
    // Values from https://api.sunrise-sunset.org/json?lat=0.0&lng=0.0&date=2020-01-01
    ASSERT_EQ(__sunriset__(2020,1,1,0.0,0.0,-0.8,1,&rise,&tset), 0);
    ASSERT_EQ(HOURS(rise),5);
    ASSERT_EQ(HOURS(tset),18);
    // TODO: Check minutes in Range?
}

TEST(SunTimeTestSuite, pole) {
    double rise;
    double tset;
    // https://api.sunrise-sunset.org/json?lat=90.0&lng=0.0&date=2020-01-01
    ASSERT_EQ(__sunriset__(2020,1,1,0.0,90.0,-0.8,1,&rise,&tset), -1);
    ASSERT_EQ(HOURS(rise),12);
    ASSERT_EQ(HOURS(tset),12);
}
