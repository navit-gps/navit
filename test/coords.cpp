#include "navit/coord.h"
#include <gtest/gtest.h>

TEST(CoordsTestSuite, none_dec) {
    struct coord *result = coord_new(0,0);
    const char* coord_str = "-33.355300,6.334000";
    int out = coord_parse(coord_str,projection_none,result);
    ASSERT_EQ(out, 19);

    // projection_none will not output anything new
    ASSERT_EQ(result->x,0);
    ASSERT_EQ(result->y,0);
}
