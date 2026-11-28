// Text の複数行の計算(TextLayout.h)のテスト。失敗したら行番号を出して 1 を返す
#include "TextLayout.h"
#include <cmath>
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

static bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

int main()
{
	auto adv = [](unsigned char) { return 10.0f; };

	// \n が無ければ 1 行。幅と高さは従来と同じ(幅 = 文字の送りの合計、高さ = fontSize)
	TextLayout one = layoutText("abc", 20, adv);
	CHECK(one.lines.size() == 1 && one.lines[0].text == "abc");
	CHECK(near(one.width, 30) && near(one.height, 20));

	// 2 行: 幅は長い方、高さは fontSize + 行の高さ(1.2 倍)
	TextLayout two = layoutText("ab\ncdef", 20, adv);
	CHECK(two.lines.size() == 2 && two.lines[0].text == "ab" && two.lines[1].text == "cdef");
	CHECK(near(two.lines[0].width, 20) && near(two.lines[1].width, 40));
	CHECK(near(two.width, 40) && near(two.lineHeight, 24) && near(two.height, 44));

	// 空の文字・連続した \n・末尾の \n も行として数える
	CHECK(layoutText("", 16, adv).lines.size() == 1);
	TextLayout gaps = layoutText("a\n\nb\n", 10, adv);
	CHECK(gaps.lines.size() == 4 && gaps.lines[1].text.empty() && gaps.lines[3].text.empty());
	CHECK(near(gaps.height, 10 + 3 * 12));

	// ASCII 以外は幅に数えない
	CHECK(near(layoutText("a\xE3\x81\x82", 10, adv).width, 10));

	if (failures == 0) std::cout << "TextLayoutTest: OK" << std::endl;
	return failures == 0 ? 0 : 1;
}
