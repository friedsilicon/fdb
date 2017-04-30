/*
 * RunAllTests.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: shiva
 */
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MockSupportPlugin.h>
#include <CppUTestExt/MockSupport.h>

int
main(int ac, char** av)
{
    // Add mock support to CppUTest
    MockSupportPlugin MockPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&MockPlugin);

    // Clean the memory inside the mock (otherwise you will see memory leaks in
    // the first testcase)
    mock().clear();

    return CommandLineTestRunner::RunAllTests(ac, av);
}
