#include <iostream>
#include "gtest/gtest.h"

int main(int argc, char* argv[])
{
    std::cout << "Hello Tester!" << std::endl;

    EXPECT_EQ(5, 5);

    return 0;
}