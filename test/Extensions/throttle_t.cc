/**
 * @file throttle_t.cc
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#include "mfextensions/Extensions/throttle.hh"

#define BOOST_TEST_MODULE throttle_t
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

#define TRACE_NAME "throttle_t"
#include "TRACE/tracemf.h"

BOOST_AUTO_TEST_SUITE(throttle_t)

BOOST_AUTO_TEST_CASE(Throttle)
{
	throttle t("test", 2, 1);

	struct timeval tv;
	gettimeofday(&tv, nullptr);

	BOOST_REQUIRE(!t.reach_limit("test", tv));
	BOOST_REQUIRE(!t.reach_limit("test", tv));
	BOOST_REQUIRE(t.reach_limit("test", tv));
	BOOST_REQUIRE(!t.reach_limit("quiz", tv));

	t.use(false);
	BOOST_REQUIRE(!t.reach_limit("test", tv));

	t.use(true);
	tv.tv_sec += 2;
	BOOST_REQUIRE(!t.reach_limit("test", tv));
}

BOOST_AUTO_TEST_SUITE_END()
