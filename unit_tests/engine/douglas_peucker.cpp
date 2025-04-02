#include "engine/douglas_peucker.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/debug.hpp"

#include <boost/test/unit_test.hpp>

#include <osrm/coordinate.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(douglas_peucker_simplification)

using namespace osrm;
using namespace osrm::engine;

BOOST_AUTO_TEST_CASE(calibrate_thresholds)
{
    auto thresholds = detail::generateThreshold(5, 19);
    auto z = 0;
    for (auto t : thresholds)
    {
        BOOST_TEST_MESSAGE(t << ", // z" << z++);
    }
}

BOOST_AUTO_TEST_CASE(removed_middle_test)
{
    /*
            x
           / \
          x   \
         /     \
        x       x
    */
    std::vector<util::Coordinate> coordinates = {
        5_lon + 5_lat, 12.5_lon + 12.6096298302_lat, 20_lon + 20_lat, 25_lon + 5_lat};

    for (unsigned z = 0; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 3);
        BOOST_CHECK_EQUAL(result[0], coordinates[0]);
        BOOST_CHECK_EQUAL(result[1], coordinates[2]);
        BOOST_CHECK_EQUAL(result[2], coordinates[3]);
    }
}

BOOST_AUTO_TEST_CASE(removed_middle_test_zoom_sensitive)
{
    /*
            x
           / \
          x   \
         /     \
        x       x
    */
    std::vector<util::Coordinate> coordinates = {
        5_lon + 5_lat, 6_lon + 6_lat, 20_lon + 20_lat, 25_lon + 5_lat};

    // Coordinate 6,6 should start getting included at Z9 and higher
    // Note that 5,5->6,6->10,10 is *not* a straight line on the surface
    // of the earth
    for (unsigned z = 0; z < 9; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 3);
    }
    // From 9 to max zoom, we should get all coordinates
    for (unsigned z = 9; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 4);
    }
}

BOOST_AUTO_TEST_CASE(remove_second_node_test)
{
    // derived from the degreeToPixel function
    const auto delta_pixel_to_delta_degree = [](const int pixel, const unsigned zoom)
    {
        const double shift = (1u << zoom) * 256;
        const double b = shift / 2.0;
        return pixel * 180. / b;
    };
    // For zoom level 0 all gets reduced to a line
    for (unsigned z = 1; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    {
        /*
             x
             | \
           x-x  x
                |
                x
        */
        double delta = delta_pixel_to_delta_degree(2, z);
        std::vector<util::Coordinate> input = {5_lon + 5_lat,
                                               5_lon + 5_lat + 1_lon * delta,
                                               10_lon + 10_lat,
                                               5_lon + 15_lat,
                                               5_lon + 15_lat + 1_lat * delta};
        BOOST_TEST_MESSAGE("Delta (" << z << "): " << delta);
        auto result = douglasPeucker(input, z);
        BOOST_CHECK_EQUAL(result.size(), 3);
        BOOST_CHECK_EQUAL(input[0], result[0]);
        BOOST_CHECK_EQUAL(input[2], result[1]);
        BOOST_CHECK_EQUAL(input[4], result[2]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
