= スライス機能つきスイッチ


== ネットワークを分割する

スライス機能つきスイッチは，OpenFlowネットワーク全体をスライスに分割し，
複数の L2 スイッチに仮想化します。
ちょうど，L2 スイッチを複数の VLAN に分けて使うイメージです。

スライスを 2 つ設定した例を @<img>{sliceable_switch} に示します。
同一のスライスに接続されたホスト同士はパケットをやりとりできますが，
異なるスライスに接続されたホスト同士ではできません。
このようにうまくスライスを設定することで，
アプリケーションやグループ別など用途に応じて独立した
ネットワークを作ることができます。

//image[sliceable_switch][スライスの作成]

=== スライスの実現
== 実行してみよう
 * 準備
 * REST API を使った設定
 * 試してみる
== VLAN (従来技術) との違い
=== 動作を比べてみる
=== 長所・短所
== まとめ/参考文献