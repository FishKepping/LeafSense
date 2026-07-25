#include <catch2/catch_test_macros.hpp>

#include "leafsense/roi/pixel_selection.h"

using namespace leafsense::roi;

TEST_CASE("New selection is empty")
{
    PixelSelection selection;

    REQUIRE(selection.empty());
    REQUIRE(selection.size() == 0);
}

TEST_CASE("Capacity is 64")
{
    PixelSelection selection;

    REQUIRE(selection.capacity() == 64);
}

TEST_CASE("Add one pixel")
{
    PixelSelection selection;

    REQUIRE(selection.add(5));

    REQUIRE(selection.size() == 1);
    REQUIRE_FALSE(selection.empty());

    REQUIRE(selection[0] == 5);
}

TEST_CASE("Add multiple pixels")
{
    PixelSelection selection;

    REQUIRE(selection.add(2));
    REQUIRE(selection.add(7));
    REQUIRE(selection.add(31));

    REQUIRE(selection.size() == 3);

    REQUIRE(selection[0] == 2);
    REQUIRE(selection[1] == 7);
    REQUIRE(selection[2] == 31);
}

TEST_CASE("Duplicate pixels are ignored")
{
    PixelSelection selection;

    REQUIRE(selection.add(10));

    REQUIRE_FALSE(selection.add(10));
    REQUIRE_FALSE(selection.add(10));

    REQUIRE(selection.size() == 1);
}

TEST_CASE("Contains reports correctly")
{
    PixelSelection selection;

    selection.add(12);

    REQUIRE(selection.contains(12));
    REQUIRE_FALSE(selection.contains(5));
}

TEST_CASE("Invalid pixel indices are rejected")
{
    PixelSelection selection;

    REQUIRE_FALSE(selection.add(64));
    REQUIRE_FALSE(selection.add(100));
    REQUIRE_FALSE(selection.add(255));

    REQUIRE(selection.empty());
}

TEST_CASE("Fill to capacity")
{
    PixelSelection selection;

    for (std::uint8_t i = 0; i < PixelSelection::MAX_PIXELS; ++i)
    {
        REQUIRE(selection.add(i));
    }

    REQUIRE(selection.size() == PixelSelection::MAX_PIXELS);

    REQUIRE_FALSE(selection.add(0));
    REQUIRE_FALSE(selection.add(64));
}

TEST_CASE("Clear removes all pixels")
{
    PixelSelection selection;

    selection.add(5);
    selection.add(9);
    selection.add(14);

    REQUIRE(selection.size() == 3);

    selection.clear();

    REQUIRE(selection.empty());
    REQUIRE(selection.size() == 0);
}

TEST_CASE("Iteration order matches insertion order")
{
    PixelSelection selection;

    selection.add(9);
    selection.add(1);
    selection.add(27);

    std::uint8_t expected[] = {9, 1, 27};

    std::size_t i = 0;

    for (auto pixel : selection)
    {
        REQUIRE(pixel == expected[i]);
        ++i;
    }

    REQUIRE(i == 3);
}

TEST_CASE("operator[] returns inserted values")
{
    PixelSelection selection;

    selection.add(20);
    selection.add(21);
    selection.add(22);

    REQUIRE(selection[0] == 20);
    REQUIRE(selection[1] == 21);
    REQUIRE(selection[2] == 22);
}