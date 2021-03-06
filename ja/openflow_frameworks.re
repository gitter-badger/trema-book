= OpenFlow の開発フレームワーク

//lead{
さっそく OpenFlow で何かを作ってみたい! その前に便利な OpenFlow の開発フレームワークを見てみましょう。巨人の肩に乗ってしまえば目的地まではもうすぐです。
//}

//indepimage[robot][][width=10cm]

== 開発フレームワークを活用しよう

新しくWebサービスを立ち上げるには、今やRuby on Rails@<fn>{rails}などのWebアプリケーションフレームワークが不可欠です。もしもフレームワークの助けを借りずに、フルスクラッチでWebサイトを構築しなければならないとしたら…何十倍ものコーディングが発生し、しかもそのほとんどは車輪の再発明に終わるでしょう。効率的にWebサイトを作りたいプログラマ向けに、書店の技術書コーナーにはさまざまなフレームワークの本があふれかえっています。こうしたフレームワークを使ってWebサービスを構築することは、すでに常識なのです。

//footnote[rails][@<href>{http://rubyonrails.org/}]

OpenFlowコントローラもフルスクラッチで作るのは大変です。OpenFlowの標準仕様はCで書いてあるので、まずはCが読めることが必須です。仕様が理解できたら、開発に使うプログラミング言語向けにライブラリを書き、その上にコントローラを構築し…考えただけでひと仕事です。動作テストのためのツール類も自分で準備しなければなりません。

そこで、OpenFlowコントローラフレームワークの出番です。Web業界ほどではありませんが、世の中にはすでに主要なプログラミング言語向けのOpenFlowコントローラフレームワークがそろいつつあります。これらを使えば効率的にコントローラを開発できます。またいくつかのフレームワークは開発やデバッグに便利なツールをフレームワークの一部として提供しています。使わない手はありませんね。

@<table>{frameworks}に主なOpenFlowコントローラフレームワークを挙げます。いずれもいわゆるオープンソースソフトウェアで、それぞれ対応する言語が異なります。

//table[frameworks][主なOpenFlowコントローラフレームワーク]{
名前		開発言語		開発元										ライセンス
----------------------------------------------------------------------------------
Trema		Ruby			Tremaプロジェクト							GPL2
NOX			C++				Nicira、スタンフォード大、UC バークレイ		GPL3
POX			Python			UCバークレイ								GPL3
Floodlight	Java			Big Switch Networks Inc.					Apache
//}

それでは、それぞれの特長を詳しく見ていきましょう。

== Trema

TremaはRuby用のOpenFlowコントローラフレームワークです(@<img>{trema})。GPLバージョン2ライセンスのフリーソフトウェアです。

//image[trema][Tremaのサイト(@<href>{http://trema.github.com/trema})][width=12cm]

ターゲット言語がRubyであることからもわかるとおり、Tremaの最大の特長は実行速度よりも開発効率に重きを置いていることです。たとえば、Tremaを使うと他のフレームワークに比べて大幅に短いコードでコントローラを実装できます。@<list>{trema_hub}はTremaで書いたコントローラの一例ですが、たった14行のコードだけでハブとして動作する完全なコントローラが書けます。

//list[trema_hub][Tremaで書いたコントローラ(ハブ)の例]{
class RepeaterHub < Controller
  def packet_in datapath_id, message
    send_flow_mod_add(
      datapath_id,
      :match => ExactMatch.from( message ),
      :actions => SendOutPort.new( OFPP_FLOOD )
    )
    send_packet_out(
      datapath_id,
      :packet_in => message,
      :actions => SendOutPort.new( OFPP_FLOOD )
    )
  end
end
//}

開発効率向上のしくみとして、Tremaにはコントローラ開発に役立つツールが充実しています。その中でも強力なツール、ネットワークエミュレータはコントローラのテストに便利です。これはノートPC1台でコントローラを開発できるというもので、仮想スイッチと仮想ホストを組み合わせた任意の仮想環境上でコントローラを実行できます。もちろん、こうして開発したコントローラは実際のネットワーク上でもそのまま動作します。

== NOX

NOXはOpenFlowの生まれ故郷スタンフォード大で開発されたもっとも古いフレームワークで、C++に対応しています(@<img>{nox})。ライセンスはGPLバージョン3のフリーソフトウェアです。

//image[nox][NOXのサイト(@<href>{http://www.noxrepo.org/nox/about-nox/})][width=12cm]

NOXの長所はユーザ層の厚さです。OpenFlowの登場直後から開発しており、メーリングリストではOpenFlow仕様を作った研究者など、SDNの主要な関係者が活発に議論しています。また歴史が古いため、Webで情報を集めやすいという利点もあります。

最後にNOXのサンプルコードとして、Tremaと同じくハブを実装した例を紹介します(@<list>{nox_hub})。

//list[nox_hub][NOXで書いたコントローラ(ハブ)の例]{
#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include "assert.hh"
#include "component.hh"
#include "flow.hh"
#include "packet-in.hh"
#include "vlog.hh"

#include "netinet++/ethernet.hh"

namespace {

using namespace vigil;
using namespace vigil::container;

Vlog_module lg("hub");

class Hub 
    : public Component 
{
public:
     Hub(const Context* c,
         const json_object*) 
         : Component(c) { }

    void configure(const Configuration*) {
    }

    Disposition handler(const Event& e)
    {
        const Packet_in_event& pi = assert_cast<const Packet_in_event&>(e);
        uint32_t buffer_id = pi.buffer_id;
        Flow flow(pi.in_port, *(pi.get_buffer()));

        if (flow.dl_type == ethernet::LLDP){
            return CONTINUE;
        }

        ofp_flow_mod* ofm;
        size_t size = sizeof *ofm + sizeof(ofp_action_output);
        boost::shared_array<char> raw_of(new char[size]);
        ofm = (ofp_flow_mod*) raw_of.get();

        ofm->header.version = OFP_VERSION;
        ofm->header.type = OFPT_FLOW_MOD;
        ofm->header.length = htons(size);
        ofm->match.wildcards = htonl(0);
        ofm->match.in_port = htons(flow.in_port);
        ofm->match.dl_vlan = flow.dl_vlan;
        ofm->match.dl_vlan_pcp = flow.dl_vlan_pcp;
        memcpy(ofm->match.dl_src, flow.dl_src.octet, sizeof ofm->match.dl_src);
        memcpy(ofm->match.dl_dst, flow.dl_dst.octet, sizeof ofm->match.dl_dst);
        ofm->match.dl_type = flow.dl_type;
        ofm->match.nw_src = flow.nw_src;
        ofm->match.nw_dst = flow.nw_dst;
        ofm->match.nw_proto = flow.nw_proto;
        ofm->match.tp_src = flow.tp_src;
        ofm->match.tp_dst = flow.tp_dst;
        ofm->cookie = htonl(0);
        ofm->command = htons(OFPFC_ADD);
        ofm->buffer_id = htonl(buffer_id);
        ofm->idle_timeout = htons(5);
        ofm->hard_timeout = htons(5);
        ofm->priority = htons(OFP_DEFAULT_PRIORITY);
        ofm->flags = htons(0);
        ofp_action_output& action = *((ofp_action_output*)ofm->actions);
        memset(&action, 0, sizeof(ofp_action_output));
        action.type = htons(OFPAT_OUTPUT);
        action.len = htons(sizeof(ofp_action_output));
        action.port = htons(OFPP_FLOOD);
        action.max_len = htons(0);
        send_openflow_command(pi.datapath_id, &ofm->header, true);
        free(ofm);

        if (buffer_id == UINT32_MAX) {
            size_t data_len = pi.get_buffer()->size();
            size_t total_len = pi.total_len;
            if (total_len == data_len) {
                send_openflow_packet(pi.datapath_id, *pi.get_buffer(), 
                        OFPP_FLOOD, pi.in_port, true);
            }
        }

        return CONTINUE;
    }

    void install()
    {
        register_handler<Packet_in_event>(boost::bind(&Hub::handler, this, _1));
    }
};

REGISTER_COMPONENT(container::Simple_component_factory<Hub>, Hub);

}
//}

== POX

POXはNOXから派生したプロジェクトで、Pythonでのコントローラ開発に対応したフレームワークです(@<img>{pox})。ライセンスはGPLバージョン3のフリーソフトウェアです。

//image[pox][POXのサイト(@<href>{http://www.noxrepo.org/pox/about-pox/})][width=12cm]

POXの特長は実装がPure Pythonであるため、Linux/Mac/WindowsのいずれでもOSを問わず動作することです。まだまだ若いプロジェクトであるためサンプルアプリケーションの数は少ないものの、Pythonプログラマには注目のプロジェクトです。

最後にPOXのサンプルコードとして、同じくハブを実装した例を紹介します(@<list>{pox_hub})。

//list[pox_hub][POXで書いたコントローラ(ハブ)の例]{
from pox.core import core
import pox.openflow.libopenflow_01 as of

class RepeaterHub (object):
  def __init__ (self, connection):
    self.connection = connection
    connection.addListeners(self)

  def send_packet (self, buffer_id, raw_data, out_port, in_port):
    msg = of.ofp_packet_out()
    msg.in_port = in_port
    if buffer_id != -1 and buffer_id is not None:
      msg.buffer_id = buffer_id
    else:
      if raw_data is None:
        return
      msg.data = raw_data
    action = of.ofp_action_output(port = out_port)
    msg.actions.append(action)
    self.connection.send(msg)

  def act_like_hub (self, packet, packet_in):
    self.send_packet(packet_in.buffer_id, packet_in.data,
                     of.OFPP_FLOOD, packet_in.in_port)

  def _handle_PacketIn (self, event):
    packet = event.parsed
    if not packet.parsed:
      return
    packet_in = event.ofp # The actual ofp_packet_in message.
    self.act_like_hub(packet, packet_in)

def launch ():
  def start_switch (event):
    RepeaterHub(event.connection)
  core.openflow.addListenerByName("ConnectionUp", start_switch)
//}

== Floodlight

FloodlightはJava用のフレームワークです(@<img>{floodlight})。ライセンスはApacheのフリーソフトウェアです。

//image[floodlight][Floodlightのサイト(@<href>{http://www.noxrepo.org/pox/about-pox/})][width=12cm]

Floodlightの特長はずばり、プログラマ人口の多いJavaを採用していることです。最近は大学のカリキュラムで最初にJavaを学ぶことが多いため、大部分の人にとって最もとっつきやすいでしょう。また実装がPure Javaであるため、POXと同じくOSを問わず動作するという利点もあります。

最後にFloodlightのサンプルコードとして、同じくハブを実装した例を紹介します(@<list>{floodlight_hub})。

//list[floodlight_hub][Floodlightで書いたコントローラ(ハブ)の例]{
package net.floodlightcontroller.hub;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;
import net.floodlightcontroller.core.FloodlightContext;
import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.IOFMessageListener;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import org.openflow.protocol.OFMessage;
import org.openflow.protocol.OFPacketIn;
import org.openflow.protocol.OFPacketOut;
import org.openflow.protocol.OFPort;
import org.openflow.protocol.OFType;
import org.openflow.protocol.action.OFAction;
import org.openflow.protocol.action.OFActionOutput;
import org.openflow.util.U16;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Hub implements IFloodlightModule, IOFMessageListener {
    protected static Logger log = LoggerFactory.getLogger(Hub.class);
    protected IFloodlightProviderService floodlightProvider;

    public void setFloodlightProvider(IFloodlightProviderService floodlightProvider) {
        this.floodlightProvider = floodlightProvider;
    }

    @Override
    public String getName() {
        return Hub.class.getPackage().getName();
    }

    public Command receive(IOFSwitch sw, OFMessage msg, FloodlightContext cntx) {
        OFPacketIn pi = (OFPacketIn) msg;
        OFPacketOut po = (OFPacketOut) floodlightProvider.getOFMessageFactory()
                .getMessage(OFType.PACKET_OUT);
        po.setBufferId(pi.getBufferId())
            .setInPort(pi.getInPort());

        OFActionOutput action = new OFActionOutput()
            .setPort((short) OFPort.OFPP_FLOOD.getValue());
        po.setActions(Collections.singletonList((OFAction)action));
        po.setActionsLength((short) OFActionOutput.MINIMUM_LENGTH);

        if (pi.getBufferId() == 0xffffffff) {
            byte[] packetData = pi.getPacketData();
            po.setLength(U16.t(OFPacketOut.MINIMUM_LENGTH
                    + po.getActionsLength() + packetData.length));
            po.setPacketData(packetData);
        } else {
            po.setLength(U16.t(OFPacketOut.MINIMUM_LENGTH
                    + po.getActionsLength()));
        }
        try {
            sw.write(po, cntx);
        } catch (IOException e) {
            log.error("Failure writing PacketOut", e);
        }

        return Command.CONTINUE;
    }

    @Override
    public boolean isCallbackOrderingPrereq(OFType type, String name) {
        return false;
    }

    @Override
    public boolean isCallbackOrderingPostreq(OFType type, String name) {
        return false;
    }

    @Override
    public Collection<Class<? extends IFloodlightService>> getModuleServices() {
        return null;
    }

    @Override
    public Map<Class<? extends IFloodlightService>, IFloodlightService>
            getServiceImpls() {
        return null;
    }

    @Override
    public Collection<Class<? extends IFloodlightService>>
            getModuleDependencies() {
        Collection<Class<? extends IFloodlightService>> l = 
                new ArrayList<Class<? extends IFloodlightService>>();
        l.add(IFloodlightProviderService.class);
        return l;
    }

    @Override
    public void init(FloodlightModuleContext context)
            throws FloodlightModuleException {
        floodlightProvider =
                context.getServiceImpl(IFloodlightProviderService.class);
    }

    @Override
    public void startUp(FloodlightModuleContext context) {
        floodlightProvider.addOFMessageListener(OFType.PACKET_IN, this);
    }
}
//}

== どれを選べばいい？

では、いくつもあるフレームワークのうちどれを使えばいいでしょうか？まっとうな答は「開発メンバーが使い慣れた言語をサポートするフレームワークを使え」です。つまり、RubyプログラマのチームであればTrema一択ですし、C++プログラマならNOX一択ということです。

これを裏付けるものとして、名著『Code Complete 第2版 - 完全なプログラミングを目指して(上下巻)』@<fn>{codecomplete}に説得力のあるデータがあります。

//footnote[codecomplete][Steve McConell著／日経BP刊。]

//quote{
プログラマの生産性は、使い慣れた言語を使用したときの方が、そうでない言語を使用したときよりも向上する。COCOMO IIという見積もりモデルがはじき出したデータによると、3年以上使っている言語で作業しているプログラマの生産性は、ほぼ同じ経験を持つプログラマが始めての言語を使っている場合の生産性を、約30%上回る(Boehm et al. 2000)。これに先立って行われたIBMの調査では、あるプログラミング言語での経験が豊富なプログラマは、その言語にほとんど経験のないプログラマの3倍以上の生産性があることがわかっている(Walston and Felix 1977)。
//}

これはごくあたりまえの原則ですが、プログラミングの現場では無視されていることが少なくありません。「上司が使えと言ったから」「流行っているらしいから」という理由でなんとなくフレームワークを選び、そしてプロジェクトが炎上するというケースが後をたちません。かならず、プログラマ自身が慣れたプログラミング言語で作るべきです。

一方で、プログラマがいくつもの言語に習熟していた場合、それらの言語の間に明らかな生産性の差が出てくるのも事実です。CやC++のような明示的にメモリ管理が必要な低水準言語と、これにガベージ・コレクションを付け加えたJavaやC#のような言語、また最近のRubyやPythonのように、さらに高レベルで記述できるスクリプティング言語では、生産性と品質に何十倍もの差が出ます。さきほどの『Code Complete』をふたたび引きましょう。

//quote{
高級言語を使って作業するプログラマの生産性と品質は、低水準言語を使用するプログラマより高い。(中略) C言語のように、ステートメントが仕様どおりに動いたからといって、いちいち祝杯をあげる必要がなければ、時間が節約できるものというものだ。そのうえ、高級言語は低水準言語よりも表現力が豊かである。つまり、1行のコードでより多くの命令を伝えることができる。
//}

このことは、今まで見てきたハブ実装のコード行数を比べても明らかです(@<img>{comparison})。

//image[comparison][主なフレームワークでハブを実装したときのコード行数を比較][width=12cm]

//noindent
Trema(Ruby)やPOX(Python)などスクリプティング言語を採用するフレームワークでは短い行数で実装できていますが、NOX(C++)やFloodlight(Java)など従来の言語を採用するフレームワークでは一気に行数がはねあがっています。とくに、最も短いTrema(14行)と最も長いFloodlight(111行)を比べるとその差は8倍にもなります。単純には言えませんが、行数だけで見るとTremaはFloodlightの1/8の労力で同じ機能を実装できるのです。

みなさんだったら、どのフレームワークを選びますか？

===[column] @<ruby>{取間,とれま}先生曰く：どうなる！？OpenFlowコントローラ開発の今後

私の予想では、OpenFlowコントローラフレームワークはかつてのWebアプリケーションフレームワークと同じ道をたどるのではと思っています。歴史をさかのぼると、1990年代〜2000年代初頭はJava用フレームワーク全盛期でした。無数のJava用フレームワークが雨後の竹の子のように登場し、Java EE、JSP、JSFなど新しい技術も次々と出てきました。IDEが自動生成する長いコードやXMLファイルと格闘しながら、次々と登場する新しい仕様を理解してWebアプリケーションを書くのは至難の業でした。しかし2004年、RubyのWebアプリケーションフレームワークであるRuby on Railsの登場によってWeb業界は一変します。Javaによる鈍重な実装は避け、なるべく短いコードで書こうという考え方がWeb業界を席巻したのです。この流れは、「コードが長くなるフレームワーク」の代名詞であったJavaの世界にも取り入れられ、最近のDjangoやPlayなど近代的なフレームワークを産んできました。

OpenFlowコントローラフレームワークはまだまだ黎明期にあります。TremaやPOXのように最近の考えかたを取り入れたフレームワークはありますが、とくに海外ではNOXやFloodlightなど旧来的なフレームワークが主流を占めています。しかし、ネットワーク業界でもスクリプティング言語を使えるプログラマが増えれば、古い設計のフレームワークを使うプログラマよりも何倍もの生産性をあげることができるようになるでしょう。そしてこの考え方が順調に浸透していけば、さまざまな言語で生産性の高いフレームワークが登場するはずです。

===[/column]

== その他のツール(Oflops)

OflopsはOpenFlowコントローラとスイッチのためのマイクロベンチマークです。コントローラ用のベンチマークCbenchとスイッチ用のベンチマークOFlopsを提供します。スイッチを作る機会はめったにないのでここではコントローラのベンチマークであるCbenchについて説明します。

Cbenchは「1秒あたりにコントローラが出せるFlow Modの数」を計測します。Cbenchはスイッチのふりをしてコントローラに接続し、コントローラにPacket Inを送ります。これに反応したコントローラからのFlow Modの数をカウントし、スコアとします。このスコアが大きいコントローラほど「速い」とみなすのです。

Cbenchは次の2種類のベンチマークをサポートします。

//noindent
@<em>{レイテンシモード}

 1. Packet Inをコントローラに送り、
 2. コントローラからFlow Modが帰ってくるのを待ち、
 3. これを繰り返す

//noindent
@<em>{スループットモード}

 1. Flow Modを待たずにPacket Inを送信し続け、
 2. Flow Modが返信されたらカウントする。


=== Cbenchの実行例(Tremaの場合)   

TremaはCbenchおよびCbenchと接続できるコントローラを含むので、この2つのベンチマークを簡単に実行できます。次のコマンドは、Cbenchをレイテンシモードとスループットモードで実行し結果を表示します(Tremaのインストール方法は続く@<chap>{openflow_framework_trema}で説明します)。

//cmd{
% ./build.rb cbench
./trema run src/examples/cbench_switch/cbench-switch.rb -d
/home/yasuhito/play/trema/objects/oflops/bin/cbench --switches 1 --loops 10 --delay 1000
cbench: controller benchmarking tool
   running in mode 'latency'
   connecting to controller at localhost:6633 
   faking 1 switches :: 10 tests each; 1000 ms per test
   with 100000 unique source MACs per switch
   starting test with 1000 ms delay after features_reply
   ignoring first 1 "warmup" and last 0 "cooldown" loops
   debugging info is off
1   switches: fmods/sec:  10353   total = 10.352990 per ms 
1   switches: fmods/sec:  10142   total = 10.141990 per ms 
1   switches: fmods/sec:  10260   total = 10.259990 per ms 
1   switches: fmods/sec:  10736   total = 10.734497 per ms 
1   switches: fmods/sec:  10884   total = 10.883989 per ms 
1   switches: fmods/sec:  10752   total = 10.751989 per ms 
1   switches: fmods/sec:  10743   total = 10.742989 per ms 
1   switches: fmods/sec:  10828   total = 10.827989 per ms 
1   switches: fmods/sec:  10454   total = 10.453990 per ms 
1   switches: fmods/sec:  10642   total = 10.641989 per ms 
RESULT: 1 switches 9 tests min/max/avg/stdev = 10141.99/10883.99/10604.38/245.53 responses/s
./trema killall
./trema run src/examples/cbench_switch/cbench-switch.rb -d
/home/yasuhito/play/trema/objects/oflops/bin/cbench --switches 1 --loops 10 --delay 1000 --throughput
cbench: controller benchmarking tool
   running in mode 'throughput'
   connecting to controller at localhost:6633 
   faking 1 switches :: 10 tests each; 1000 ms per test
   with 100000 unique source MACs per switch
   starting test with 1000 ms delay after features_reply
   ignoring first 1 "warmup" and last 0 "cooldown" loops
   debugging info is off
1   switches: fmods/sec:  36883   total = 36.761283 per ms 
1   switches: fmods/sec:  36421   total = 36.398433 per ms 
1   switches: fmods/sec:  37286   total = 37.174106 per ms 
1   switches: fmods/sec:  36559   total = 36.526637 per ms 
1   switches: fmods/sec:  36072   total = 36.007331 per ms 
1   switches: fmods/sec:  34130   total = 33.993855 per ms 
1   switches: fmods/sec:  32119   total = 32.086016 per ms 
1   switches: fmods/sec:  33733   total = 33.533876 per ms 
1   switches: fmods/sec:  33270   total = 33.262582 per ms 
1   switches: fmods/sec:  32119   total = 32.107056 per ms 
RESULT: 1 switches 9 tests min/max/avg/stdev = 32086.02/37174.11/34565.54/1866.96 responses/s
./trema killall
//}

====[column] @<ruby>{取間,とれま}先生曰く：Cbenchの注意点

Cbench のスコアを盲信しないようにしてください。現在、いくつかの OpenFlow コントローラフレームワークは Cbench のスコアだけを競っているように見えます。たとえば Floodlight は 1 秒間に 100 万発の Flow Mod を打てると宣伝しています。これはなかなかすごい数字です。きちんと計算したわけではないですが、スレッドを駆使してめいっぱい I/O を使い切るようにしなければなかなかこの数字は出ません。とにかくすごい。でも、この数字にはまったく意味がありません。

Flow Mod を一秒間に 100 万発打たなければならない状況を考えてみてください。それは、Packet In が一秒間に 100 万発起こる状況ということになります。Packet In が一秒間に 100 万発起こるとはどういうことでしょうか? スイッチに何らかのフローが設定されているが入ってきたパケットがまったくそれにマッチせず、どうしたらいいかわからないパケットがすべてコントローラへやってくる、これが一秒間に 100 万回起こるということです。何かがまちがっていると思えないでしょうか？

コントローラが Packet In を何発さばけるかという性能は、極端に遅くない限りは重要ではありません。データセンターのように、どこにどんなマシンがありどういう通信をするか把握できている場合は、フローをちゃんと設計していれば Packet In はそんなに起こらないからです。力技で Packet In をさばくよりも、いかに Packet In が起こらないネットワーク設計やフロー設計をするかの方がずっと大事です。

Cbench のようなマイクロベンチマークでは、測定対象が何でその結果にはどんな意味があるか？を理解しないと針小棒大な結論を招きます。Cbench のスコアは参考程度にとどめましょう。

====[/column]

== まとめ

本章では現在利用できる主なOpenFlowコントローラフレームワークを紹介しました。すでに主要な言語のフレームワークがそろっているので、自分の使う言語に合わせてフレームワークを選択できます。

もし生産性の高いフレームワークをお望みであればTremaかPOXを選択してください。流れの速いSDN業界では、実行効率よりも「いかに早くサービスインできるか」という生産性の方がずっと重要だからです。

続く第II部では、Tremaを使ったOpenFlowプログラミングを学習します。Rubyの基礎から解説しますので、Rubyが初めてのプログラマでも読み進められるようにしてあります。
