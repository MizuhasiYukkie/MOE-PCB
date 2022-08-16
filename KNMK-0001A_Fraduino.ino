/*!
 * KNMK-0001A Fraduino向けソースコード 1.1
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
 * 「ADCTouch」 https://github.com/martin2250/ADCTouch
 * 「Adafruit_NeoPixel」 https://github.com/adafruit/Adafruit_NeoPixel
 * 
 * ツール→ボード："Arduino Leonardo"を選択
 * ツール→シリアルポート：「USBで接続済みの萌基板のポート」（ArduinoLeonardoと表示されます）を選択
 * 「マイコンボードに書き込む」を実行するとここに書いてあるプログラムが書き込まれます。
 * ソースが汚いのでちんぷんかんぷんだと思いますが、BRT_VAL[]={ここの数字}や、BRT_RND[]＝{ここの数字}を変えたり
 * delayの値を変えてみたり、random(0,ここの数字)を変えてみるとちょっぴり挙動が変わるでしょう。
 * 
 * 既知の基板上のエラー：HWB端子をプルダウンするのを忘れてしまいました。放っておいても基本的にロー状態なので無害ですが、
 * もし書き込み後に裏面LEDが高速点滅して言うことを聞かない……などが起きた場合は10kΩでGndに落とすと安定するかもしれません。
 * 
 * 1.1：少し見やすく編集、機能の更新は無し。使用していないシリアル通信の初期化をコメントアウト
 */

#include <ADCTouch.h>
#include <Adafruit_NeoPixel.h>

#define LED0 13 // 通常の単色LED
#define LED1 0  // 基板上の表示に対応
#define LED2 1
#define LED3 2
#define LED4 3
#define LED5 4
#define LED6 5
#define LED7 6
#define RGBLED_NUM 7 // RGBLEDの数
#define RGBLED_PIN 6 // NeoPixel接続ピン

#define KAMI  0  //タッチ判別ID
#define MUNE  1
#define SKIRT 2

#define KAMI_P  A0 //タッチセンシングのポート設定（基板により違うので注意）
#define MUNE_P  A1
#define SKIRT_P A2

Adafruit_NeoPixel pixels(RGBLED_NUM, RGBLED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t BRT_LEVEL=1;//明るさレベル0-3　起動時は1から開始
uint8_t BRT_VAL[]={10,25,65,175};//段階ごとの明るさ詳細値　お好みで変更を。
uint8_t BRT_RND[]={40,65,155,255};//ランダムで変更する値（きらきら効果）

uint8_t V_raw[RGBLED_NUM];//各LEDごとの明るさ実際の値（自動追従）
uint8_t FuryGuage=0;//0-255 お胸を触り続けると溜まる

int   T_offset[3];  //タッチセンシングの初期値
bool  T_flag[3];    //タッチセンシングフラグ

uint8_t PATTERN_MODE = 0;//点灯パターン 0-2


void setup() {
  pinMode(LED0, OUTPUT);
  digitalWrite(LED0, LOW);//ON
//  Serial.begin(19200);
  pixels.begin();

  //タッチセンシングの初期値を登録
  T_offset[KAMI] = ADCTouch.read(KAMI_P, 500);
  T_offset[MUNE] = ADCTouch.read(MUNE_P, 500);
  T_offset[SKIRT] = ADCTouch.read(SKIRT_P, 500);

  //明るさ０で初期化
  for(int i=0;i<RGBLED_NUM;i++){
    V_raw[i]=0;
  }
  digitalWrite(LED0, HIGH);//OFF
}


void loop() {
  static uint8_t cnt;//0-255
  cnt++;

    uint8_t H;//色環 0-255
    uint8_t S;//彩度
    uint8_t V;//輝度
    const int SEL[]={LED7, LED6, LED5, LED2, LED3, LED4};//処理したいLEDの並び順

    uint8_t H_tmp;
    uint8_t V_tmp;

    H = cnt;//全体のルーチンカウンタをそのまま色環に流用
    S = 255;//今回は彩度マックスなので触らない
    V = BRT_VAL[BRT_LEVEL];//設定した明るさレベルに応じた値をここで読み込み

    //羽LEDの数だけ繰り返す
    for(int i=0;i<6;i++){
      //元データが汚れてしまうので一旦tmpに値を移してから計算
      H_tmp = H;
      V_tmp = V;

      //ゲーミングモードなら勝手に一段明るくする
      if(PATTERN_MODE==2 && BRT_LEVEL<3)V = BRT_VAL[BRT_LEVEL+1];
      
      if(V_raw[i] > V ) V_raw[i]--;//設定値より現実の値が高ければ徐々に減算
      if(V_raw[i] < V ) V_raw[i]++;//設定値より現実の値が低ければ徐々に加算
      

      //光モード０、１の時だけ
      if((PATTERN_MODE==0)||(PATTERN_MODE==1)){
        //ランダムで輝度をBRT_RNDで設定した値に変更する（きらきら効果）300を減らすともっと頻度高くキラキラする
        if(random(0,300)==0){
          V_raw[i]=BRT_RND[BRT_LEVEL];//変更に使う値は明るさレベル（BRT_LEVEL）によって違う
        }
      }
      
      //お胸を触り続けると怒りゲージが溜まり色環が赤に振り切る
      H_tmp = max( H_tmp, (FuryGuage));
      if (240 < FuryGuage)H_tmp=250;

      //怒りゲージにより明るさ設定を無視して最大輝度になる
      V_tmp = max( V_raw[i], FuryGuage);

      //チカチカ色が変わるモード（色環を低解像度で回す）
      if(PATTERN_MODE==1){
        H_tmp = H_tmp >>5;
        H_tmp = H_tmp <<5;
      }
  
      
      //LEDに設定値を送信
      pixels.setPixelColor(SEL[i], pixels.ColorHSV(H_tmp<<8,S,V_tmp));
      //Hは本来0-65535だけれどそんなに解像度必要ないので0-255で計算。出力するときに8ビットシフトして帳尻を合わせてある



      //ゲーミングモード（色環を高速で回す）
      if(PATTERN_MODE==2){
        cnt = cnt+1;
        H = H +20;
        delay(1);
      
      }else{//他のモードでは
        
        //次のLEDのために色環を回転
        H = H + 40;//LED間の色環の進むステップ。大きいとカラフル、少ないと近い色に光る    
      }
      
    }

    //背景LED用
    if(cnt<128)pixels.setPixelColor(0, pixels.ColorHSV(0,0,cnt));
    else pixels.setPixelColor(0, pixels.ColorHSV(0,0,255-cnt));
    if (230 < FuryGuage)pixels.setPixelColor(0, pixels.ColorHSV(-5<<8,255,255));//怒り時

    //ゲーミングモード（色環を高速で回す）
    if(PATTERN_MODE==2){
      //背景LEDを上書き
      pixels.setPixelColor(0, pixels.ColorHSV(cnt<<8,255,255));
    }


    //設定を反映
    pixels.show();



    int T_sense0 = ADCTouch.read(KAMI_P,10) -T_offset[KAMI];   //髪の毛タッチ
    int T_sense1 = ADCTouch.read(MUNE_P,10) -T_offset[MUNE];   //お胸タッチ
    int T_sense2 = ADCTouch.read(SKIRT_P,10) -T_offset[SKIRT];  //スカートタッチ



    //髪タッチ検知
    if(50 < T_sense0){
        //フラグがまだ立っていなければ以下を実行
        if(T_flag[KAMI]==0){          
          digitalWrite(LED0, LOW);//ON
          if (BRT_LEVEL == 0)     BRT_LEVEL = 1;
          else if(BRT_LEVEL == 1) BRT_LEVEL = 2;
          else if(BRT_LEVEL == 2) BRT_LEVEL = 3;
          else                    BRT_LEVEL = 0;
          //明るさ変更したら即反映
          for(int i=0;i<RGBLED_NUM;i++){
            V_raw[i]=BRT_VAL[BRT_LEVEL];
          }
          //了解コール
          pixels.clear();
          pixels.show();
          delay(8);
          digitalWrite(LED0, HIGH);//OFF         
        }
        T_flag[KAMI]=1;//フラグを立てることで立ち上がり時のみ実行
    }else{
        T_flag[KAMI]=0;//離したのでタッチフラグクリア
    }


    //胸タッチ検出
    if(50 < T_sense1){
      digitalWrite(LED0, LOW);//ON
      if(254<FuryGuage){//怒りがマックスになったら脈動させる
        delay(200);
        FuryGuage=200;
      }
      if((FuryGuage+6)<255) FuryGuage = FuryGuage + 5;
      if((FuryGuage)<255) FuryGuage = FuryGuage + 1;
      T_flag[MUNE]=1;
    }else{
      digitalWrite(LED0, HIGH);//OFF
      if(0<(FuryGuage-12)) FuryGuage = FuryGuage - 12;
      if(0<FuryGuage) FuryGuage = FuryGuage - 1;
      T_flag[MUNE]=0;
    }

    //スカートタッチ検出
    if(50 < T_sense2){
        //フラグがまだ立っていなければ以下を実行
        if(T_flag[SKIRT]==0){          
          digitalWrite(LED0, LOW);//ON
          if (PATTERN_MODE == 0)     PATTERN_MODE = 1;
          else if(PATTERN_MODE == 1) PATTERN_MODE = 2;
          else                       PATTERN_MODE = 0;

          //了解コール
          pixels.clear();
          pixels.show();
          delay(20);
          digitalWrite(LED0, HIGH);//OFF         
        }
        T_flag[SKIRT]=1;//フラグを立てることで立ち上がり時のみ実行
    }else{
        T_flag[SKIRT]=0;//離したのでタッチフラグクリア
    }

//  Serial.println(FuryGuage);

//  タッチセンシングの値を見たいとき
//  Serial.println(T_sense0);
//  Serial.println(T_sense1);
//  Serial.println(T_sense2);


    
    delay(15);

}
