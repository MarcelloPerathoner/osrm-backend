#include "util/viewport.hpp"

using namespace osrm::util;

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(viewport_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(zoom_level_test)
{
    BOOST_CHECK_EQUAL(viewport::getFittedZoom(5.668343999999995_lon + 45.111511000000014_lat,
                                              5.852471999999996_lon + 45.26800200000002_lat),
                      12);
}

BOOST_AUTO_TEST_SUITE_END()
