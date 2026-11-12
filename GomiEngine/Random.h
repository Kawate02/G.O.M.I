#pragma once
#include <cstdint>
#include <random>
#include <sstream>
#include <string>

/// <summary>
/// ゲームで使う乱数。記録の再生では seed で種を決めて、毎回同じ並びにする
/// </summary>
class Random
{
private:
    // DLL をまたいで1つにするため、実体は Random.cpp に置く
    static std::mt19937& engine();
public:
    static void seed(uint32_t value)
    {
        engine().seed(value);
    }
    /// <summary>
    /// 今の状態を文字にする(巻き戻しで戻す)
    /// </summary>
    static std::string saveState()
    {
        std::ostringstream out;
        out << engine();
        return out.str();
    }
    static void loadState(const std::string& state)
    {
        std::istringstream in(state);
        in >> engine();
    }
    static int range(int n, int m)
    {
        std::uniform_int_distribution<> dist(n, m);
        return dist(engine());
    }
};
