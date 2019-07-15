超小型 wifi コントローラー ボード サポートページ

## 概要
KOSHINの電池式灯油ポンプ[EP-105](https://www.koshin-ltd.jp/products/1144.html)の内部にハンダ付け不要で組み込み可能な超小型 wifi コントローラー ボードです。スマホで予約が可能なタイマー式水やり機として使うことができるようになります(**改造によりメーカー保証対象外となりますので、自己責任でお願いします！**)。

- ベランダ菜園など、電源や水道がない場所で使う際に便利です
- 使い方にもよりますが、電池は１週間程度しか持ちませんので旅行などの短期間だけ家を空ける際にお使いになるのがよいと思います
- 水やり時刻は１時間刻みで複数設定できます
- 時差は１日１５秒程度は発生するようです
- ニッケル水素充電池でも作動します（単三電池２本）
- ESP32に詳しい方はご自分でプログラムを書き換えて単なるwifiコントローラーとして使うことも可能です（電源は3V。内部で3.3Vに昇圧しています。詳しくは[回路図](https://github.com/okameinko/typctr/blob/master/typctr/circuite/touyu_pomp.pdf)および基盤のシルクをご覧ください）

## wifiコントローラー ボードの組み込みかた
EP-105電池ケース内部にあるプラスチックのツメの内側にマイナスドライバーの先を当てます。つぎにドライバーの棒の部分を壁側にあてて、テコの原理の要領でツメを広げて上下のケースを外します。ケースの間に隙間ができたらそれを拡げてケースを完全に外します。

![IMG_20190630_111006](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_111006.jpg)

![IMG_20190630_111028](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_111028.jpg)

![IMG_20190630_111040](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_111040.jpg)

ポンプ部分が電源ケーブルでつながっています。ポンプ側からのケーブルが二本にわかれた部分から1.2センチ程度はなれた部分をニッパーで切断します。切断されて４本になったケーブルの先のビニールの被覆を、ワイヤーストリッパーで3ミリほど剥いておきます。切り離されたポンプ部分はいったん別の場所においておきます。

![IMG_20190630_111109](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_111109.jpg)

![IMG_20190630_111113](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_111113.jpg)

電池ケース部分の右下にあるプラスチックの突起部分を二つともニッパーで切り取ります。

![IMG_20190630_112842](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_112842.jpg)

![IMG_20190630_112859](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_112859.jpg)

![IMG_20190630_112928](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_112928.jpg)

wifiコントローラー ボードの４つのスクリューターミナルをプラスドライバーをつかってあらかじめ緩めておきます。電池ボックスのスイッチ側（電池はマイナス）から伸びているケーブルの露出された導線の先を、写真をよく見ながら一番はじにあるスクリューターミナルへピンセットをつかって差し込み、プラスドライバーでスクリューを締め付けて固定します。固定したケーブルをピンセットでしっかり引っ張っても外れないかどうかを確認します。（★***ケーブルおよび取付位置を間違えないように細心の注意をはらってください。間違ったつなぎ方をするとwifiコントローラーが壊れます***★）

![IMG_20190630_113048](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113048.jpg)

![IMG_20190630_113029](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113029.jpg)

![IMG_20190630_113100](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113100.jpg)

電池ボックスのプラス側から出ているケーブルを先を、となりのスクリューターミナルに同様の手順でとりつけます。（***★ケーブルおよび取付位置を間違えないように細心の注意をはらってください。間違ったつなぎ方をするとwifiコントローラーが壊れます。★***）

![IMG_20190630_113319](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113319.jpg)

ポンプ側から出ているケーブルの先を、ピンセットをつかってスクリューターミナルに差し込みます。このとき、スクリューターミナルの内側同士と外側同士が同じ色になるように差し込みます。スクリューをドライバーで締め付けて、ピンセットでしっかりひっぱってもケーブルがとれないことを確認します。***（★ケーブルおよび取付位置を間違えないように細心の注意をはらってください★）***

![IMG_20190630_113627](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113627.jpg)

写真をよく見ながら、ケーブルが正しいターミナルに接続されているかどうか再確認します。間違ったつなぎ方をするとwifiコントローラー ボードが壊れます。

ケーブルに無理な力が入らないように注意しながら、ポンプ部分をもとの場所に納めます。収まったら、wifiコントローラーの黒い板が電池ボックスとは反対側になるようにwifiコントローラーを位置を合わせて、指で押さえます。

![IMG_20190630_113649](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113649.jpg)

![IMG_20190630_113703](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113703.jpg)

![IMG_20190630_113719](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113719.jpg)

外しておいたカバーを上からかぶせます。wifiコントローラー全体がケース内に収まるように調整しながら、ゆっくりとカバー全体を押し付けてカチッとおとがするまではめ込みます。

![IMG_20190630_113741](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113741.jpg)

![IMG_20190630_113749](https://github.com/okameinko/typctr/blob/master/typctr/img/IMG_20190630_113749.jpg)

スイッチをオフにした状態で電池を入れ、電池ボックスのフタを閉じれば完成です。

## タイマー設定の仕方

１.ポンプ本体のスイッチを入れます

２.スマホのwifi設定を開き、"ESP32-WiFi"というアクセスポイントに接続します

３.スマホのブラウザを起動します

４.以下のurlを開きます

http://192.168.0.2/

5.「水やり設定」という画面が開くので、あとは指示に従って設定を行います。タイマー作動後は一定間隔でLEDが点滅します。LEDが点滅しなくなったら電池切れです。

6.使い終わったら液漏れを防ぐため電池は抜いてください。また、ポンプは錆びをさけるためにしっかり乾燥させることをお勧めします。

## 回路図

 回路図はこの[リンク](https://github.com/okameinko/typctr/blob/master/typctr/circuite/touyu_pomp.pdf)です。特徴は以下の通り。

- ポンプ本体の単三電池二本を3.3Vに昇圧してESP32を作動
- モーターのオンオフにはパワーFETを使用
- 作動確認用のLEDを表面と裏面に実装
- スクリューターミナルの使用によりはんだ付け不要
- 書き込み用スルーホール（GND,EN,RX,TX,3V3,IO0）あり。ピンソケットやピンヘッダをはんだ付けしてしまうとEP-105の内部に入りきれなくなってしまいますのでご注意ください。
- Y1（32.768Khzクリスタル）、C11、C12、R7は実装してありません。自作派の方はY1のスルーホールを普通のGPIO(32,33)として使ってみてはいかがでしょうか。

## プログラム

arduino IDEで開発しています。主な工夫は以下の通り。

- ソフトウェアアクセスポイントによりルーター不要でスマホと接続可能
- タイマー動作中はディープスリープで省電力化
- ディープスリープ中もLED点滅で動作確認できる
- PWMでチャージポンプを駆動することでFETに十分なゲート電圧を供給

ソースコードは[こちら](https://github.com/okameinko/typctr/blob/master/typctr/src)。

コンパイルには、

[Arduino core for ESP32 WiFi chip](https://github.com/espressif/arduino-esp32)

[ESP32 Filesystem Uploader](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)

[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)

[ulptool v2.3.0](https://github.com/duff2013/ulptool)

のインストールが必要です。(たぶんpython2.7インストールしてpathが通っていないと動きません。)

## 書き込み回路

[このあたり](https://ht-deko.com/arduino/esp-wroom-02.html#13_08)を参考にして作ってください。ENTとGNDの間に10uFの電解コンデンサを入れた方が自動リセットが安定すると思います。製作者の環境ではボードに電池をつなぎ、書き込みのたびにUSBケーブルを外さないと書き込みできませんでした。なお書き込み用スルーホールにソケットやピンコネクタをはんだ付けしてしまうとEP-105の内部に入りきれなくなってしまいますのでご注意ください。