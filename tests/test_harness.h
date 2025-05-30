#pragma once
#include <iostream>
#include <stdexcept>
#include <functional>

inline int run_test(const char* name, std::function<void()> test) {
    try {
        test();
        std::cout << name << ": OK" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << name << ": FAIL - " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << name << ": FAIL - unknown" << std::endl;
        return 1;
    }
}

#define EXPECT_TRUE(cond) do { if (!(cond)) throw std::runtime_error("Expectation failed: " #cond); } while(0)
#define EXPECT_EQ(a,b) do { if (!((a) == (b))) throw std::runtime_error("Expectation failed: " #a " == " #b); } while(0)
