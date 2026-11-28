#include "Random.h"
#include <chrono>

std::mt19937& Random::engine()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

uint32_t& Random::startSeedRef()
{
    static uint32_t value = 0;
    return value;
}

uint32_t Random::timeSeed()
{
    return static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
}
