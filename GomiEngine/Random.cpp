#include "Random.h"

std::mt19937& Random::engine()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}
