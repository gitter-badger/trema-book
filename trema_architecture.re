= Trema のアーキテクチャ

とある夜更け、友太郎君がいつものようにコーディングしていたときのこと。
玄関口から聞き覚えのある声。@<br>{}

//noindent
@<em>{謎の声}「おーい、友太郎君はおるかの？」@<br>{}
@<em>{友太郎}「あっ! 取間先生じゃないですか。こんな夜中にどうしたんですか？」@<br>{}
@<em>{取間先生}「さっき三軒茶屋で飲んでいたら、履いていたサンダルが無く
なってしまってのう。サンダルを探していたら終電が無くなってしまったん
じゃ。そこで相談じゃが友太郎君、今夜はわしを泊めてくれんかのう？お礼と
言ってはなんだが、始発の時間まで Trema のアーキテクチャを説明しよう。友
太郎君は Trema に詳しくなってきたし、そのへんの話に興味あろうと思ってな」@<br>{}
@<em>{友太郎}「いいですよ! さあどうぞどうぞ」

== Trema のアーキテクチャ

//noindent
@<em>{取間先生}「じゃあ基本的な話から。Trema でコントローラを動かすときは
"trema run" コマンドじゃったな。これって、実は裏でいろんなプロセスを起
動しているんじゃ。どんなプロセスが起動しているか、"trema run" に -v オ
プションをつけると見ることができるぞ。ためしにサンプルの
learning-switch を起動してみてごらん」

//cmd{
% ./trema run src/examples/learning_switch/learning-switch.rb \\
  -c src/examples/learning_switch/learning_switch.conf -v
.../trema/objects/switch_manager/switch_manager --daemonize \\
--port=6633 -- port_status::LearningSwitch packet_in::LearningSwitch \\
state_notify::LearningSwitch vendor::LearningSwitch
...
//}

//noindent
@<em>{友太郎}「うわっ、なんかいっぱい字が出てきましたね!」@<br>{}
@<em>{取間先生}「この出力を見れば、trema コマンドが中でどんなことをしてく
れるかわかるのじゃ。上のほうを見ると、switch_manager というプロセスが
起動されておるな？ Trema ではこのプロセスがスイッチと最初に接続するん
じゃ。」

=== スイッチ接続の受付

Switch Manager はスイッチからの接続要求を待ち受けます。接続を確立すると、
子プロセスとして Switch Daemon プロセスを起動し、接続を引き渡します
(図○)。@<br>{}

//noindent
@<em>{友太郎}「いきなりむずかしいですね」@<br>{}
@<em>{取間先生}「そんなことはないぞ、ぜんぜん簡単じゃ。inetd と同じで、
OpenFlow が使う TCP の 6633 番ポートを Switch Manager が見張っていると
考えるとよい。そして、スイッチが接続してきたら子プロセスとして Switch
Daemon を起動し、そいつに後のことを全部まかせるのじゃ」@<br>{}
@<em>{友太郎}「なるほど! たしかに inetd の仕組みと同じですね」

==== Switch Daemon

Switch Daemon は、Switch Manager が確立したスイッチとの接続を引き継ぎ、
スイッチと Trema 上のアプリケーションプロセスとの仲介をします。

 * アプリケーションプロセスが生成した OpenFlow メッセージをスイッチへ
   配送する
 * スイッチから受信した OpenFLow メッセージをアプリケーションプロセス
   へ届ける

Switch Daemon は、交換される OopenFlow メッセージの中身をすべて検査し
ます。もし不正なメッセージを発見するとエラーを出します。@<br>{}

//noindent
@<em>{友太郎}「いくつかのプロセスで役割分担ができていて、いかにも UNIX
っぽいですね。送信するメッセージは厳密にチェックするのはいいと思うんで
すが、受信もチェックするのってやりすぎではないですか？ほら、"送信する
ものは厳密に、受信するものは寛容に" って言うじゃないですか」@<br>{}
@<em>{取間先生}「受信メッセージのチェックをあまり寛容にすると、後々とんでも
ないことが起こるのじゃ。Trema の開発者が言っておったが、昔それをやって
インターオペラビリティのテストで死ぬ目に遭ったそうじゃよ。それに、
Trema は受信メッセージもちゃんとチェックするようにしたおかげで、実際に
助かったことがたくさんあったそうじゃ。たとえば OpenFlow の標準的なベン
チマークツール cbench のバグを発見したのも Trema チームだし、この間の
Interop では様々なベンダのスイッチのバグ発見に一役買ったそうじゃ」@<br>{}
@<em>{友太郎}「へえー! すごいですね!」

=== Trema C ライブラリの構成

@<em>{友太郎}「あっそういえば、Trema って実は C からも使えるらしいじゃ
ないですか。せっかくだから、どんなライブラリがあるか教えてくださいよ」@<br>{}
@<em>{取間先生}「よかろう。ちょっとわしは飲み足りないので、一緒にビール
でも買いに行かんか？ C ライブラリもなにしろ数がたくさんあるから、歩きな
がら説明することにしよう」@<br>{}
@<em>{友太郎}「いいですね! そうしましょう」

==== OpenFlow Application Interface

OpenFlow メッセージを受信したときにアプリケーションのハンドラを起動した
り、逆にアプリケーションが送信した OpenFlow メッセージを適切なSwitch
Daemon に配送したりするのが OpenFlow Application Interface
(@<tt>{src/lib/openflow_application_interface.c}) です。

OpenFlow Application Interface は、受信した OpenFlow メッセージをアプリ
ケーションが扱いやすいに変換してハンドラに渡します。たとえば、次の処理
を行います。

 * OpenFlow メッセージのマッチングルール部をホストバイトオーダへ変換
 * 可変長部分をリストに変換
 * packet_in メッセージに含まれるイーサネットフレームを解析し、フレーム
   と解析結果 (パケットの種類など) を組にしてハンドラへ提供

//noindent
@<em>{友太郎}「うーん、ちょっとむずかしいけど、要はプログラムから扱いや
すいように OpenFlow メッセージを変換したり、あらかじめ解析しておいてく
れたりするってことですか？」@<br>{}
@<em>{取間先生}「そうそう、そういうことじゃ。友太郎君冴えておるぞ」

==== OpenFlow Messages

アプリケーションが OpenFlow メッセージを生成する歳に利用するのが、
OpenFlow Messages (@<tt>{src/lib/openflow_message.c}) です。メッセージ
受信の場合とは逆に、

 * ホストバイトオーダで指定された値をネットワークバイトオーダで変換し、
   OpenFlow メッセージのヘッダへ格納
 * リストの形で与えられた値を可変長ヘッダへ格納  

などを行います。また、不正なメッセージの生成を防ぐため、値の範囲やビッ
トの検査も行います。OpenFlow Messages によって生成したメッセージは、
OpenFlow Application Interface が提供するメッセージ送信 API を用いて、
Switch Daemon を介してスイッチへ送信します。@<br>{}

//noindent
@<em>{友太郎}「いろいろと気が効いてますね」@<br>{}
@<em>{取間先生}「そうじゃろうそうじゃろう。まあふつうプログラミングフレーム
ワークと言ったらバイトオーダの自動変換や可変長ヘッダのリストへの変換く
らいやってくれてもいいと思うんじゃが。しかし、他のフレームワークではやっ
てくれないことがほとんどなのじゃ。。。」@<br>{}
@<em>{友太郎}「そうなんですね! Trema って便利だなあ」

==== パケットパーサ

packet_in メッセージに含まれるイーサネットフレームを解析したり、解析結
果を参照する API を提供したりするのが、パケットパーサ
(@<tt>{src/lib/packet_parser.c}) です。パケットが TCP なのか UDP なのか
といったパケットの種類の判別や、MAC や IP アドレスといったヘッダフィー
ルド値の参照を用意にしています。

==== プロセス間通信

Switch Daemon とユーザのアプリケーション間の OpenFlow メッセージのやり
とりなど、プロセス間の通信には @<tt>{src/lib/messenger.c} で定義される
プロセス間通信 API が使われます。

==== 基本データ構造

その他、C ライブラリでは基本的なデータ構造を提供しています。たとえば、
可変長バッファ (@<tt>{src/lib/buffer.c})、連結リスト
(@<tt>{linked_list.c, doubly_linked_list.c})、ハッシュテーブル
(@<tt>{src/lib/hash_table.c}) などです。

=== 仮想ネットワークの仕組み

//noindent
@<em>{友太郎}「Trema でなにがうれしいかって、仮想ネットワークの機能です
よね! おかげでノート PC 1 台で開発できるからすっごく楽なんですけど、あ
れってどういうしくみなんですか？」@<br>{}
@<em>{取間先生}「むずかしいことはしておらん。Linux カーネルの標準機能を
使って、仮想スイッチプロセスと仮想ホストプロセスをつなげているだけじゃ」@<br>{}
@<em>{友太郎}「？」

==== 仮想スイッチ

ソフトウェア実装の OpenFlow スイッチです。Trema では、フリーの
OpenFlow スイッチ実装である Open vSwitch
(@<tt>{http://openvswitch.org/}) を利用しています。"trema run" コマン
ドに与えられた仮想ネットワーク設定ファイル中の仮想スイッチ定義
(@<tt>{vswitch} で始まる行) に従って、スイッチプロセスを必要な数だけ起
動します。

==== 仮想ホスト

仮想ホストの実態は、任意のイーサネットインタフェースから任意のイーサネッ
トフレーム・UDP/IP パケットを送受信できる phost と呼ばれるソフトウェア
です (@<tt>{vendor/phost/)。"trema run" コマンドに与えられた設定ファイ
ル中の仮想ホスト定義 (@<tt>{vhost} で始まる行) に従って、必要な数のホス
トプロセスを起動します。

==== 仮想リンク

仮想スイッチと仮想ホストを接続するため、Linux が標準で提供する Virtual
Ethernet Device を仕様しています。これは、Point-to-Point のイーサネット
リンクを仮想的に構成してくれるものです。"trema run コマンドに与えられた
仮想ネットワーク設定ファイル中の仮想リンク定義 (@<tt>{link} で始まる行)
に従って、必要な数の仮想リンクが設定されます。

#@warn(今まで説明してきたこのあたり、trema run -v の出力と対応させたいな)

== 低レベルデバッグツール Tremashark

//noindent
@<em>{取間先生}「こうして見ると Trema って意外と複雑じゃろう。もし友太
郎君がさらに Trema をハックしてみたいとして、ふつうにやるのはちょっとた
いへんだと思うから、いいツールを紹介してあげよう。これは Tremashark と
言って、Trema の内部動作を可視化してくれるありがたいツールじゃ。これを
使うと、アプリケーションと Switch Daemon の間でやりとりされるメッセージ
など、いろんなものが Wireshark の GUI で見られて便利じゃぞ!」@<br>{}
@<em>{友太郎}「おお! ぜひ教えてください!」

=== Tremashark の強力な機能

Tremashark は Trema の内部動作と関連するさまざまな情報を可視化するもの
で、具体的には次の情報を収集/解析/表示する機能を持ちます (図○)。

 1. Trema 内部・アプリケーションのモジュール間通信 (IPC) イベント
 2. セキュアチャネル、および任意のネットワークインタフェース上を流れるメッセージ
 3. スイッチなどから送信された Syslog メッセージ
 4. スイッチの CLI 出力など、任意テキスト文字列

情報の収集を行うのが Tremashark イベントコレクタと呼ばれるモジュールで
す。これは、Trema 内部、もしくは外部プロセス・ネットワーク装置から情報
を収集して時系列に整列します。整列した情報は、ファイルに保存したりユー
ザインタフェース上でリアルタイムに表示したりできます。

Tremashark のユーザインタフェースは Wireshark と Trema 用プラグインから
構成されています。Tremashark イベントコレクタによって収集した情報はこの
プラグインが解析し、GUI もしくは CUI 上に表示されます。

=== 動かしてみよう

Tremashark イベントコレクタは、Trema をビルドする際に自動的にビルドされ
ます。しかし、ユーザインタフェースは標準ではビルドされていませんので、
利用するには次の準備が必要です。

==== Wireshark のインストール

Tremashark のユーザインタフェースは Wireshark を利用していますので、
Wireshark のインストールが必要です。Ubuntu Linux や Debian GNU/Linux で
は、次のようにインストールできます。

//cmd{
% sudo apt-get install wireshark
//}

==== Trema プラグインのインストール

次に、Wireshark の Trema プラグインをビルドしてインストールします。
Ubuntu Linux 11.10 の場合の手順は次のとおりです。

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

Trema のモジュール間で交換される OpenFlow メッセージを解析・表示するに
は、Trema プラグインに加えて OpenFlow プロトコルのプラグインも必要です。
OpenFlow プロトコルのプラグインは OpenFlow のリファレンス実装とともに配
布されており、次の手順でインストールできます。

//cmd{
% git clone git://gitosis.stanford.edu/openflow.git
% cd openflow/utilities/wireshark_dissectors/openflow
% patch < [trema ディレクトリ]/vendor/packet-openflow.patch
% cd ..
% make
% cp openflow/pcacket-openflow.so ~/.wireshark/plugins
//}

==== 実行してみよう

いよいよ、Tremashark を使ってみましょう。ここでは、Trema のモジュール間
通信を覗いてみます。Trema サンプルアプリケーションとして提供されている
Learning Switch と Switch Daemon 間の通信を見てみることにします。

次のコマンドで Learning Switch を起動してください。オプションに -s を指
定することで、Tremashark のイベントコレクタとユーザインタフェースが起動
します。

//cmd{
% ./trema run src/examples/learning_switch/learning-switch.rb \\
  -c src/examples/learning_switch/learning_switch.conf -s -d
//}

Learning Switch の起動後、Tremashark イベントコレクタへの IPC イベント
通知を有効にします。これは、モジュールのプロセスに USR2 シグナルを送る
ことで有効にできます。シグナルを送るための各モジュールのプロセスの PID
は、Trema のディレクトリの下の @<tt>{tmp/pid} 以下のファイルに保存され
ています。Ruby で書かれたアプリケーションの PID は、
[コントローラのクラス名].pid という名前のファイルに保存されます。
Switch Daemon の PID は、switch.[管理するスイッチの Datapath ID].pid と
いう名前のファイルに保存されます。

今回の例では、次のようにアプリケーションと Switch Daemon に対してシグナ
ルを送りましょう。

//cmd{
% kill -USR2 `cat tmp/pid/LearningSwitch.pid`
% kill -USR2 `cat tmp/pid/switch.0x1.pid`
//}

これで、モジュール間の IPC イベントを除く準備ができました。たとえば、以
下のようにスイッチに接続されたホスト間でパケットを交換してみましょう。

//cmd{
% ./trema send_packets --source host1 --dest host2
% ./trema send_packets --source host2 --dest host1
//}

すると、図○に示すようにモジュール間の通信をリアルタイムに観測できます。
これによって、モジュール間で正常に情報交換が行われていることや、アプリ
ケーションが OpenFlow メッセージの送受信を正常に行っているかどうかなど
を知ることができます。

たとえば、図○の No.7 〜 No.10 のメッセージにより、packet_in メッセージ
をトリガとして、Learning Switch が Flow Mod メッセージをスイッチに対し
て送信していることがわかります (図○)。

== まとめ

//noindent
@<em>{友太郎}「今夜は、いろいろとためになったなあ」@<br>{}
@<em>{取間先生}「どうじゃ？やる気が出てきたようじゃな。もし Trema 自体
の開発にも興味があれば、どんどん Trema へコミットしてみるとよいぞ」@<br>{}

そうこうしている間に夜が明け、取間先生は自宅へと帰っていきました。今回
は、Trema の構成、内部動作について学びました。学んだことは、次の 3 点で
す。

 * Trema を利用して開発した OpenFlow コントローラは、Trema が提供する
   Switch Manager、Switch Daemon とアプリケーションプロセスから構成され
   ます。
 * Trema は複数プロセスから構成されるシステムであり、必要なプロセスの起
   動・停止・管理は trema コマンドにより行われます。
 * Tremashark により、Trema の内部動作を可視化することができます。これ
   は、アプリケーションや Trema の動作を詳細に確認したい場合に役立ちま
   す。
   
本書はこれでおしまいです。Trema を使えば OpenFlow コントローラが簡単に
開発できることを実感していただけたのではないでしょうか？Trema を使って、
ぜひあなたも OpenFlow コントローラを開発してみてください。

== 参考文献