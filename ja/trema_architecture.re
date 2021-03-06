= Trema のアーキテクチャ

//lead{
今まで触れてこなかったTremaの内部構造を詳しく見てみましょう。他の章に比べ少し難易度が高いので、取っつきやすいように対話形式にしました。
//}

//indepimage[yonaka][][width=7cm]

== OpenFlow先生が家にやってきた

3月にしてはあたたかい夜更け、@<ruby>{友太郎,ゆうたろう}君が自宅の書斎にていつものようにコーディングを楽しんでいたときのこと。ふいに、玄関の外から聞き覚えのある声。@<br>{}

//noindent
@<em>{謎の声}：「もしもし、友太郎君はおるかの？」@<br>{}
@<em>{友太郎君}：「あっ! @<ruby>{取間,とれま}先生じゃないですか。こんな夜中にどうされたんですか？」@<br>{}
@<em>{取間先生}：「三軒茶屋で飲んでいたら、愛用のサンダルがなくなってしまってのう。必死に探したんじゃが、どうやら別の客が間違えて履いて行ってしまったようじゃ。そのおかげで終電までなくしてしもうたわい! ワッハッハ」@<br>{}

//noindent
この取間先生、飲んでいて陽気ですが足元を見るとたしかに裸足です。それに着物の@<ruby>{裾,すそ}もやたらと汚れています。どうやら酔った勢いだけで、友太郎君の住む武蔵小杉まではるばる歩いてきてしまったようです。@<br>{}

//noindent
@<em>{取間先生}：「そこで悪いのじゃが友太郎君、今夜はこのみじめな老人を泊めてくれんか？お礼と言ってはなんじゃが、始発まで時間はたっぷりあるからTremaについてまだ話してなかったことをすべて教えよう。友太郎君は最近Tremaでいろいろとアプリを書いているようだし、いろいろ聞きたい事もあるだろうと思ってな」@<br>{}
@<em>{友太郎君}：「ぜひそのお話聞きたいです! さあさどうぞお上がりください」

== @<tt>{trema run} の裏側

//noindent
@<em>{取間先生}：「では基本的な話から。Tremaでコントローラを動かすには@<tt>{trema run}じゃったな(@<chap>{openflow_framework_trema})。このコマンド、実は裏でいろんなプロセスを起動しているんじゃ。具体的に何をやっているか、@<tt>{trema run}に@<tt>{-v}オプションをつけると見ることができるぞ。さっそく、サンプルに付いているラーニングスイッチ(@<chap>{learning_switch})を試しに起動してみてごらん」

//cmd{
% trema -v run learning-switch.rb -c learning_switch.conf
.../switch_manager --daemonize --port=6633 \
  -- port_status::LearningSwitch packet_in::LearningSwitch \
  state_notify::LearningSwitch vendor::LearningSwitch
sudo ip link delete trema0-0 2>/dev/null
sudo ip link delete trema1-0 2>/dev/null
sudo ip link add name trema0-0 type veth peer name trema0-1
sudo sysctl -w net.ipv6.conf.trema0-0.disable_ipv6=1 >/dev/null 2>&1
sudo sysctl -w net.ipv6.conf.trema0-1.disable_ipv6=1 >/dev/null 2>&1
sudo /sbin/ifconfig trema0-0 up
sudo /sbin/ifconfig trema0-1 up
sudo ip link add name trema1-0 type veth peer name trema1-1
sudo sysctl -w net.ipv6.conf.trema1-0.disable_ipv6=1 >/dev/null 2>&1
sudo sysctl -w net.ipv6.conf.trema1-1.disable_ipv6=1 >/dev/null 2>&1
sudo /sbin/ifconfig trema1-0 up
sudo /sbin/ifconfig trema1-1 up
sudo .../phost/phost -i trema0-1 -p .../trema/tmp/pid -l .../trema/tmp/log -D
sudo .../phost/cli -i trema0-1 set_host_addr --ip_addr 192.168.0.1 \
  --ip_mask 255.255.0.0 --mac_addr 00:00:00:01:00:01
sudo .../phost/phost -i trema1-1 -p .../trema/tmp/pid -l .../trema/tmp/log -D
sudo .../phost/cli -i trema1-1 set_host_addr --ip_addr 192.168.0.2 \
  --ip_mask 255.255.0.0 --mac_addr 00:00:00:01:00:02
sudo .../openvswitch/bin/ovs-openflowd --detach --out-of-band --fail=closed \
  --inactivity-probe=180 --rate-limit=40000 --burst-limit=20000 \
  --pidfile=.../trema/tmp/pid/open_vswitch.lsw.pid --verbose=ANY:file:dbg \
  --verbose=ANY:console:err --log-file=.../trema/tmp/log/openflowd.lsw.log \
  --datapath-id=0000000000000abc --unixctl=.../trema/tmp/sock/ovs-openflowd.lsw.ctl \
   --ports=trema0-0,trema1-0 netdev@vsw_0xabc tcp:127.0.0.1:6633
sudo .../phost/cli -i trema0-1 add_arp_entry --ip_addr 192.168.0.2 \
  --mac_addr 00:00:00:01:00:02
sudo .../phost/cli -i trema1-1 add_arp_entry --ip_addr 192.168.0.1 \
  --mac_addr 00:00:00:01:00:01
//}

//noindent
@<em>{友太郎君}：「うわっ! なんだかいっぱい字が出てきましたね」@<br>{}
@<em>{取間先生}：「この出力をよーく見れば、@<tt>{trema run}コマンドが裏でどんなことをしてくれているのかスグわかるのじゃ。たとえばログの最初を見ると、@<tt>{switch_manager}というプロセスが起動されておるな？Tremaではこのプロセスがスイッチと最初に接続するんじゃ」

#@warn(port_status:: とかの引数が Switch Manager に渡ることの説明)

== Switch Manager

Switch Manager(@<tt>{[trema]/objects/switch_manager/switch_manager})はスイッチからの接続要求を待ち受けるデーモンです。スイッチとの接続を確立すると、子プロセスとしてSwitch Daemonプロセスを起動し、スイッチとの接続をこのプロセスへ引き渡します(@<img>{switch_manager_daemon})。@<br>{}

//image[switch_manager_daemon][スイッチからの接続の受付][width=12cm]

//noindent
@<em>{友太郎君}：「うーむ。いきなりむずかしいです」@<br>{}
@<em>{取間先生}：「そんなことはないぞ、ぜんぜん簡単じゃ。Switch Managerの役割はいわばUnixのinetdと同じと考えればよい。つまりSwitch Managerは新しいスイッチが接続してくるのを見張っているだけで、実際のスイッチとのやりとりはスイッチごとに起動するSwitch Daemonプロセスに一切合切まかせてしまうというわけじゃ」@<br>{}
@<em>{友太郎君}：「なるほど! たしかによく似ていますね」

== Switch Daemon

Switch Daemon(@<tt>{[trema]/objects/switch_manager/switch})は、Switch Managerが確立したスイッチとの接続を引き継ぎ、スイッチとTrema上のアプリケーションプロセスとの間で流れるOpenFlowメッセージを仲介します(@<img>{switch_daemon})。

//image[switch_daemon][スイッチとTremaアプリケーションの間でOpenFlowメッセージを仲介するSwitch Daemon][width=12cm]

 * アプリケーションプロセスが生成したOpenFlowメッセージをスイッチへ配送する
 * スイッチから受信したOpenFlowメッセージをアプリケーションプロセスへ届ける

=== OpenFlowメッセージの検査

Switch Daemonの重要な仕事として、OpenFlowメッセージの検査があります。Switch DaemonはTremaアプリケーションとスイッチの間で交換されるOpenFlowメッセージの中身をすべて検査します。そして、もし不正なメッセージを発見するとログファイル(@<tt>{[trema]/tmp/log/switch.[接続しているスイッチの Datapath ID].log})にエラーを出します。@<br>{}

//noindent
@<em>{友太郎君}：「Switch Manager と Switch Daemon プロセスでの役割分担とか、スイッチごとに Switch Daemon プロセスが起動するところなど、ほんとにUnixっぽいですね」@<br>{}
@<em>{取間先生}：「うむ。そうしたほうがひとつひとつのデーモンが単純化できて、実装も簡単になるからな」@<br>{}
@<em>{友太郎君}：「送信するメッセージを厳密にチェックするのはいいと思うんですが、受信もチェックするのってやりすぎではないですか？ほら、"送信するものに関しては厳密に、受信するものに関しては寛容に@<fn>{postel}" って言うじゃないですか」@<br>{}
@<em>{取間先生}：「受信メッセージのチェックをあまり寛容にしすぎると、後々とんでもないことが起こるのじゃ。Trema の開発者が言っておったが、昔あるネットワーク機器の開発でそれをやって相互接続テストでひどい目に遭ったそうじゃよ。それに、Trema は受信メッセージもちゃんとチェックするようにしたおかげで、実際に助かったことがたくさんあったのじゃ。たとえば OpenFlow の標準的なベンチマークツール Cbench(@<chap>{openflow_frameworks}) のバグ発見に Trema が一役買ったそうじゃ」@<br>{}
@<em>{友太郎君}：「へえー! すごいですね!」

//footnote[postel][TCP を規定した RFC 793 において、ジョン・ポステルが "相互運用性を確保するために TCP の実装が持つべき性質" として要約した、いわゆる@<ruby>{堅牢,けんろう}さ原則のこと。]

====[column] 取間先生曰く: きれいなOpenFlowメッセージを作る

OpenFlowメッセージフォーマットはCの構造体で定義されていて、ところどころパディング(詰め物)領域があります。このパディング領域はまったく使われないので、実際は何が入っていても動きます。しかしデバッグでOpenFlowメッセージを16進ダンプしたときなど、ここにゴミが入っていると大変見づらいものです。

そこでSwitch Daemonは送受信するOpenFlowメッセージのパディング部分をすべてゼロで埋め、きれいなOpenFlowメッセージにして送ってあげるという地道なことをやっています。これに気付く人は少ないでしょうが、こういう見えない工夫こそがTremaの品質です。

====[/column]

=== スイッチの仮想化

Switch Daemon のもう1つの重要な役割がスイッチの仮想化です。実は、Trema では1つのスイッチにいくつものアプリケーションをつなげられます。このとき、アプリケーションの間でスイッチ制御を調停し、競合が起きないようにするのも Switch Daemon の大きな役割です (@<img>{switch_virtualization})。

//image[switch_virtualization][Switch Daemon はスイッチ制御を調停することでアプリ間の競合を防ぐ]

例を1つ挙げます。OpenFlow の仕様に Flow Cookie という便利な機能があります。この Cookie を Flow Mod のパラメータとして指定すると、たとえば同じ Cookie を持つフローエントリーどうしを 1 つのグループとしてまとめて管理するなどができます。Trema でこの Cookie を使った場合、よく調べると Flow Mod で指定した Cookie 値とスイッチのフローテーブルに実際に設定された Cookie 値が異なることがわかります。

これは Switch Daemon がアプリケーション間で Cookie 値の競合が起こらないように調停しているからです。たとえば、Trema 上で動くアプリケーション A と B がたまたま同じ Cookie 値の 10 を使おうとするとどうなるでしょうか (@<img>{flow_cookie_virtualization})。そのままだと、両者が混じってしまい混乱します。Switch Daemon は、こうした Cookie 値の重複を避けるための次の変換を行います。Switch Daemon に Cookie 値が指定された Flow Mod が届くと、Switch Daemon は Cookie 値をまだ使われていない値に変換してからスイッチに打ち込みます (逆方向のメッセージではこれと逆のことを行います)。この変換のおかげで、プログラマはほかのアプリケーションと Cookie 値がかぶらないように注意する必要がなくなります。

//image[flow_cookie_virtualization][Cookie 値を自動変換してアプリ間の競合を防ぐ]

このほかにも、Switch Daemon は OpenFlow メッセージのトランザクション ID も自動で変換します。まさに Trema の世界の平和を守る縁の下の力持ちと言えます。

== 仮想ネットワーク

//noindent
@<em>{友太郎君}：「Trema でなにがうれしいかって、仮想ネットワークの機能ですよね! おかげでノートパソコン 1 台で開発できるからすっごく楽なんですけど、あれってどういうしくみなんですか？」@<br>{}
@<em>{取間先生}：「むずかしいことはしておらん。Linux カーネルの標準機能を使って、仮想スイッチプロセスと仮想ホストプロセスをつなげているだけじゃ」@<br>{}
@<em>{友太郎君}：「？」

=== 仮想スイッチ

仮想スイッチの実体は、フリーの OpenFlow スイッチ実装である Open vSwitch (@<href>{http://openvswitch.org/}) です。@<tt>{trema run} コマンドに与えられた仮想ネットワーク設定ファイル中の仮想スイッチ定義 (@<list>{vswitch}) に従って、Trema はスイッチプロセスを必要な数だけ起動します。

//list[vswitch][仮想ネットワーク設定ファイル中の仮想スイッチ定義例]{
vswitch("lsw") {
  datapath_id "0xabc"
}
//}

これに対応する @<tt>{trema run} のログはこうなります。

//cmd{
% trema -v run learning-switch.rb -c learning_switch.conf
  ...
sudo .../openvswitch/bin/ovs-openflowd --detach --out-of-band --fail=closed \
  --inactivity-probe=180 --rate-limit=40000 --burst-limit=20000 \
  --pidfile=.../trema/tmp/pid/open_vswitch.lsw.pid --verbose=ANY:file:dbg \
  --verbose=ANY:console:err --log-file=.../trema/tmp/log/openflowd.lsw.log \
  --datapath-id=0000000000000abc --unixctl=.../trema/tmp/sock/ovs-openflowd.lsw.ctl \
   --ports=trema0-0,trema1-0 netdev@vsw_0xabc tcp:127.0.0.1:6633
  ...
//}

=== 仮想ホスト

仮想ホストの実体は、phost と呼ばれるユーザーレベルプロセスです (@<tt>{[trema]/objects/phost/phost})。これは、任意のイーサネットフレーム・UDP/IP パケットを送受信できます。@<tt>{trema run} コマンドに与えられた設定ファイル中の仮想ホスト定義 (@<list>{vswitch}) に従って、Trema は必要な数の phost プロセスを起動します。

//list[vhost][仮想ネットワーク設定ファイル中の仮想ホスト定義例]{
vhost("host1") {
  ip "192.168.0.1"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:01"
}

vhost("host2") {
  ip "192.168.0.2"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:02"
}
//}

これに対応する @<tt>{trema run} のログはこうなります。

//cmd{
% trema -v run learning-switch.rb -c learning_switch.conf
  ...
sudo .../phost/phost -i trema0-1 -p .../trema/tmp/pid -l .../trema/tmp/log -D
sudo .../phost/cli -i trema0-1 set_host_addr --ip_addr 192.168.0.1 \
  --ip_mask 255.255.0.0 --mac_addr 00:00:00:01:00:01
sudo .../phost/phost -i trema1-1 -p .../trema/tmp/pid -l .../trema/tmp/log -D
sudo .../phost/cli -i trema1-1 set_host_addr --ip_addr 192.168.0.2 \
  --ip_mask 255.255.0.0 --mac_addr 00:00:00:01:00:02
  ...
sudo .../phost/cli -i trema0-1 add_arp_entry --ip_addr 192.168.0.2 \
  --mac_addr 00:00:00:01:00:02
sudo .../phost/cli -i trema1-1 add_arp_entry --ip_addr 192.168.0.1 \
  --mac_addr 00:00:00:01:00:01
//}

=== 仮想リンク

仮想スイッチと仮想ホストを接続する仮想リンクの実体は、Linux が標準で提供する Virtual Ethernet Device です。これは、Point-to-Point のイーサネットリンクを仮想的に構成してくれるものです。@<tt>{trema run} コマンドに与えられた仮想ネットワーク設定ファイル中の仮想リンク定義 (@<list>{vlink}) に従って、Trema は必要な数の仮想リンクを作ります。

//list[vlink][仮想ネットワーク設定ファイル中の仮想リンク定義例]{
link "lsw", "host1"
link "lsw", "host2"
//}

これに対応する @<tt>{trema run} のログはこうなります。

//cmd{
% trema -v run learning-switch.rb -c learning_switch.conf
  ...
sudo ip link delete trema0-0 2>/dev/null
sudo ip link delete trema1-0 2>/dev/null
sudo ip link add name trema0-0 type veth peer name trema0-1
sudo sysctl -w net.ipv6.conf.trema0-0.disable_ipv6=1 >/dev/null 2>&1
sudo sysctl -w net.ipv6.conf.trema0-1.disable_ipv6=1 >/dev/null 2>&1
sudo /sbin/ifconfig trema0-0 up
sudo /sbin/ifconfig trema0-1 up
sudo ip link add name trema1-0 type veth peer name trema1-1
sudo sysctl -w net.ipv6.conf.trema1-0.disable_ipv6=1 >/dev/null 2>&1
sudo sysctl -w net.ipv6.conf.trema1-1.disable_ipv6=1 >/dev/null 2>&1
sudo /sbin/ifconfig trema1-0 up
sudo /sbin/ifconfig trema1-1 up
  ...
//}

== Trema C ライブラリ

@<em>{友太郎君}：「そういえば、Trema って実は C からも使えるらしいじゃないですか。せっかくだから、どんなライブラリがあるか教えてくれますか」@<br>{}
@<em>{取間先生}：「よかろう。しかし、わしはちょっと飲み足りないので、一緒にビールでも買いに行かんか？ C ライブラリもなにしろ数がたくさんあるから、歩きながらひとつずつ説明することにしよう」@<br>{}
@<em>{友太郎君}：「まだ飲むんだ…」@<br>{}
@<em>{取間先生}：「なんじゃとお!」@<br>{}

酔っ払いに怒られることほどみじめなことはありません。でも友太郎君は取間先生の話が聞きたいので仕方無くコンビニエンスストアまで付き合います。

=== OpenFlow Application Interface

OpenFlow メッセージを受信したときにアプリケーションのハンドラを起動したり、逆にアプリケーションが送信した OpenFlow メッセージを適切な Switch Daemon に配送したりするのが OpenFlow Application Interface (@<tt>{[trema]/src/lib/openflow_application_interface.c}) です。

OpenFlow Application Interface は、アプリケーションが受信した OpenFlow メッセージをプログラマが扱いやすい形に変換してハンドラに渡します。たとえば、次の処理を行います。

 * OpenFlow メッセージのマッチングルール部をホストバイトオーダーへ変換
 * アクションなどの可変長部分をリストに変換
 * Packet In メッセージに含まれるイーサネットフレームを解析し、フレームと解析結果 (パケットの種類など) を組にしてハンドラへ渡す

//noindent
@<em>{友太郎君}：「なるほど。Trema の OpenFlow API が使いやすい理由って、裏でこういう変換をたくさんやってくれているからなんですね。たしかにバイトオーダー変換とかを意識しないで書けるのってすごく楽です」@<br>{}
@<em>{取間先生}：「そうそう。他の OpenFlow フレームワークでここまで親切なものはないぞ。Trema はこうやってプログラマの凡ミスによるバグを減らしているんじゃ」

=== OpenFlow Messages

アプリケーションが OpenFlow メッセージを生成するときに使うのが、OpenFlow Messages (@<tt>{[trema]/src/lib/openflow_message.c}) です。メッセージ受信の場合とは逆に、

 * ホストバイトオーダーで指定された値をネットワークバイトオーダーで変換し、OpenFlow メッセージのヘッダへ格納
 * リストの形で与えられた値を可変長ヘッダへ格納  

などを行います。

また、不正なメッセージの生成を防ぐため、パラメータ値の範囲検査やフラグの検査など OpenFlow 仕様と照らし合わせた厳密なチェックをここでやります。生成したメッセージは、OpenFlow Application Interface が提供するメッセージ送信 API を用いて、Switch Daemon を介してスイッチへ送信します。@<br>{}

//noindent
@<em>{友太郎君}：「チェックが厳しいですね!」@<br>{}
@<em>{取間先生}：「これはさっきも言ったように、Trema をいろいろなベンダのスイッチと相互接続したときに相手に迷惑をかけないための最低限の礼儀じゃ。逆に言うと、Trema と問題無くつながるスイッチは正しい OpenFlow 1.0 をしゃべることができる、とも言えるな」@<br>{}
@<em>{友太郎君}：「かっこいい!」

=== パケットパーサ

Packet In メッセージに含まれるイーサネットフレームを解析したり、解析結果を参照する API を提供したりするのが、パケットパーサ (@<tt>{[trema]/src/lib/packet_parser.c}) です。パケットが TCP なのか UDP なのかといったパケットの種類の判別や、MAC や IP アドレスといったヘッダフィールド値の参照を容易にしています。

=== プロセス間通信

Switch Daemon とユーザーのアプリケーション間の OpenFlow メッセージのやりとりなど、プロセス間の通信には @<tt>{[trema]/src/lib/messenger.c} で定義されるプロセス間通信 API が使われます。

=== 基本データ構造

その他、C ライブラリでは基本的なデータ構造を提供しています。たとえば、イーサネットフレームを扱うための可変長バッファ (@<tt>{[trema]/src/lib/buffer.c})、アクションリストなどを入れるための連結リスト (@<tt>{[trema]/src/lib/{linked_list.c,doubly_linked_list.c}})、FDB などに使うハッシュテーブル (@<tt>{[trema]/src/lib/hash_table.c}) などです。ただし、Ruby には標準でこれらのデータ構造があるため、Ruby ライブラリでは使われません。@<br>{}

//noindent
@<em>{取間先生}：「Trema の Ruby ライブラリも、このしっかりとした C ライブラリの上に構築されておる。@<tt>{[trema]/ruby/} ディレクトリの中を見てみるとわかるが、Ruby ライブラリの大半は C で書かれておるのだ。そのおかげでこうした各種チェックが Ruby からも利用できる」@<br>{}
@<em>{友太郎君}：「へー! あ、家に着きましたね」

== 低レベルデバッグツール Tremashark

コンビニエンスストアから帰ってきましたが、深夜の2人のTrema 談義はさらに続きます。@<br>{}

//noindent
@<em>{取間先生}：「こうして見ると Trema って意外と複雑じゃろう。もし友太郎君がさらに Trema をハックしてみたいとして、ツールの手助けなしにやるのはちょっとたいへんだと思うから、いいツールを紹介してあげよう。これは Tremashark と言って、Trema の内部動作を可視化してくれるありがたいツールじゃ。これを使うと、アプリケーションと Switch Daemon の間でやりとりされるメッセージの中身など、いろんなものが Wireshark の GUI で見られて便利じゃぞ」@<br>{}
@<em>{友太郎君}：「おお! そんなものがあるんですね!」

=== Tremashark の強力な機能

Tremashark は Trema の内部動作と関連するさまざまな情報を可視化するツールで、具体的には次の情報を収集・解析し、表示する機能を持ちます (@<img>{tremashark_overview})。

 1. Trema 内部やアプリケーション間の通信 (IPC) イベント
 2. セキュアチャネル、および任意のネットワークインターフェース上を流れるメッセージ
 3. スイッチやホストなどから送信された Syslog メッセージ
 4. スイッチの CLI 出力など、任意の文字列

//image[tremashark_overview][Tremashark の概要]

各種情報の収集を行うのが Tremashark のイベントコレクタです。これは、Trema 内部や外部プロセス・ネットワーク装置などから情報を収集し、時系列順に整列します。整列した情報は、ファイルに保存したりユーザーインターフェース上でリアルタイムに表示したりできます。

Tremashark のユーザーインターフェースは Wireshark と Trema プラグインからなります。イベントコレクタによって収集した情報はこのプラグインが解析し、Wireshark の GUI もしくは CUI 上に表示します。

=== 動かしてみよう

イベントコレクタは、Trema をビルドする際に自動的にビルドされます。しかし、Trema プラグイン標準ではビルドされませんので、利用するには次の準備が必要です。

==== Wireshark のインストール

Tremashark のユーザーインターフェースは Wireshark を利用していますので、まずは Wireshark のインストールが必要です。Ubuntu Linux や Debian GNU/Linux での手順は次のようになります。

//cmd{
% sudo apt-get install wireshark
//}

==== Trema プラグインのインストール

次に、Wireshark の Trema プラグインをビルドしてインストールします。Ubuntu Linux 11.10 の場合の手順は次のとおりです。

//cmd{
% cd /tmp
% apt-get source wireshark
% sudo apt-get build-dep wireshark
% cd [trema ディレクトリ]/src/tremashark/plugin
% ln -s /tmp/wireshark-(バージョン番号) wireshark
% cd wireshark
% ./configure
% cd ../packet-trema
% make
% mkdir -p ~/.wireshark/plugins
% cp packet-trema.so ~/.wireshark/plugins
% cp ../user_dlts ~/.wireshark
//}

==== OpenFlow プラグインのインストール

Trema のモジュール間で交換される OpenFlow メッセージを解析し表示するには、Trema プラグインに加えて OpenFlow プロトコルのプラグインも必要です。OpenFlow プロトコルのプラグインは OpenFlow のレファレンス実装とともに配布されており、次の手順でインストールできます。

//cmd{
% git clone git://gitosis.stanford.edu/openflow.git
% cd openflow/utilities/wireshark_dissectors/openflow
% patch < [trema ディレクトリ]/vendor/packet-openflow.patch
% cd ..
% make
% cp openflow/packet-openflow.so ~/.wireshark/plugins
//}

==== 実行してみよう

いよいよ、Tremashark を使って Trema のモジュール間通信をのぞいてみましょう。例として Trema サンプルアプリケーションのひとつ、ラーニングスイッチと Switch Daemon 間の通信を見てみることにします。

まず、次のコマンドでラーニングスイッチを起動してください。ここでオプションに @<tt>{-s} を指定することで、Tremashark のイベントコレクタとユーザーインターフェースが起動します。

//cmd{
% trema run learning-switch.rb -c learning_switch.conf -s -d
//}

ラーニングスイッチの起動後、イベントコレクタへのイベント通知を有効にする必要があります。これは、イベントを収集したいプロセスに USR2 シグナルを送ることで有効にできます。シグナルを送るための各プロセスの PID は、Trema のディレクトリの下の @<tt>{tmp/pid} 以下のファイルに保存されています。たとえば、Ruby で書かれたアプリケーションの PID は、"[コントローラのクラス名].pid" という名前のファイルに保存されます。また Switch Daemon の PID は、"switch.[管理するスイッチの Datapath ID].pid" という名前のファイルに保存されます。

今回の例では、ラーニングスイッチと Switch Daemon のイベントを見るのですから、次のように @<tt>{kill} コマンドを使って各プロセスへ USR2 シグナルを送ります。

//cmd{
% kill -USR2 `cat tmp/pid/LearningSwitch.pid`
% kill -USR2 `cat tmp/pid/switch.0x1.pid`
//}

これで、プロセス間の IPC イベントをのぞく準備ができました。ではイベントを発生させるために、以下のようにスイッチに接続されたホスト間でパケットを交換してみましょう。

//cmd{
% trema send_packets --source host1 --dest host2
% trema send_packets --source host2 --dest host1
//}

すると、@<img>{tremashark_gui} に示すようにモジュール間の通信をリアルタイムに観測できます。これによって、アプリケーションがどのような OpenFlow メッセージを送受信しているかなどを調べられます。

//image[tremashark_gui][Tremashark ユーザーインターフェース]

たとえば、@<img>{tremashark_gui} の一連の解析結果 (7, 8, 9, 10 番のメッセージ) により、Packet In メッセージをトリガとしてラーニングスイッチが Flow Mod メッセージをスイッチ 0x1 に対して送信していることがわかります。また、下半分のペインには送信した Flow Mod メッセージの各フィールドの値が表示されています (@<img>{trema_internal_with_tremashark})。

//image[trema_internal_with_tremashark][Tremashark による解析結果 (@<img>{tremashark_gui})]

//noindent
@<em>{友太郎君}：「もし原因不明なバグに遭遇したときは、こうやってメッセージの中身まで追える Tremashark を使えばいいわけですね」@<br>{}
@<em>{取間先生}：「ここまでわかっていれば、本格的な実用コントローラを作るのも難しくはないぞ。そういえば Trema はサンプルとは別に Trema Apps という実用アプリも公開しておる。何か大きなアプリケーションを作るときに役立つと思うから、友太郎君のためについでに紹介しておこうかの」@<br>{}

== Trema Apps

Trema Apps (@<href>{http://github.com/trema/apps}) は、Trema を使った実用的・実験的な少し大きめのアプリケーションを集めたリポジトリです。Trema 本体と同様に GitHub 上で公開されており、次の手順でダウンロードできます。

//cmd{
% git clone https://github.com/trema/apps.git
//}

それでは、Trema Apps の中でもとくに実用的なアプリを中心に解説していきましょう。

: ルーティングスイッチ (@<tt>{routing_switch})
  複数の OpenFlow スイッチで構成されるネットワークを 1 つのスイッチに仮想化します。サンプルプログラムのマルチラーニングスイッチと異なる点は、ループのあるネットワークトポロジにも対応している点と、パケットの転送に必ず最短パスを選択する点です。詳しくは@<chap>{routing_switch}で説明します。

: スライス機能付きスイッチ (@<tt>{sliceable_switch})
  OpenFlow ネットワーク全体を独立したスライスに分割し、複数の独立したスイッチに仮想化します。ちょうど、ネットワークを複数の VLAN に分けるイメージです。これによって、アプリケーションやユーザーグループなど用途に応じて独立した仮想ネットワークを作れます。詳しくは@<chap>{sliceable_switch}で説明します。

: リダイレクト機能付きルーティングスイッチ (@<tt>{redirectable_routing_switch})
  ルーティングスイッチの亜種で、ユーザー認証とパケットのリダイレクト機能を付け加えたものです。基本的な動作はルーティングスイッチと同じですが、認証されていないホストからのパケットをほかのサーバーに強制的にリダイレクトします。この仕組みを使えばたとえば、認証していないユーザーの HTTP セッションを強制的に特定のサイトへ飛ばすなどといったことが簡単にできます。

: memcached 版ラーニングスイッチ (@<tt>{learning_switch_memcached})
  サンプルプログラムのラーニングスイッチ (@<chap>{learning_switch}) と同じ機能を持ちますが、FDB の実装に memcached (@<href>{http://memcached.org/}) を用いています。Ruby のmemcached ライブラリを使うことで、オリジナルのラーニングスイッチにほとんど変更を加えずに memcached 対応できているところがポイントです。また、マルチラーニングスイッチ (@<chap>{openflow_framework_trema}) の memcached 版もあります。

: シンプルマルチキャスト (@<tt>{simple_multicast})
  IPTV サービスで使われているマルチキャスト転送を実現するコントローラです。配信サーバーから送られたパケットを OpenFlow スイッチがコピーして、すべての視聴者へと届けます。

: フローダンパー (@<tt>{flow_dumper})
  OpenFlow スイッチのフローテーブルを取得するためのユーティリティです。デバッグツールとしても便利です。@<chap>{diy_switch} で使い方を紹介しています。

: Packet In ディスパッチャ (@<tt>{packetin_dispatcher})
  Packet In メッセージを複数の Trema アプリケーションに振り分けるサンプルです。物理アドレスから、ユニキャストかブロードキャストかを判断します。

: ブロードキャストヘルパ (@<tt>{broadcast_helper})
  ブロードキャストやマルチキャストなど、コントローラに負荷の大きいトラフィックを分離して、別の独立したコントローラで処理させるためのアプリです。

: フローマネージャ (@<tt>{flow_manager})
  関連するフローエントリーをまとめて管理する API を提供します。かなり実験的な実装なので、API は変更する可能性があります。

== まとめ

そうこうしている間に夜は白み、取間先生はゆっくりと帰り支度を始めました。@<br>{}

//noindent
@<em>{取間先生}：「友太郎君どうもありがとう。今夜はひさびさに若者と話せて楽しかったよ。わしが教えられることはすべて教えたし、これで君も立派な OpenFlow プログラマじゃ」@<br>{}
@<em>{友太郎君}：「こちらこそありがとうございました。なにかいいアプリケーションができたら、先生にも教えますね!」@<br>{}

今回は、今まで触れてこなかった Trema の構成や内部動作について学びました。

 * Trema で開発した OpenFlow コントローラは、Trema が提供する Switch Manager、Switch Daemon とアプリケーションプロセスから構成される
 * コントローラの動作に必要なプロセスの起動や停止などの管理は @<tt>{trema} コマンドが自動的に行ってくれる
 * Tremashark により、Trema の内部動作を可視化できる。これは、アプリケーションや Trema の動作を詳細に確認したい場合に役立つ
 * Trema Apps には、大きめの Trema アプリケーションが公開されており、自分で実用アプリケーションを作る際の参考になる

さて、第 II 部プログラミング編はこれでおしまいです。Trema を使えば OpenFlow コントローラが簡単に開発できることを実感していただけたのではないでしょうか？Trema を使って、ぜひあなたも便利な OpenFlow コントローラを開発してみてください。
