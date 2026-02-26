#pragma once
#include <random>

class Random
{
public:
    static int range(int n, int m)
    {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> dist(n, m);
        return dist(gen);
    }
};