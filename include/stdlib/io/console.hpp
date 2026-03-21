#pragma once
#include <iostream>
#include <string>
#include <sstream>

auto FUNC_PRINT = [](auto&&... args) {
    std::cout << std::boolalpha;
    (std::cout << ... << args);
};

auto FUNC_PRINTLN = [](auto&&... args) {
    std::cout << std::boolalpha;
    (std::cout << ... << args) << std::endl;
};