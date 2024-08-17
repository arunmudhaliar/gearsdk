#include <gtest/gtest.h>
#include <sdktypes.hpp>

#undef __LOGTAG__
#define __LOGTAG__ "qh3client_test"

// Main function that runs all the tests
int main(int argc, char **argv) {
    init_gsdk();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
