#pragma once
#include <cstdint>
#include <random>

/// <summary>
/// ゲームで使う乱数。記録の再生では seed で種を決めて、毎回同じ並びにする
/// </summary>
class Random
{
private:
    static std::mt19937& engine()
    {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }
public:
    static void seed(uint32_t value)
    {
        engine().seed(value);
    }
    static int range(int n, int m)
    {
        std::uniform_int_distribution<> dist(n, m);
        return dist(engine());
    }
};
