**GomiEngine**

**ゴミダスのゲームを作る為のエンジンという意味で、ごみみたいなエンジンという訳ではない。**



world内でcreateEntity()を呼ぶとEntityを取得できます。

Entityに対してaddComponent()をして機能を追加可能。

ISystemを継承してworldのinitialize()で読み込めばそのworld内でupdateを呼べます。


PlayerControlComponentが用意されており、ActionID(論理キー)とInputKey(実際のキー)のキーマップを
入れれば操作可能。アクションの中身はplayerControlSystemで定義してください。

renderComponentでリソースを指定し、transformComponentを持っていれば画面に表示できます。