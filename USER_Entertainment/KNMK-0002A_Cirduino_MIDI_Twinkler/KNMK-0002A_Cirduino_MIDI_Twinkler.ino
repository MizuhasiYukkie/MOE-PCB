/*!
 * KNMK-0002A Cirduino向けUSB-MIDI遊びコード 1.0
 *
 * Copyright (c) 2022 Mizuhasi Yukkie
 * This software is released under the MIT license.
 * see https://opensource.org/licenses/MIT
*/

/*
 * This program requires the folloing two libraries.
 * このコードを使用するためには、下記のライブラリをインストールしてください。Githubからのダウンロードがよくわからないという方は、
 * スケッチ→ライブラリをインクルード→ライブラリを管理→検索窓からライブラリ名を検索（”ADCTouch”など）→インストールがおすすめです。
 * 
 * 「Adafruit_NeoPixel」 https://github.com/adafruit/Adafruit_NeoPixel
 * 
 * ツール→ボード："Arduino Leonardo"を選択
 * ツール→シリアルポート：「USBで接続済みの萌基板のポート」（ArduinoLeonardoと表示されます）を選択
 * 「マイコンボードに書き込む」を実行するとここに書いてあるプログラムが書き込まれます。
 * 
 * ベロシティを検出したりCCを検出したり…はしていません。ノートオンが来たらとりあえず光る、それだけです。
 * コメントアウトしてあるシリアル出力を解除すると受信しているMIDIメッセージが見えると思います。
 * ほしいデータを捕まえていろいろ遊んでみてください。上手く作れば光り方をMIDIでコントロール出来る、
 * いわゆるシーケンス動作用のプログラムにも変えられると思います。 
 */

#include "MIDIUSB.h"

#include <Adafruit_NeoPixel.h>

#define LED1 0
#define LED2 1
#define LED3 2
#define LED4 3
#define LED5 4
#define LED6 5
#define LED7 6
#define RGBLED_NUM 7
#define RGBLED_PIN 6 // NeoPixel pin
#define BLINK_LED 13 // Normal LED pin


Adafruit_NeoPixel pixels(RGBLED_NUM, RGBLED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t BRT_LEVEL=1;//明るさレベル0-3　起動時は1から開始
uint8_t BRT_VAL[]={10,5,65,175};//段階ごとの明るさ詳細値　お好みで変更を。
uint8_t BRT_RND[]={40,150,155,255};//ランダムで変更する値（きらきら効果）
uint8_t V_raw[10];//各LEDごとの明るさ実際の値（自動追従）
uint8_t H_raw[10];//LEDごとの色環 0-255
uint8_t S_raw[7];//LEDごとの彩度 0-255

void setup() {
  pinMode(BLINK_LED, OUTPUT);
  digitalWrite(BLINK_LED, LOW);//ON
  Serial.begin(115200);
  pixels.begin();

  //明るさ０で初期化
  for(int i=0;i<10;i++){
    V_raw[i]=0;
    H_raw[i]=0;
    S_raw[i]=0;
  }
  digitalWrite(BLINK_LED, HIGH);//OFF
}

int B_brt;//背面LEDの明るさ変数

int note_LED;//ランダムでオンにするLED 0-5
int last_note;//最後のノートと被ってたらもう一度ランダムし直す

int note_on_flag=0;

float cnt;//0-255



void loop() {

  //MIDI受信
  midiEventPacket_t rx;
  do {
    rx = MidiUSB.read();
    if (rx.header != 0) {
      if(rx.header==9){//まずノートオンのみに絞る（９）
        if(rx.byte3!=0){//ベロシティがゼロではない時（ノートオン＋ベロシティ０でノートオフとする機材もあるため）
          note_LED = random(0-6);
          while(last_note == note_LED){
            note_LED = random(0-6);//前回のノートと被ってたらもう一度生成
          }
          last_note = note_LED;
          note_on_flag=1;

/*
          Serial.print("Received: ");
          Serial.print(rx.header);
          Serial.print("-");
          Serial.print(rx.byte1);
          Serial.print("-");
          Serial.print(rx.byte2);
          Serial.print("-");
          Serial.println(rx.byte3);
*/
        }
      }
    }
  } while (rx.header != 0);


  
  cnt=cnt+0.1;
  if(255<cnt)cnt=0;

  B_brt=0;

    uint8_t H;//色環 0-255
    uint8_t S;//彩度
    uint8_t V;//輝度
    const int SEL[]={LED7, LED6, LED5, LED2, LED3, LED4};//処理したいLEDの並び順

    uint8_t H_tmp;
    uint8_t S_tmp;
    uint8_t V_tmp;

    H = cnt;//全体のルーチンカウンタをそのまま色環に流用
    S = 255;//今回は彩度マックスなので触らない
    V = BRT_VAL[BRT_LEVEL];//設定した明るさレベルに応じた値をここで読み込み

    //羽LEDの数だけ繰り返す
    for(int i=0;i<6;i++){
      //元データが汚れてしまうので一旦tmpに値を移してから計算
      H_tmp = H;
      V_tmp = V;
      
      if(V_raw[i] > V ) V_raw[i]--;//設定値より現実の値が高ければ徐々に減算
      if(V_raw[i] < V ) V_raw[i]++;//設定値より現実の値が低ければ徐々に加算
      
/*
      //ランダムで輝度をBRT_RNDで設定した値に変更する（きらきら効果）300を減らすともっと頻度高くキラキラする
      if(random(0,300)==0){
        V_raw[i]=BRT_RND[BRT_LEVEL];//変更に使う値は明るさレベル（BRT_LEVEL）によって違う
      }
*/

      //MIDI受信時に生み出したランダムノートとLEDの番号が一致したら明るくする
      if((i == note_LED) && (note_on_flag ==1)){
        V_raw[i]=BRT_RND[BRT_LEVEL];//変更に使う値は明るさレベル（BRT_LEVEL）によって違う
        //色を作る
        S_raw[i]=  random(150,235);//彩度をランダム変更
        if(random(0,100)<90){
          H_raw[i]=random(100,180);//水色〜青のランダム色
        }else {
          H_raw[i]=42;//稀に黄色
          S_raw[i]=180;//黄色の時は彩度少し抑える
        }
        note_on_flag=0;
      }

      
      H_tmp = H_raw[i];
      V_tmp = V_raw[i];
      S_tmp = S_raw[i];
            
      //LEDに設定値を送信
      pixels.setPixelColor(SEL[i], pixels.ColorHSV(H_tmp<<8,S_tmp,V_tmp));
      //Hは本来0-65535だけれどそんなに解像度必要ないので0-255で計算。出力するときに8ビットシフトして帳尻を合わせてある
        
      //次のLEDのために色環を回転
      H = H + 40;//LED間の色環の進むステップ。大きいとカラフル、少ないと近い色に光る    
      
    }

    //背景LED用
    if(cnt<128)pixels.setPixelColor(0, pixels.ColorHSV(0,0,cnt));
    else pixels.setPixelColor(0, pixels.ColorHSV(0,0,255-cnt));


    //設定を反映
    pixels.show();

    
//    delay(15);
    delay(3);

}
