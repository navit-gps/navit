#include "navit/search.h"
#include <gtest/gtest.h>

TEST(SearchTestSuite, search_fix_spaces) {
    const char *in = " Hello, world/!  ";
    ASSERT_STREQ(search_fix_spaces(in), "Hello world !");
    // Explicit cast is necessary here because cpputest runs with -Wwrite-strings which assumes ISO C++ is respected
    char *non_const = (char *)"Spaces after string...  ";
    ASSERT_STREQ(search_fix_spaces(non_const), "Spaces after string...");
    ASSERT_STREQ(search_fix_spaces("   Spaces before string."), "Spaces before string.");
    ASSERT_STREQ(search_fix_spaces("Don't change a thing!"), "Don't change a thing!");
    ASSERT_STREQ(search_fix_spaces(""), "");
    ASSERT_STREQ(search_fix_spaces(", / "), "");
    ASSERT_STREQ(search_fix_spaces(", */ "), "*");
}
