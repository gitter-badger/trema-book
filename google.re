= Google のデータセンタ間接続

2012 年 4 月、カリフォルニア州サンタクララで開催された Open Networking Summit において、Google の Urs Hoelzle 氏が衝撃の発表を行いました。Google は早くから OpenFlow に取り組んでおり、Google 内部のネットワークではすでに OpenFlow が動作しているというのです。Internet 界の巨人 Google は、OpenFlow のどのような特徴に着目し、どのように活用しているのでしょうか？Hoelzle 氏の講演内容から、読み解いていきましょう。

== Google が OpenFlow を使う理由

Google が多くの巨大データセンターを保有し、インターネット上の様々な情報を収集していることは、みなさんもご存知でしょう。例えば Google が公開している資料からは、以下のようなデータを見つけることができます。

 * 毎月数十億ページを収集している
 * 毎分 60 時間分の動画が投稿されている
 * ...

これらのデータは、Google では非常に多くのデータを日々扱っていることを示しています。このように大量のデータを処理するためには、Google はスケーラビリティの高いコンピューティング環境を低コストに実現しています。

ネットワークに関してはどうでしょうか？ Google 以外にも、多くの事業者によって、巨大なネットワークの運用が行われています。そもそもインターネットがきちんと動作してることを考えると、今の IP ネットワーク技術は高いスケーラビリティを実現可能であると言ってもよいでしょう。

しかし、Google は既存のネットワーク技術には満足していないようです。特にデータセンター間を繋ぐ WAN 接続に関しては、OpenFlow を用いた新しい試みを行なっています。なぜ彼らは既存の技術に満足しなかったのでしょうか？ポイントは、そのコストにあるようです。

=== 低コスト化への課題

高品質なサービスを如何に低コストに提供するかは、どのサービス事業者にとっても共通の課題でしょう。特に Google のような巨大な設備を持つサービス事業者では、小さなコスト削減の積み重ねが、結果として大きなコスト削減につながります。

コスト削減を考える際に、大きな課題となるのが、WAN 回線の効率化です。世界最大規模のトラフィックを誇る Google のトラフィックを捌くためには、Google は世界各国に巨大なデータセンターを多数持っており、これらの間をつなぐために WAN 回線も多数有しています。これらの回線は容量が大きなものほど高価になるので、必要最小限の回線を最大限に活用する必要があります。

しかしながら、WAN 回線の効率化の実現は、容易ではありません。従来の IP のルーティングでは、宛先に対して最短となるパスが選択され、転送が行われます。回線の使用帯域に関わらず、最短となる経路が選択されるため、トラフィックが集中する回線や、全く使用されない回線が生じる可能性があります。

=== Google の WAN

Google の WAN は役割に応じて、以下の二つのパートに分かれています。

 * I-Scale : インターネットに面するネットワーク。インターネットを介して Google を利用するユーザ向けのトラフィックを運ぶ。
 * G-Scale : データセンター間を繋ぐバックボーンネットワーク。内部向けトラフィックを扱う。

Google は、これらのうち G-Scale に、OpenFlow を適用しました。G-Scale は内部向けのネットワークであり、以下の理由により、効率化の余地が大きいためです。

 * どのようなトラフィックが流れているかを予測できる
 * トラフィック毎の優先度を自身で決定できる

== どのように WAN 回線を効率的に使う?

ここでは、データセンター間を結ぶ WAN 回線を効率的に使うためにはどのようにすればよいか、その方法と課題について見ていきます。

//image[wan][データセンター A からデータセンター B へどのくらいトラフィックを流せる？][scale=0.4]

まずは、@<img>{wan} のデータセンター A からデータセンター B へどの程度のトラフィックを流せるかを考えてみましょう。通常の IP ルーティングでは、最短経路となるリンク 1 しか使用しないため、このリンクの帯域が上限となります。リンク帯域の上限を超えるトラフィックを扱うためには、リンク帯域の増設が必要になり、コストが増える要因となります。しかし、@<chap>{openflow_usecases} で紹介したように、スイッチ C を経由する経路もあわせて使うことで、さらなるトラフィックを流せます(@<img>{multipath})。

//image[multipath][複数経路を使ってトラフィックを転送する][scale=0.4]

@<img>{multipath} のように複数経路を使うと、どの程度までトラフィック流すことができるでしょうか？他のトラフィックが全くなければ、それぞれのリンク帯域の上限まで流すことができます。例えば、リンク 1 には、リンク帯域の 10 Gbps までトラフィックを流せます。複数のリンクを経由する場合には、流すトラフィック量を帯域が最小のリンクに合わせる必要があります。スイッチ C を経由する経路で流せるトラフィック量は、リンク 3 の帯域である 6Gbps までとなります。つまり、@<img>{multipath} のネットワークでは、データセンター A からデータセンター B へ、最大で 16Gbps のトラフィックを流せるということになります。

しかし、前に説明した WAN 効率化を実現するためには、以下の三つの課題をクリアしなければなりません。

 1. 各リンクの空き帯域を把握する
 2. 空き帯域を、どのトラフィック転送に使うかを決定する
 3. トラフィックを、それぞれの経路毎に適切な量に振り分ける

それぞれの課題を、それぞれ詳しく見ていきましょう。

=== 課題 1 : 空き帯域の把握

一つ目の課題は、各リンクの空き帯域を知る必要があるという点です。@<img>{multipath} では、データセンター A からデータセンター B へは 16Gbps までトラフィックを流せると説明しました。しかし、他のデータセンター間にもトラフィックがある場合、16Gbps のうち空いている帯域しか使用できません。実際にどの程度トラフィックを流せるかを知るためには、各リンクの空き帯域の情報が必要です。データセンター A は、スイッチ A がやり取りしているトラフィック量を調べることで、リンク 1 とリンク 2 の空き帯域を知ることはできます。しかし、データセンター A から離れた場所にあるリンク 3 の空き帯域については、何らかの方法で取得しなければなりません。@<img>{multipath} では、リンクが 3 つしかないため、空き帯域を調べるリンクは一つだけで済みますが、 実際のネットワークにはより多くのリンクあるため、これらの空き帯域をすべて調べる必要があります。

=== 課題 2 : どのトラフィックが空き帯域を使用するか

二つ目の課題は、空き帯域をどのトラフィック転送に使用するかを、どのように決めるかという点です。例えば、@<img>{conflict} のネットワークにおいて、リンク 1 の空き帯域が 10Gbps であったとします。データセンター A とデータセンター B それぞれが勝手に、データセンター C へ 10Gbps のトラフィックを流そうとすると、当然このうち半分のトラフィックしかリンク 1 を通過できません。通過できないトラフィックは、スイッチによって破棄されます。このような事態を避けるためには、どちらのトラフィックがリンク 1 の空き帯域を利用するか、もしくはリンク 1 の空き帯域を分けあって利用する場合、どちらがどれだけの量のトラフィックを流すかを、決める仕組みが必要です。

//image[conflict][どのトラフィックが、空き帯域を使うかを決定する必要がある][scale=0.4]

=== 課題 3 : 複数経路へのトラフィックの振り分け

三つ目の課題は、複数の経路へのトラフィック振り分けに関する点です。例えば、@<img>{traffic} 中のデータセンターからのトラフィックのために、リンク 1 経由の経路で 10Gbps、リンク 2 経由の経路で 6Gbps の帯域がそれぞれ使用可能であるとします。スイッチは、これらの使用可能帯域を超えないように、これらの経路に各サービスのトラフィックを振り分ける必要があります。@<chap>{openflow} で説明したように、OpenFlow スイッチではヘッダの情報を見てどちらのパスにパケットを転送するかを決定することしかできません。そのため、トラフィックを振り分けるためには、各サービス毎のトラフィック量を知っている必要があります。

//image[traffic][どのトラフィックを、どの経路に流すかを決定する必要がある][scale=0.4]

== G-Scale での WAN 回線の効率利用

ここでは Google のデータセンター間ネットワークである G-Scale が、前節で述べた課題をどのように解決して、WAN の効率化を実現しているかを見ていきます。

G-Scale では、流入するトラフィック量を測定し、全体を調停するトラフィックエンジニアリングサーバ (TE サーバ) を導入しています(@<img>{interdc-network})。TE サーバは、各データセンターからの流入トラフィックに関する情報を集めます。そして、各データセンター間のトラフィックを、どのパスを使って転送すべきか決定し、その情報を WAN 接続部に送ります。

//image[interdc-network][G-Scale はデータセンター間のトラフィックを運ぶ][scale=0.4]

WAN 接続部の構成は @<img>{quagga-ofc} のようになっています。G-Scale は、世界中に散らばる Google のデータセンター間を結ぶ巨大なネットワークですが、各データセンターと接続する OpenFlow スイッチや BGP 処理部は、そのデータセンターと同じ拠点に配置されています。

//image[quagga-ofc][それぞれの WAN 接続部には、OpenFlow コントローラと BGP 処理部が配置されている][scale=0.4]

G-Scale を構成する OpenFlow スイッチは、Google が自ら設計したものです。大量のトラフィックを扱う必要があるため、このスイッチは 10Gbps Ethernet のインターフェイスを 100 ポート以上持ちます。このような仕様を持つスイッチは市場には存在しないため、Google は自作を行ったようです。自前での装置開発にはそれなりのコストが掛かりますが、WAN の効率化により削減できるコストの方が大きいと、Google は判断したのでしょう。

各 OpenFlow コントローラは、それぞれの WAN 接続部内の OpenFlow スイッチの制御を行なっているだけです。しかし、BGP や TE サーバからの情報を元にフローを決定することで、G-Scale 全体として WAN の帯域の有効活用を実現しています。

ネットワーク中の全 OpenFlow スイッチを一台のコントローラで制御するルーティングスイッチ (@<chap>{routing_switch}) とは異なり、G-Scale は複数の OpenFlow コントローラが協調してネットワークを制御しています。複数コントローラの協調動作については他にさまざまな方法が考えられるため、OpenFlow の仕様自体には規定されていません。G-Scale は、その協調動作の代表的な一例です。

G-Scale が、データセンターから受信したパケットを他のデータセンターに適切に届けるためには、以下の二つを行う必要があります。

 * パケットの宛先アドレスを参照し、どのデータセンター宛なのかを調べる
 * 宛先となるデータセンターまでの、どのパスを使ってパケットを転送するかを決定する
 * 決定したパスにそってパケットを転送する

=== 届け先のデータセンターを決定する

データセンターが G-Scale からパケットを受け取るためには、データセンター内にあるホストの IP アドレスを、G-Scale に教えてあげる必要があります。そうしなければ、G-Scale は受け取ったパケットをどのデータセンターに送り届ければいいかを判断できません。

G-Scale では、この目的のために BGP を用いています。BGP は、異なる組織間で、アドレスに関する情報 (経路情報と呼びます) を交換するためのプロコトルです。インターネットのように、様々な組織から構成されるネットワークを作るためには欠かすことのできないプロトコルです。BGP では、それぞれの組織を AS と呼び、AS 単位での経路情報の交換を行います。

BGP は、通常異なる組織同士の接続に用いられますが、Google ではデータセンター間の接続に用いています。各データセンターと G-Scale それぞれを一つの組織に見立て、それぞれが一つの AS として動作しています。G-Scale は、BGP により各データセンターから経路情報をもらい、その情報を元にしてパケットの届け先を決定しています。

//image[bgp][BGP を用いてデータセンター A 内のアドレスを G-Scale に通知する][scale=0.4]

例えば、データセンター A 内のホストには 192.168.1.0/24 のアドレスを持っていたとします(@<img>{bgp})。データセンター A 内の BGP ルータはこのアドレスに対する経路情報を BGP を用いて、G-Scale の BGP 処理部に通知します。経路情報を受け取った BGP 処理部は、やはり BGP を用いて、G-Scale 内の他の BGP 処理部へと経路情報を転送します。このようにすることで、例えば宛先が 192.168.1.1 であるパケットを受け取った時に、そのパケットをデータセンター A へと送り届ければよいということを、G-Scale は知ることができます。

===[column] E-BGP と I-BGP

BGP には External BGP (E-BGP) と Internal BGP (I-BGP) の二種類の接続形態があります。E-BGP は異なる組織間の、I-BGP は組織内の情報交換に、それぞれ用います。@<img>{bgp} では、データセンター A の BGP ルータと G-Scale 内の BGP 処理部との間の接続が、E-BGP になります。また G-Scale 内の BGP 処理部同士は I-BGP で接続されています。E-BGP と I-BGP では、受け取った経路情報を転送する際のルールが異なっています。

===[/column]

=== 転送パスを決定する

届け先のデータセンターが決定したら、次はトラフィックを転送するパスを決める必要があります。G-Scale では、ネットワーク中のリンク帯域を有効活用するために、複数のパスを有効に活用します。

== まとめ

Google が、WAN 回線の有効活用を行うために、データセンター間トラフィックの転送に OpenFlow をどのように活用しているかを紹介しました。

== 参考文献

: Urs Hoelzle 氏 Open Networking Summit 2012 での講演 ( @<tt>{http://youtu.be/VLHJUfgxEO4} )
  今回の取り上げた Urs Hoelzle 氏の講演内容が、Youtube に投稿されています。

: Google を支える技術 (西田圭介著、技術評論社)
  この章ではネットワーク面でのコスト削減について取り上げましたが、この本ではデータセンター自体の運営コストなどについての分析が行われています。この本を読むことで、Google が如何にして何万台ものコンピュータを使ってサービスを実現しているかを学ぶことができます。

: インターネットルーティングアーキテクチャ - BGPリソースの詳解 (サム・ハラビ、ダニー・マクファーソン著、ソフトバンククリエイティブ)
  この章でも簡単に紹介しましたが、より深く BGP について学びたい人は、この本を読むとよいでしょう。具体的な例が多く紹介されており、BGP の動作をイメージしながら理解することができます。