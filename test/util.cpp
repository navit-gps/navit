#include <gtest/gtest.h>

extern "C"{
#include "navit/util.h"
}

TEST(UtilTimeTestSuite, base) {
	unsigned int result = 0;
	char *time_str = strdup("2007-12-24T18:21:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1198520460);

	time_str = strdup("2007-12-24T18:21Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1198520460);
}

TEST(UtilTimeTestSuite, zero) {
	unsigned int result = 0;
	char *time_str = strdup("1970-01-01T00:00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 0);

	time_str = strdup("1970-01-01T00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 0);
}

TEST(UtilTimeTestSuite, last_32bit) {
	unsigned int result = 0;
	char *time_str = strdup("2038-01-19T03:14:07Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, INT32_MAX);

	time_str = strdup("2038-01-19T03:14Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, INT32_MAX-7); // 07 SECONDS REMOVED
}

TEST(UtilTimeTestSuite, feb){
	unsigned int result = 0;
	char *time_str = strdup("2021-02-01T00:00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1612137600);

	time_str = strdup("2021-02-01T00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1612137600);
}

TEST(UtilTimeTestSuite, mar){
	unsigned int result = 0;
	char *time_str = strdup("2021-03-01T00:00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1614556800);

	time_str = strdup("2021-03-01T00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1614556800);
}

TEST(UtilTimeTestSuite, feb_leep){
	unsigned int result = 0;
	char *time_str = strdup("2020-02-29T00:00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1582934400);

	time_str = strdup("2020-02-29T00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1582934400);
}

TEST(UtilTimeTestSuite, mar_leep){
	unsigned int result = 0;
	char *time_str = strdup("2020-03-01T00:00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1583020800);

	time_str = strdup("2020-03-01T00:00Z");
	result = iso8601_to_secs(time_str);
	ASSERT_EQ(result, 1583020800);
}


