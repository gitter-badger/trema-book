= ルーティングスイッチで複数台のスイッチを制御する

筆者は、ネットワークの仕事に十余年も関わっているためか、多数のスイッチ、ルータから構成される大規模なネットワークの構成図を見ると心が踊ります。``どんなプロトコルで動いているのか?'' を知りたくなったり、``いつか自分でも動かしてみたい!'' と思ったりします。ここまでの章では一台の OpenFlow スイッチを制御する方法について学んできましたが、もちろん、OpenFlow は、複数台のスイッチからなるネットワークでも使えます。

そんなネットワークを制御するためには、スイッチ同士の接続情報 (トポロジー) を検出する必要があります。OpenFlow を使うことで、ネットワークのトポロジー構成を検出できます。検出したトポロジーを使うことで、ネットワーク上の最短パスでパケットを転送することができます。

本章では、一例として、ルーティングスイッチを紹介します。ルーティングスイッチが、複数の OpenFlow スイッチからなるネットワークの制御をどのように実現しているか見ていきましょう。

== 複数のスイッチを扱う

ルーティングスイッチは、@<chap>{learning_switch} で扱ったラーニングスイッチと同様に普通の L2 スイッチとして動作するアプリケーションです。ラーニングスイッチは、1 台の OpenFlow スイッチが 1 台の L2 スイッチとして動作するよう制御するコントローラでした。それに対してルーティングスイッチを使えば、複数台の OpenFlow スイッチが連動して、一つの大きな L2 スイッチとして動作します (@<img>{routing_switch})。

//image[routing_switch][ルーティングスイッチ]

=== ルーティングスイッチの動作

@<img>{flow_mod} のネットワーク中で、ホスト 1 からホスト 4 へとパケットを届けるためにはどうすればよいでしょうか？ホスト 1 が接続するスイッチ 1 から、スイッチ 5、スイッチ 6 の順にパケットが送られ、ホスト 4 へと届くようにフローを設定すればよさそうです。

//image[flow_mod][フローの設定]

//image[behavior][ルーティングスイッチの動作][scale=0.4]

それでは、コントローラの動作を、@<img>{behavior} を使って、順に見ていきましょう。

 1. ルーティングスイッチは、 スイッチが受信したパケットを Packet In メッセージで受け取ります。@<img>{flow_mod} の例では、ホスト 1 からホスト 4 へのパケットが、スイッチ 1 から送られてきます。
 2. 次に FDB を検索し、宛先であるホストが接続されているスイッチ、ポートを検索します。
 3. Packet In を送ってきたスイッチから出口となるスイッチまでの最短パスを計算します。この場合、スイッチ 1 からスイッチ 6 の最短パスとして、スイッチ 5 を経由するパスが得られます。
 4. 最短パスに沿ってパケットが転送されるよう、各スイッチそれぞれに Flow Mod メッセージを送り、フローを設定します。
 5. その後、Packet In でコントローラに送られたパケットを 2 で決定したスイッチのポートから出力するよう、Packet Out メッセージを送ります。

基本的な動作はラーニングスイッチと同じですが、パケットを受信したスイッチと違うスイッチまでパケットを届けなければならない点が異なっています。そのためには、入口となるスイッチから出口となるスイッチまでの最短パスを見つける必要があります。

#@warn(ラーニングスイッチの章と整合を取るように。比較して理解しやすいように)

=== 最短パスを計算する

それでは、スイッチ 1 からスイッチ 6 までのパスは、どのように計算すればよいでしょうか？ルーティングスイッチでは、ダイクストラ法というアルゴリズムを用いて、二つのスイッチ間の最短パスを求めています。ダイクストラ法は、ネットワークの世界では非常にポピュラーなアルゴリズムで、OSPF や IS-IS 等の L3 の経路制御プロトコルで用いられています。ここでは、ダイクストラ法を用いて最短パスを見つける方法について、簡単な説明を@<img>{dijkstra}を用いて行います。

//image[dijkstra][最短パスの計算]

まず始点となるスイッチ 1 に着目します (@<img>{dijkstra} (a))。

初めに、スイッチ 1 から 1 ホップで行けるスイッチを見つけ出します。これはスイッチ 1 に接続するリンクの先に繋がっているスイッチを、すべてピックアップすればよさそうです。その結果、スイッチ 2 とスイッチ 5 が見つけ出せます (@<img>{dijkstra} (b))。

同じようにして、今度はスイッチ 1 から 2 ホップで行けるスイッチを見つけ出します。これは、前のステップで見つけたスイッチ 2 とスイッチ 5 から、さらにその先 1 ホップで行けるスイッチを調べればよさそうです。このようにすることで、今度はスイッチ 3, 4, 6 が見つかります(@<img>{dijkstra} (c))。

終点であるスイッチ 6 が、スイッチ 5 の先で見つかりましたので、最短パスは最終的にスイッチ 1 → スイッチ 5 → スイッチ 6 であることがわかります。

=== トポロジーを検出する

ここでは、コントローラが、スイッチ同士がどのように接続しているかという情報 (トポロジー情報) を調べる方法について紹介します。ダイクストラ法には、``リンクの先に繋がっているスイッチを調べる'' というステップがありました。このステップを実行するためには、コントローラはトポロジー情報を知らなければなりません。

//image[lldps0][LLDP によるリンク発見][scale=0.4]

実際に、コントローラがスイッチ間のリンクを発見するにはどのようにすればよいでしょうか？例えば、@<img>{lldps0} のように、スイッチ 0x1 のポート 4 とスイッチ 0x2 のポート 1 が接続されていたとします。このリンクを発見するために、コントローラは以下の手順を行います。

 1. まずコントローラは、接続関係を調べたいスイッチの Datapath ID 0x1 とポート番号 4 を埋め込んだ Link Layer Discovery Protocol (LLDP) パケットを作ります。
 2. ポート 4 から出力するというアクションを含む Packet Out メッセージを作り、先ほど作った LLDP パケットをスイッチ 0x1 へと送ります。
 3. Packet Out を受け取ったスイッチはアクションに従い、LLDP パケットを指定されたポート 4 から出力します。その結果、LLDP パケットは、ポート 4 の先につながるスイッチ 0x2 へと到着します。
 4. LLDP パケットを受け取ったスイッチ 0x2 は、自身のフローテーブルを参照し、パケットの処理方法を調べます。このとき LLDP に対するフローはあえて設定していないため、今回受信した LLDP パケットは、Packet In としてコントローラまで戻されます。
 5. コントローラは、受け取った Packet In メッセージを解析することで、リンクの発見を行います。今回どのスイッチ間のリンクが発見されたのかは、@<table>{parse} のように解析します。

//table[parse][解析方法]{
情報			取得方法
-------------------------------------------------------------------
リンク上流側スイッチ 0x1	Packet In で送られてくる LLDP パケットに含まれる
リンク上流側ポート番号 4	Packet In で送られてくる LLDP パケットに含まれる
リンク下流側スイッチ 0x2	Packet In の送信元
リンク下流側ポート番号 1	Packet In 中における LLDP の受信ポート
//}

このように、Packet Out で送られた LLDP パケットは、リンクを通過し、隣のスイッチから Packet In でコントローラへと戻ってきます。この一連の動作によりコントローラは、リンクを発見することができます。

この方法自体は、仕様で特に規定されているわけではありません。しかし、上記の 3, 4 のステップで、それぞれのスイッチは OpenFlow スイッチの仕様として定められた動作を行なっているだけです。つまりこの方法は、特別な動作を必要とせず、仕様に従っていればどのような OpenFlow スイッチでも使えるため、OpenFlow ではよく用いられています。Packet Out と Packet In を用いた ``OpenFlow ならでは'' のリンク発見方法だと言えます。

このリンク発見方法をネットワーク中のすべてのスイッチのすべてのポートに順に適用していけば、ネットワーク全体のスイッチの接続関係、つまりトポロジーを知ることができます。例えば @<img>{lldp0} のような 3 台の OpenFlow スイッチからなるネットワークにおいて、どのようにトポロジーが検出されるかを見ていきましょう。@<img>{lldp0} は各 OpenFlow スイッチがコントローラに接続した直後の状態です。この段階で、コントローラは、スイッチ同士がどのように接続されているかを知りません。

//image[lldp0][トポロジー検出前][scale=0.4]

まず スイッチ 0x1 から調べていきます。はじめに Features Request メッセージを送ることで、スイッチ 0x1 が持つポート一覧を取得します。そして、それぞれのポートに対して、前述のリンク発見手順を行います(@<img>{lldp1})。その結果、スイッチ 1 からスイッチ 2 およびスイッチ 3 へと至るリンクがそれぞれ発見できます。

//image[lldp1][スイッチ 0x1 のリンク発見][scale=0.4]

あとは同様の手順を、ネットワーク中の各スイッチに対して順に行なっていくだけです。スイッチ 2, 3 に接続するリンクを順に調べていくことで、ネットワークの完全なトポロジー情報を知ることができます (@<img>{lldp2})。

//image[lldp2][スイッチ 0x2, 0x3 のリンク発見]

== 実行してみよう

ルーティングスイッチは、Trema Apps の一部として、Github 上で公開されています。公開されているルーティングスイッチを使いながら、実際の動作を見ていきましょう。

=== 準備

Trema Apps のソースコードは、@<tt>{https://github.com/trema/apps/} にあります。まずは、@<tt>{git} を使って、ソースコードを取得しましょう。

//cmd{
% git clone https://github.com/trema/apps.git
//}

@<chap>{trema_architecture} で紹介したように、Trema Apps にはさまざまなアプリケーションが含まれています。そのうち、今回使用するのは @<tt>{topology} と @<tt>{routing_switch} です。@<tt>{topology} には、トポロジー検出を担当するモジュール @<tt>{topology_discovery} と検出したトポロジーを管理するモジュール @<tt>{topology} が含まれています。また @<tt>{routing_switch} には、ルーティングスイッチの本体が含まれています。この二つを順に @<tt>{make} してください。

//cmd{
% (cd apps/topology/; make)
% (cd apps/routing_switch; make)
//}

=== ルーティングスイッチを動かす

それでは、ルーティングスイッチを動かしてみましょう。今回は Trema のネットワークエミュレータ機能を用いて、@<img>{fullmesh} ネットワークを作ります。

//image[fullmesh][ネットワーク構成]

このネットワーク構成を作るためには、@<list>{conf} のように記述を行う必要があります。

//list[conf][@<tt>{routing_switch_fullmesh.conf}]{
vswitch {
  datapath_id "0xe0"
}

vswitch {
  datapath_id "0xe1"
}

vswitch {
  datapath_id "0xe2"
}

vswitch {
  datapath_id "0xe3"
}

vhost ("host1") {
  ip "192.168.0.1"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:01"
}

vhost ("host2") {
  ip "192.168.0.2"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:02"
}

vhost ("host3") {
  ip "192.168.0.3"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:03"
}

vhost ("host4") {
  ip "192.168.0.4"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:04"
}

link "0xe0", "host1"
link "0xe1", "host2"
link "0xe2", "host3"
link "0xe3", "host4"
link "0xe0", "0xe1"
link "0xe0", "0xe2"
link "0xe0", "0xe3"
link "0xe1", "0xe2"
link "0xe1", "0xe3"
link "0xe2", "0xe3"

run {
  path "../apps/topology/topology"
}

run {
  path "../apps/topology/topology_discovery"
}

run {
  path "../apps/routing_switch/routing_switch"
}

event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
filter :lldp => "topology_discovery", :packet_in => "routing_switch"
//}

このファイルは、@<tt>{routing_switch_fullmesh.conf} として、ソースコード一式の中に用意されています。今回はこのファイルを使って、以下のように起動してください。

//cmd{
% trema run -c ./apps/routing_switch/routing_switch_fullmesh.conf -d
//}

=== 見つけたリンクを表示する

@<tt>{topology} ディレクトリには、検出したトポロジーを表示するコマンドが用意されていますので、使ってみましょう。以下のように実行してください。

//cmd{
% ./apps/topology/show_topology -D
vswitch {
  datapath_id "0xe0"
}

vswitch {
  datapath_id "0xe2"
}

vswitch {
  datapath_id "0xe3"
}

vswitch {
  datapath_id "0xe1"
}

link "0xe3", "0xe2"
link "0xe1", "0xe0"
link "0xe3", "0xe0"
link "0xe2", "0xe1"
link "0xe2", "0xe0"
link "0xe3", "0xe1"
//}

ルーティングスイッチの起動時に指定した設定ファイル (@<list>{conf}) やネットワーク構成 (@<img>{fullmesh}) と比較してみましょう。@<tt>{topology_discovery} モジュールが検出できるのは、スイッチ間のリンクのみです。仮想ホストとスイッチ間のリンクは検出できないため、@<tt>{show_topology} の検出結果には表示されないことに注意しましょう。

=== パケットを送り、フローが設定されているかを確認する
   
次に、仮想ホストからパケットを送り、フローが設定されることを確認しましょう。

//cmd{
% trema send_packets --source host1 --dest host2
% trema send_packets --source host2 --dest host1
//}

ルーティングスイッチ起動直後は、まだ MAC アドレスの学習を行なっていないので、host1 から host2 へとパケットを送っただけではフローは設定されません。この段階で host1 の MAC アドレスを学習したので、host2 から host1 へと送った段階でフローが設定されます。

それでは、どのようなフローが設定されたかを見てみます。設定されているフローの確認は、@<tt>{trema dump_flows [表示したいスイッチの Datapath ID]} でできます。@<tt>{0xe0} から @<tt>{0xe1} まで順に表示してみましょう。

//cmd{
% trema dump_flows 0xe0
NXST_FLOW reply (xid=0x4):
 cookie=0x3, duration=41s, table=0, n_packets=0, n_bytes=0, idle_timeout=62, \
 ...	     		   	    		 	    		     \
 dl_src=00:00:00:01:00:02,dl_dst=00:00:00:01:00:01,nw_src=192.168.0.2,	     \
 nw_dst=192.168.0.1,nw_tos=0,tp_src=1,tp_dst=1 actions=output:3
% ./trema dump_flows 0xe1
NXST_FLOW reply (xid=0x4):
 cookie=0x3, duration=42s, table=0, n_packets=0, n_bytes=0, idle_timeout=61, \
 ...	     		   	    		 	    		     \
 dl_src=00:00:00:01:00:02,dl_dst=00:00:00:01:00:01,nw_src=192.168.0.2,	     \
 nw_dst=192.168.0.1,nw_tos=0,tp_src=1,tp_dst=1 actions=output:3
% ./trema dump_flows 0xe2
NXST_FLOW reply (xid=0x4):
% ./trema dump_flows 0xe3
NXST_FLOW reply (xid=0x4):
//}

@<tt>{0xe0} と @<tt>{0xe1} のスイッチそれぞれに、@<tt>{dl_src} が host2 の MAC アドレス、@<tt>{dl_dst} が host1 の MAC アドレスがマッチングルールのフローが設定されていることが分かります。一方で @<tt>{0xe2} と @<tt>{0xe3} のスイッチには、フローがありません。@<img>{fullmesh} をもう一度見てください。host2 から host1 への最短パスは@<tt>{0xe1} → @<tt>{0xe0} なので、この二つのスイッチにきちんとフローが設定されています。

== 利点・欠点

本章のはじめで説明したように、ルーティングスイッチは、OpenFlow ネットワークを普通の L2 スイッチとして動作させるコントローラアプリケーションです。普通の L2 スイッチとして動かすだけならば、わざわざ OpenFlow を使わなくてもよいのでは？と思うかもしれません。ここでは、OpenFlow で実現した場合の利点と欠点について、少し考えてみたいと思います。

=== スパニングツリーがいらない

通常の L2 スイッチで構成されたネットワークでは、パケットがループすることを防ぐために、スパニングツリープロトコルが用いられています。例えば、@<img>{spt} (a) のようなループを含むネットワークでスパニングツリープロトコルを使うと、スイッチ 2 とスイッチ 3 間のリンクをブロックされ、ループが解消されます。この時、例えばホスト 2 からホスト 3 へのパケットは、ブロッキングリンクを通過できないため、スイッチ 1 を経由して転送されます。

//image[spt][ループを含むネットワークでのパケット転送]

ルーティングスイッチでは、Packet In メッセージが送られてきた時に、そのパケットに対するフローを各スイッチに設定するという動作を行います。この時、Packet In を受信したスイッチから、出口となるスイッチまでの最短パスを計算し、そのパス上の各スイッチに対してフローの設定を行います。このため、ループを含むトポロジーであっても問題なく動作します。ブロックリンクを作る必要がないため、スパニングツリーを使う場合と比べて、ネットワーク中のリンクを有効に使うことができます(@<img>{spt}(b))。

=== いろいろなパス選択アルゴリズムを使える

前に述べた通り、パケットの通過するパスの決定は、OpenFlow コントローラであるルーティングスイッチが行ないます。ルーティングスイッチでは、ダイクストラ法で用いて計算した最短パスを用いると説明しました。しかし、このパス決定アルゴリズムを入れ替えることで、@<img>{multipath} のようにフロー毎に異なるパスを設定することもできます。

//image[multipath][マルチパス]

IETF で標準化が行われている TRILL (Transparent Interconnect of Lots of Links) や IEEE で標準化が行われている SPB (Shortest Path Bridges) でも、同様にマルチパスを扱うことができます。ただし、コストが最短となるパスが複数存在する場合のみに限定されます@<fn>{ecmp}。なぜなら、各スイッチの自律分散で動作しているからです。各スイッチが、宛先に向かって最短となるパスに沿ってパケットの転送を行うことで、パケットはループすることなく宛先まで届きます。

一方、ルーティングスイッチでは、OpenFlow コントローラが一括してパスを決定し、各スイッチにフローが設定されます。そのため、@<img>{multipath} のように最短ではないパスを用いることもできます。

//footnote[ecmp][このようなパスを、イコールコストマルチパス (Equal Cost Multipath) と呼びます。]

== まとめ

本章で学んだことは、次の 3 つです。

 * 複数のスイッチからなる大規模ネットワークを扱うことができる、ルーティングスイッチがどのように動作するかを見てみました。
 * トポロジーを検出する仕組みを学びました。エミュレータ機能上のネットワークでの動作を通して、LLDP を使ったトポロジー検出の動作を理解しました。
 * 最短パスを計算する方法について学びました。

また、ルーティングスイッチの動作を参考に、L2 スイッチ機能を OpenFlow を使って実現する利点と欠点についても見てみました。次の章では、より本格的な Trema のアプリケーションとして、ルーティングスイッチを発展させたスライス機能付きスイッチについて説明します。

== 参考文献

: マスタリング TCP/IP 応用編 (Philip Miller 著、オーム社)
  L3 の経路制御プロトコルについて詳しく説明されています。本章で扱ったダイクストラ法を用いた経路制御プロトコルである OSPF についても説明がされているので、ルーティングスイッチとの違いを比べてみるのも面白いでしょう。

: 最短経路の本 レナのふしぎな数学の旅 (P・グリッツマンら著、シュプリンガー・ジャパン株式会社)
  最短経路を題材にした読み物で、難しい理論を知らなくても読むことができます。本章でも最短パスの計算を簡単に紹介しましたが、この本を読めばより理解が深まるでしょう。

