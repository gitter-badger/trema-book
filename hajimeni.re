= はじめに

夕食前に自宅で原稿を書いていると、5歳になる長男が保育所で拾ってきた石を見せに来たものです。見ると何の変哲もない灰色の石ころですが、本人にとっては大切なものだったようで、ぜったいに捨てないでね、と念を押してきます。そのうちこうした"たからもの"の石ころが机の上に5個、6個、とだんだんたまっていきました。

思えば、この本もそんな石ころ集めから始まりました。ソフトウェア工学の大家、ジェラルド・ワインバーグの最近の著書に『ワインバーグの文章読本 - 自然石構築法(翔泳社)』があります。これは、多作なワインバーグじきじきの書き方指南で、メモや引用といった文章の断片を"石"と呼び、この石を集めたり並べ替えたりすることで立派な石造りの建物やアーチを組み上げる、つまり一冊書き上げる方法を説明した名著です。「よし、まずは石を集めるか」。自然石構築法を知った私は、さっそくこれに取りかかりました。

本書の執筆にあたって集めた石は、すぐに膨大な数になりました。OpenFlowの仕様書や論文はもちろん、職場での雑談やソースコードの切れはし、大学での講義、メーリングリストでみつけた投稿、雑誌やWebの連載記事@<fn>{sd}、気晴らしの読書やテレビ、昔よく聴いた夕方のラジオ番組、iPhoneに残っていた写真、とにかく大小いろんな色の石を、そこらじゅうから集めまくっては並べかえる毎日でした。

//footnote[sd][本書のいくつかの章は、SoftwareDesign2011年12月号〜2012年5月号に連載の『こんな夜中にOpenFlowでネットワークをプログラミング！』を大幅に加筆修正したものです。]

こうして集めた石の中には、けっして独力では収集できなかった貴重な石も多くあります。本書の原稿をgithubで公開したところ@<fn>{github}、たくさんの方々から100件以上のレビューをいただきました。石をくれる息子のように、こちらからお願いしなくとも善意でコメントを送ってくれる方が大勢いたのです。こうしたいくつかの石は、本書の重要な部分を占めています。とくに@<chap>{datacenter_wakame}のTremaを使ったデータセンター、@<chap>{google}のGoogleでのOpenFlowユースケース、そして@<chap>{diy_switch}のOpenFlowスイッチ自作法は、普通では絶対に手に入らない金ピカの宝石をいろいろな人からいただいたおかげで、何とか形にすることができました。@<br>{}

//footnote[github][@<href>{https://github.com/yasuhito/trema-book}]

"石集め"協力者のみなさん (敬称略、順不同)：壬生 亮太、宮下 一博、石井 秀治、金海 好彦、@<tt>{@stereocat}、高田 将司、富永 光俊、沼野 秀吾、富田 和伸、前川 峻志、園田 健太郎、大山 裕泰、藤原 智弘、空閑 洋平、佛崎 雅弘、阿部 博、小谷 大祐、笹生 健、山口 啓介、森部 正二朗、高橋 大輔、山本 宏、橋本 匡史、小泉 佑揮、廣田 悠介、長谷川 剛、千葉 靖伸、須堯 一志、下西 英之、角 征典、高橋 征義、早水 悠登、弓削 吉正、高宮 友太郎、高宮 葵、高宮 優子。とくに、以下の方々には特別感謝です：@<tt>{@SRCHACK}、栄 純明、坪井 俊樹、黄 恵聖、山崎 泰宏、取口敏憲。@<br>{}

石はただ集めるだけではなく、必要な形に整形したり隙間を埋めたり、さらには磨き上げたりする必要があります。実は、私はもともとネットワークの専門家ではないので、ネットワークに特有な考え方や用語の説明に苦労しました。そうした部分を補ってくれたのが、ネットワーク研究者でありこの本の共著者でもある鈴木一哉氏でした。また私なりにも、石1つひとつの正確さにはベストを尽くしました。たとえばとある章に、酔っぱらいが三軒茶屋(東京都)から武蔵小杉(神奈川県)まで歩いて帰るというエピソードがありますが、私は本当に歩けることを体を張って検証しました。

執筆と並行してやったのが、本書で取り上げたOpenFlowプログラミングフレームワークTremaの開発です。Tremaはもともと、その場しのぎで書いたソフトウェアを出発として、大量のテストコード、リポジトリサーバーのクラッシュ、@<tt>{svn}から@<tt>{git}への乗り換え、二度の忘年会、いきなりの人事異動、インドとの長距離電話会議、を経験して鍛えられてきたフレームワークです。ずいぶんと曲折を経たものですが、まさしく石のような意思で乗り切りました。

私は、いいフレームワークといい本ができあがったと思っています。私のいいか悪いかの判断基準は、「まだ誰もやっていないことは、いいことだ」という単純な考えに基いています。Tremaみたいなフレームワークは、まだ誰もやってない。OpenFlowを正面きってここまで扱っている本は、まだ他にはない。だからどちらも私にとっては"いいもの"なのです。もちろん、本当にいいか悪いかは、読者のみなさんのご判断におまかせすることにします。

//flushright{
2012年11月14日@<br>{}高宮 安仁 (@<tt>{@yasuhito})
//}