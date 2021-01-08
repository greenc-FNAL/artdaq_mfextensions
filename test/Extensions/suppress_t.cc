#include "mfextensions/Extensions/suppress.hh"

#define BOOST_TEST_MODULE suppress_t
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

#define TRACE_NAME "suppress_t"
#include "tracemf.h"

BOOST_AUTO_TEST_SUITE(suppress_t)

BOOST_AUTO_TEST_CASE(Suppress)
{
	suppress s("test");

	BOOST_REQUIRE(s.match("test"));
	BOOST_REQUIRE(!s.match("another_test"));
	BOOST_REQUIRE(!s.match("testing"));
	BOOST_REQUIRE(!s.match("quiz"));

	s.use(false);
	BOOST_REQUIRE(!s.match("test"));

	s.use(true);
	BOOST_REQUIRE(s.match("test"));
}

BOOST_AUTO_TEST_SUITE_END()
