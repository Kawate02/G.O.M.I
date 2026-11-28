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
    static uint32_t& startSeedRef();
public:
    /// <summary>
    /// 起動時の種を入れる(--replay は記録の種、--record は 1、--seed は N、無ければ時刻)。startSeed() で読める
    /// </summary>
    static void begin(uint32_t value)
    {
        startSeedRef() = value;
        engine().seed(value);
    }
    /// <summary>
    /// 起動時に begin で入れた種(ゲームが1ラン目の種に使い、結果の画面に出す)
    /// </summary>
    static uint32_t startSeed() { return startSeedRef(); }
    /// <summary>
    /// 起動した時刻から取った種
    /// </summary>
    static uint32_t timeSeed();
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
