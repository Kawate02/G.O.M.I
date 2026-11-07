// ComponentArray の単体テスト。失敗したら行番号を出して 1 を返す
#include "ComponentArray.h"
#include <iostream>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl; failures++; } } while (0)

int main()
{
	// 小さいIDを後から追加しても、大きいIDの対応が消えない
	{
		ComponentArray<int> a;
		a.addComponent({ 5, 0 }, 50);
		a.addComponent({ 2, 0 }, 20);
		CHECK(a.hasComponent({ 5, 0 }));
		CHECK(a.getComponent({ 5, 0 }) == 50);
		CHECK(a.getComponent({ 2, 0 }) == 20);
	}
	// 同じEntityへの二重追加は上書きで、要素数は増えない
	{
		ComponentArray<int> a;
		a.addComponent({ 1, 0 }, 1);
		a.addComponent({ 1, 0 }, 2);
		CHECK(a.getComponentCount() == 1);
		CHECK(a.getComponent({ 1, 0 }) == 2);
	}
	// 途中を消すと末尾が詰められ、世代も一緒に動く
	{
		ComponentArray<int> a;
		a.addComponent({ 0, 0 }, 0);
		a.addComponent({ 1, 3 }, 1);
		a.addComponent({ 2, 7 }, 2);
		a.removeComponent({ 0, 0 });
		CHECK(a.getComponentCount() == 2);
		CHECK(!a.hasComponent({ 0, 0 }));
		CHECK(a.hasComponent({ 2, 7 }));
		CHECK(a.getComponent({ 2, 7 }) == 2);
		CHECK(a.hasComponent({ 1, 3 }));
		for (int i = 0; i < a.getComponentCount(); i++)
		{
			Entity e = { a.getEntityId(i), a.getGeneration(i) };
			CHECK(a.hasComponent(e));
		}
	}
	// 古い世代では見えず、同じIDの新しい世代で追加すると枠を再利用する
	{
		ComponentArray<int> a;
		a.addComponent({ 3, 0 }, 30);
		CHECK(!a.hasComponent({ 3, 1 }));
		a.addComponent({ 3, 1 }, 31);
		CHECK(a.getComponentCount() == 1);
		CHECK(!a.hasComponent({ 3, 0 }));
		CHECK(a.getComponent({ 3, 1 }) == 31);
	}
	// 持っていない時と範囲外のIDは安全に false / ダミー
	{
		ComponentArray<int> a;
		CHECK(!a.hasComponent({ 100000, 0 }));
		CHECK(a.getComponent({ 4, 0 }) == 0);
		a.removeComponent({ 4, 0 });
		CHECK(a.getComponentCount() == 0);
	}

	if (failures == 0) std::cout << "ComponentArrayTest: all passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
