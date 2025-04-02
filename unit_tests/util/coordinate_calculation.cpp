#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/debug.hpp"
#include "util/web_mercator.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(coordinate_calculation_tests)

BOOST_AUTO_TEST_CASE(compute_angle)
{
    // Simple cases
    // North-South straight line
    Coordinate first{1_lon - 1_lat};
    Coordinate middle{1_lon + 0_lat};
    Coordinate end{1_lon + 1_lat};
    auto angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // North-South-North u-turn
    first = 1_lon + 0_lat;
    middle = 1_lon + 1_lat;
    end = 1_lon + 0_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 0);

    // East-west straight lines are harder, *simple* coordinates only
    // work at the equator.  For other locations, we need to follow
    // a rhumb line.
    first = 1_lon;
    middle = 2_lon;
    end = 3_lon;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // East-West-East u-turn
    first = 1_lon;
    middle = 2_lon;
    end = 1_lon;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 0);

    // 90 degree left turn
    first = 1_lon + 1_lat;
    middle = 0_lon + 1_lat;
    end = 0_lon + 2_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 90);

    // 90 degree right turn
    first = 1_lon + 1_lat;
    middle = 0_lon + 1_lat;
    end = 0_lon + 0_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 270);

    // Weird cases
    // Crossing both the meridians
    first = -1_lon - 1_lat;
    middle = 0_lon + 1_lat;
    end = 1_lon - 1_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_CLOSE(angle, 53.1, 0.2);

    // All coords in the same spot
    first = -1_lon - 1_lat;
    middle = -1_lon - 1_lat;
    end = -1_lon - 1_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot, then heading north-east
    first = -1_lon - 1_lat;
    middle = -1_lon - 1_lat;
    end = 1_lon + 1_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot, then heading west
    first = 1_lon + 1_lat;
    middle = 1_lon + 1_lat;
    end = 2_lon + 1_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot then heading north
    first = 1_lon + 1_lat;
    middle = 1_lon + 1_lat;
    end = 1_lon + 2_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Second two coords in the same spot
    first = 1_lon + 1_lat;
    middle = -1_lon - 1_lat;
    end = -1_lon - 1_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First and last coords on the same spot
    first = 1_lon + 1_lat;
    middle = -1_lon - 1_lat;
    end = 1_lon + 1_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 0);

    // Check the antimeridian
    first = 180_lon + 90_lat;
    middle = 180_lon + 0_lat;
    end = 180_lon - 90_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Tiny changes below our calculation resolution
    // This should be equivalent to having two points on the same spot.
    first = 0_lon + 0_lat;
    middle = 1_lon + 0_lat;
    end = 1_lon + 1_lon * std::numeric_limits<double>::epsilon() + 0_lat;
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);
}

BOOST_AUTO_TEST_CASE(invalid_values)
{
    // Invalid values for unsafe types
    BOOST_CHECK_THROW(
        coordinate_calculation::computeAngle(
            {UnsafeFloatLongitude{0}, UnsafeFloatLatitude{0}},
            {UnsafeFloatLongitude{1}, UnsafeFloatLatitude{0}},
            {UnsafeFloatLongitude{std::numeric_limits<double>::max()}, UnsafeFloatLatitude{0}}),
        boost::numeric::positive_overflow);
}

// Regression test for bug captured in #1347
BOOST_AUTO_TEST_CASE(regression_test_1347)
{
    Coordinate u{-100_lon + 10_lat};
    Coordinate v{-100.002_lon + 10.001_lat};
    Coordinate q{-100.001_lon + 10.002_lat};

    double d1 = coordinate_calculation::perpendicularDistance(u, v, q);

    double ratio;
    Coordinate nearest_location;
    double d2 = coordinate_calculation::perpendicularDistance(u, v, q, nearest_location, ratio);

    BOOST_CHECK_LE(std::abs(d1 - d2), 0.01);
}

BOOST_AUTO_TEST_CASE(regression_point_on_segment)
{
    //  ^
    //  |               t
    //  |
    //  |                 i
    //  |
    //  |---|---|---|---|---|---|---|--->
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |                           s
    FloatCoordinate input{55.995715_lon + 48.332711_lat};
    FloatCoordinate start{74.140427_lon - 180_lat};
    FloatCoordinate target{53.041084_lon + 77.21011_lat};

    FloatCoordinate nearest;
    double ratio;
    std::tie(ratio, nearest) = coordinate_calculation::projectPointOnSegment(start, target, input);

    FloatCoordinate diff{target.lon - start.lon, target.lat - start.lat};

    BOOST_CHECK_CLOSE(static_cast<double>(start.lon + FloatLongitude{ratio} * diff.lon),
                      static_cast<double>(nearest.lon),
                      0.1);
    BOOST_CHECK_CLOSE(static_cast<double>(start.lat + FloatLatitude{ratio} * diff.lat),
                      static_cast<double>(nearest.lat),
                      0.1);
}

BOOST_AUTO_TEST_CASE(point_on_segment)
{
    //  t
    //  |
    //  |---- i
    //  |
    //  s
    auto result_1 =
        coordinate_calculation::projectPointOnSegment(0_lon + 0_lat, 0_lon + 2_lat, 2_lon + 1_lat);
    auto reference_ratio_1 = 0.5;
    auto reference_point_1 = FloatCoordinate{0_lon + 1_lat};
    BOOST_CHECK_EQUAL(result_1.first, reference_ratio_1);
    BOOST_CHECK_EQUAL(result_1.second.lon, reference_point_1.lon);
    BOOST_CHECK_EQUAL(result_1.second.lat, reference_point_1.lat);

    //  i
    //  :
    //  t
    //  |
    //  |
    //  |
    //  s
    auto result_2 = coordinate_calculation::projectPointOnSegment(
        0._lon + 0._lat, 0_lon + 2_lat, 0_lon + 3_lat);
    auto reference_ratio_2 = 1.;
    auto reference_point_2 = FloatCoordinate{0_lon + 2_lat};
    BOOST_CHECK_EQUAL(result_2.first, reference_ratio_2);
    BOOST_CHECK_EQUAL(result_2.second.lon, reference_point_2.lon);
    BOOST_CHECK_EQUAL(result_2.second.lat, reference_point_2.lat);

    //  t
    //  |
    //  |
    //  |
    //  s
    //  :
    //  i
    auto result_3 = coordinate_calculation::projectPointOnSegment(
        0._lon + 0._lat, 0_lon + 2_lat, 0_lon - 1_lat);
    auto reference_ratio_3 = 0.;
    auto reference_point_3 = FloatCoordinate{0_lon + 0_lat};
    BOOST_CHECK_EQUAL(result_3.first, reference_ratio_3);
    BOOST_CHECK_EQUAL(result_3.second.lon, reference_point_3.lon);
    BOOST_CHECK_EQUAL(result_3.second.lat, reference_point_3.lat);

    //     t
    //    /
    //   /.
    //  /  i
    // s
    //
    auto result_4 = coordinate_calculation::projectPointOnSegment(
        0_lon + 0_lat, 1_lon + 1_lat, {FloatLongitude{0.5 + 0.1}, FloatLatitude{0.5 - 0.1}});
    auto reference_ratio_4 = 0.5;
    auto reference_point_4 = FloatCoordinate{0.5_lon + 0.5_lat};
    BOOST_CHECK_EQUAL(result_4.first, reference_ratio_4);
    BOOST_CHECK_EQUAL(result_4.second.lon, reference_point_4.lon);
    BOOST_CHECK_EQUAL(result_4.second.lat, reference_point_4.lat);
}

BOOST_AUTO_TEST_CASE(circleCenter)
{
    Coordinate a{-100._lon + 10._lat};
    Coordinate b{-100.002_lon + 10.001_lat};
    Coordinate c{-100.001_lon + 10.002_lat};

    auto result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, -100.0008333_lon + 10.0008333_lat);

    // Co-linear longitude
    a = -100._lon + 10._lat;
    b = -100.001_lon + 10.001_lat;
    c = -100.001_lon + 10.002_lat;
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, -99.9995_lon + 10.0015_lat);

    // Co-linear longitude, impossible to calculate
    a = -100.001_lon + 10._lat;
    b = -100.001_lon + 10.001_lat;
    c = -100.001_lon + 10.002_lat;
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(!result);

    // Co-linear latitude, this is a real case that failed
    a = -112.096234_lon + 41.147101_lat;
    b = -112.096606_lon + 41.147101_lat;
    c = -112.096419_lon + 41.147259_lat;
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, -112.09642_lon + 41.147071_lat);

    // Co-linear latitude, variation
    a = -112.096234_lon + 41.147101_lat;
    b = -112.096606_lon + 41.147259_lat;
    c = -112.096419_lon + 41.147259_lat;
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, -112.096513_lon + 41.146962_lat);

    // Co-linear latitude, impossible to calculate
    a = -112.096234_lon + 41.147259_lat;
    b = -112.096606_lon + 41.147259_lat;
    c = -112.096419_lon + 41.147259_lat;
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(!result);

    // Out of bounds
    a = -112.096234_lon + 41.147258_lat;
    b = -112.106606_lon + 41.147259_lat;
    c = -113.096419_lon + 41.147258_lat;
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(!result);
}

// For overflow issue #3483, introduced in 68ee4eab61548. Run with -fsanitize=integer.
BOOST_AUTO_TEST_CASE(squaredEuclideanDistance)
{
    // Overflow happens when left hand side values are smaller than right hand side values,
    // then `lhs - rhs` will be negative but stored in a uint64_t (wraps around).

    Coordinate lhs{-180_lon - 90_lat};
    Coordinate rhs{+180_lon + 90_lat};

    const auto result = coordinate_calculation::squaredEuclideanDistance(lhs, rhs);

    BOOST_CHECK_EQUAL(result, 162000000000000000ull);
}

BOOST_AUTO_TEST_CASE(vertical_regression)
{
    // check a vertical line for its bearing
    std::vector<Coordinate> coordinates;
    for (std::size_t i = 0; i < 100; ++i)
        coordinates.push_back(1_lat * (i / 100.0));

    const auto regression =
        util::coordinate_calculation::leastSquareRegression(coordinates.begin(), coordinates.end());
    const auto is_valid =
        util::angularDeviation(
            util::coordinate_calculation::bearing(regression.first, regression.second), 0) < 2;
    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(sinus_curve)
{
    // create a full sinus curve, sampled in 3.6 degree
    std::vector<Coordinate> coordinates;
    for (std::size_t i = 0; i < 360; ++i)
        coordinates.emplace_back(
            FloatLongitude{i / 360.0},
            FloatLatitude{sin(util::coordinate_calculation::detail::degToRad(i / 360.0))});

    const auto regression =
        util::coordinate_calculation::leastSquareRegression(coordinates.begin(), coordinates.end());
    const auto is_valid =
        util::angularDeviation(
            util::coordinate_calculation::bearing(regression.first, regression.second), 90) < 2;

    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(parallel_lines_slight_offset)
{
    std::vector<Coordinate> coordinates_lhs;
    for (std::size_t i = 0; i < 100; ++i)
        coordinates_lhs.emplace_back(util::FloatLongitude{(50 - (rand() % 101)) / 100000.0},
                                     util::FloatLatitude{i / 100000.0});
    std::vector<Coordinate> coordinates_rhs;
    for (std::size_t i = 0; i < 100; ++i)
        coordinates_rhs.emplace_back(util::FloatLongitude{(150 - (rand() % 101)) / 100000.0},
                                     util::FloatLatitude{i / 100000.0});

    const auto are_parallel = util::coordinate_calculation::areParallel(coordinates_lhs.begin(),
                                                                        coordinates_lhs.end(),
                                                                        coordinates_rhs.begin(),
                                                                        coordinates_rhs.end());
    BOOST_CHECK(are_parallel);
}

BOOST_AUTO_TEST_CASE(consistent_invalid_bearing_result)
{
    const Coordinate pos1{0._lon + 0._lat};
    const Coordinate pos2{5._lon + 5._lat};
    const Coordinate pos3{-5._lon - 5._lat};

    BOOST_CHECK_EQUAL(0., util::coordinate_calculation::bearing(pos1, pos1));
    BOOST_CHECK_EQUAL(0., util::coordinate_calculation::bearing(pos2, pos2));
    BOOST_CHECK_EQUAL(0., util::coordinate_calculation::bearing(pos3, pos3));
}

// Regression test for bug captured in #3516
BOOST_AUTO_TEST_CASE(regression_test_3516)
{
    const Coordinate u{-73.989687_lon + 40.752288_lat};
    const Coordinate v{-73.990134_lon + 40.751658_lat};
    const Coordinate q{-73.99039_lon + 40.75171_lat};

    BOOST_CHECK_EQUAL(Coordinate{web_mercator::toWGS84(web_mercator::fromWGS84(u))}, u);
    BOOST_CHECK_EQUAL(Coordinate{web_mercator::toWGS84(web_mercator::fromWGS84(v))}, v);

    double ratio;
    Coordinate nearest_location;
    coordinate_calculation::perpendicularDistance(u, v, q, nearest_location, ratio);

    BOOST_CHECK_EQUAL(ratio, 1.);
    BOOST_CHECK_EQUAL(nearest_location, v);
}

BOOST_AUTO_TEST_CASE(computeArea)
{
    using osrm::util::coordinate_calculation::computeArea;

    //
    auto rhombus = std::vector<Coordinate>{.00_lon + .00_lat,
                                           .01_lon + .01_lat,
                                           .02_lon + .00_lat,
                                           .01_lon - .01_lat,
                                           .00_lon + .00_lat};

    BOOST_CHECK_CLOSE(2 * 1109.462 * 1109.462, computeArea(rhombus), 1e-3);

    // edge cases
    auto self_intersection = std::vector<Coordinate>{.00_lon + .00_lat,
                                                     .00_lon + .02_lat,
                                                     .01_lon + .01_lat,
                                                     .02_lon + .00_lat,
                                                     .02_lon + .02_lat,
                                                     .01_lon + .01_lat,
                                                     .00_lon + .00_lat};
    BOOST_CHECK(computeArea(self_intersection) < 1e-3);
    BOOST_CHECK_CLOSE(0, computeArea({}), 1e-3);
}

BOOST_AUTO_TEST_SUITE_END()
