/*!
 * KNMK-0002A Cirduino向けソースコード 1.0
 * Copyright (c) 2022 Mizuhasi Yukkie
 * This software is released under the MIT license.
 * see https://opensource.org/licenses/MIT
*/

/*
 * このコードを使用するためには、下記のライブラリをインストールしてください。Githubからのダウンロードがよくわからないという方は、
 * スケッチ→ライブラリをインクルード→ライブラリを管理→検索窓からライブラリ名を検索（”ADCTouch”など）→インストールがおすすめです。
 * 「ADCTouch」 https://github.com/martin2250/ADCTouch
 * 「Adafruit_NeoPixel」 https://github.com/adafruit/Adafruit_NeoPixel
 * 
 * 以下はこの基板のリリース直前にざっくり書いたものです。そのうちサイトにまとめてリンクを書く形に変更しようと思います。
 * 
 * このプログラムをＵＳＢから書き込むには一度ArduinoブートローダーをICSP端子経由で書き込む必要があります。
 * （2022年７月現在、製造の都合上の理由により出荷時にはArduinoブートローダ部分が含まれていません） 
 * お手数ですがICSPライタ（ACRISP mkIIなど）をご用意ください。
 * 
 * ブートローダーを書き込む方法（一度だけ行えばOKです）
 * 基板上のICSPピンにAVRISP mkIIなどのICSPライターの６ピンを押し当てる（もしくははんだ付けする）
 * 
 * ツール→ボード："Arduino Leonardo"を選択
 * ツール→”ブートローダを書き込む”を一度実行
 * 
 * 今後はＵＳＢ端子に差し込むだけでArduinoの左上「マイコンボードに書き込み」を押すことで好きなプログラムを書き込めます。
 * ツール→シリアルポート：「USBで接続済みの萌基板のポート」（ArduinoLeonardoと表示されます）を選択するのをお忘れなく。
 * 
 * 既知の基板上のエラー：HWB端子をプルダウンするのを忘れてしまいました。放っておいても基本的にロー状態なので無害ですが、
 * もしブートローダー書き込み後に裏面LEDが高速点滅して言うことを聞かない……などが起きた場合は10kΩでGndに落とすと安定するかもしれません。
 */

#include <ADCTouch.h>
#include <Adafruit_NeoPixel.h>

#define LED_B1 0 // LEDのIDをわかりやすく名前に変換
#define LED_L1 1
#define LED_L2 2
#define LED_L3 3
#define LED_R1 4
#define LED_R2 5
#define LED_R3 6
#define RGBLED_NUM 7 // How many LEDs
#define RGBLED_PIN 6 // NeoPixel pin
#define BLINK_LED 13 // Normal LED pin

#define KAMI 0
#define MUNE 1
#define SKIRT 2

#define KAMI_P A1 //タッチセンシングのポート設定
#define MUNE_P A0
#define SKIRT_P A2

Adafruit_NeoPixel pixels(RGBLED_NUM, RGBLED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t BRT_LEVEL=1;//明るさ段階レベル0-3　起動時は1から開始
uint8_t BRT_VAL[]={3,10,20,30};//明るさ段階に応じた明るさ詳細値　お好みで変更を。
uint8_t BRT_RND[]={40,65,165,255};//ランダムでこの明るさに変更する（きらきら効果）段階ごとに設定

uint8_t H_raw[7];//LEDごとの色環 0-255
uint8_t S_raw[7];//LEDごとの彩度 0-255
uint8_t V_raw[7];//LEDごとの明るさ 0-255

uint8_t FuryGuage=0;//0-255 お胸を触り続けると溜まるゲージ

int   T_offset[3];//タッチセンスの初期値
bool  T_flag[3];  //タッチセンスフラグ

int PATTERN_MODE = 0;//点灯モード 0-2


void setup() {
  pinMode(BLINK_LED, OUTPUT);
  digitalWrite(BLINK_LED, LOW);//ON
  Serial.begin(19200);
  pixels.begin();

  //タッチセンシングの初期値を登録
  T_offset[KAMI] = ADCTouch.read(KAMI_P, 500);
  T_offset[MUNE] = ADCTouch.read(MUNE_P, 500);
  T_offset[SKIRT] = ADCTouch.read(SKIRT_P, 500);

  //０で初期化
  for(int i=0;i<RGBLED_NUM;i++){
    H_raw[i]=0;
    S_raw[i]=0;
    V_raw[i]=0;
  }
  digitalWrite(BLINK_LED, HIGH);//OFF
}

int B_brt;//背面LEDの明るさ変数

void loop() {
  static uint8_t cnt;//0-255
  cnt++;

  B_brt=0;

    uint8_t H;//色環 0-255
    uint8_t S;//彩度
    uint8_t V;//輝度
    const int SEL[]={LED_R3, LED_R2, LED_R1, LED_L1, LED_L2, LED_L3};//処理したいLEDの並び順

    uint16_t H_tmp;
    uint16_t V_tmp;
    uint8_t S_tmp;

    H = cnt;//全体のルーチンカウンタをそのまま色環に流用
    S = 0;//今回は彩度マックスなので触らない
    V = BRT_VAL[BRT_LEVEL];//設定した明るさレベルに応じた値をここで読み込み

    //羽LEDの数だけ繰り返す
    for(int i=0;i<6;i++){

      //光モード０の時
      if(PATTERN_MODE==0){      
        if(V_raw[i] > V ) V_raw[i]--;//設定値より現実の値が高ければ徐々に減算
        if(1<BRT_LEVEL)if(V_raw[i] > V ) V_raw[i]--;
        if(1<BRT_LEVEL)if(V_raw[i] > V ) V_raw[i]--;
  
        if(V_raw[i] < V ) V_raw[i]++;//設定値より現実の値が低ければ徐々に加算
        if(S_raw[i] > S+3 ) S_raw[i]=S_raw[i]-3;//設定値より現実の値が高ければ徐々に減算

        //ランダムで輝度をBRT_RNDで設定した値に変更する（きらきら効果）200を減らすともっと頻度高くキラキラする
        if(random(0,200)==0){
          V_raw[i]=BRT_RND[BRT_LEVEL];//変更に使う値は明るさレベル（BRT_LEVEL）によって違う
          S_raw[i]=random(150,235);
          if(random(0,100)<90){
            H_raw[i]=random(100,180);//水色〜青のランダム色
          }else {
            H_raw[i]=42;//稀に黄色
            S_raw[i]=180;//黄色の時は彩度少し抑える
          }
        }
      }
      
      //光モード１＝チカチカ色が変わるモード
      if(PATTERN_MODE==1){
        if(random(0,50)==0){//ランダム確率1/50
          V_raw[i] = random(BRT_VAL[BRT_LEVEL],BRT_RND[BRT_LEVEL]);//明るさを範囲でランダムで変更
          S_raw[i]=  random(150,235);//彩度をランダム変更
          if(random(0,100)<95){// 95/100の確率で水色系のランダム色
            H_raw[i]=random(100,180);//水色〜青のランダム色
          }else {// 5/100の確率で黄色
            H_raw[i]=42;//稀に黄色
            S_raw[i]=180;//黄色の時は彩度少し抑える
          }
        }
      }

      //ゲーミングモード（色環を高速で回す）
      if(PATTERN_MODE==2){
        cnt = cnt+1;
        H = H +20;
        delay(1); 
      }


      //お胸を触り続けるとゲージが溜まり色環がシフトする
      H_tmp = H_raw[i];
      if(PATTERN_MODE==2) H_tmp = H;      //ゲーミングモード（色環を高速で回す）
      H_tmp = min( H_tmp, (255-FuryGuage+7));//

      //ゲージにより明るさ設定を無視して最大輝度になる
      V_tmp = max( V_raw[i], FuryGuage);
      if(PATTERN_MODE==2)  V_tmp = max( BRT_RND[BRT_LEVEL], FuryGuage);//ゲーミングモード時は設定した明るさレベルに応じた値をここで読み込み
      
      //ゲージに応じて彩度増える
      S_tmp = S_raw[i];
      if(PATTERN_MODE==2)S_tmp = 255;//ゲーミングモード時には彩度マックス
      S_tmp = max( S_tmp,FuryGuage );


  
      //LEDに設定値を送信
      pixels.setPixelColor(SEL[i], pixels.ColorHSV(H_tmp<<8,S_tmp,V_tmp));
      //Hは本来0-65535だけれどそんなに解像度必要ないので0-255で計算。出力するときに8ビットシフトして帳尻を合わせてある

      //背景LED用
      if(cnt<128)pixels.setPixelColor(0, pixels.ColorHSV(0,0,cnt));
      else pixels.setPixelColor(0, pixels.ColorHSV(0,0,255-cnt));

      //ゲーミングモード（色環を高速で回す）
      if(PATTERN_MODE==2){
        //背景LEDを上書き
        pixels.setPixelColor(0, pixels.ColorHSV(cnt<<8,255,255));
      }      
      
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
          if (BRT_LEVEL == 0)     BRT_LEVEL = 1;
          else if(BRT_LEVEL == 1) BRT_LEVEL = 2;
          else if(BRT_LEVEL == 2) BRT_LEVEL = 3;
          else                    BRT_LEVEL = 0;

          pixels.clear();
          pixels.show();
          digitalWrite(BLINK_LED, LOW);//ON
            
          //明るさ確認コール
          for(int i=0;i<2;i++){
            if(BRT_LEVEL==0){
              pixels.setPixelColor(LED_L3, pixels.ColorHSV(10000,225,20));
//              pixels.setPixelColor(LED_R3, pixels.ColorHSV(15000,225,20));
            }
            if(BRT_LEVEL==1){
              pixels.setPixelColor(LED_L3, pixels.ColorHSV(10000,225,20));
              pixels.setPixelColor(LED_R3, pixels.ColorHSV(10000,225,20));
            }
            if(BRT_LEVEL==2){
              pixels.setPixelColor(LED_L3, pixels.ColorHSV(10000,205,40));
              pixels.setPixelColor(LED_L2, pixels.ColorHSV(10000,205,40));
              pixels.setPixelColor(LED_R3, pixels.ColorHSV(10000,205,40));
              pixels.setPixelColor(LED_R2, pixels.ColorHSV(10000,205,40));
            }
            if(BRT_LEVEL==3){
              pixels.setPixelColor(LED_L3, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED_L2, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED_L1, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED_R3, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED_R2, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED_R1, pixels.ColorHSV(10000,205,60));
            }
            pixels.show();
            delay(100);
            pixels.clear();
            pixels.show();
            delay(50);
          }
          digitalWrite(BLINK_LED, HIGH);//OFF         
          //明るさ変更したら即反映
          for(int i=0;i<RGBLED_NUM;i++){
            V_raw[i]=BRT_VAL[BRT_LEVEL];
          }
        }
        T_flag[KAMI]=1;//フラグを立てることで立ち上がり時のみ実行
    }else{
        T_flag[KAMI]=0;//離したのでタッチフラグクリア
    }


    //胸タッチ検出
    if(50 < T_sense1){
      digitalWrite(BLINK_LED, LOW);//ON
      if(254<FuryGuage){//怒りがマックスになったら脈動させる
        delay(200);
        FuryGuage=230;
      }      
      if((FuryGuage+3)<255) FuryGuage = FuryGuage + 2;
      if((FuryGuage)<255) FuryGuage = FuryGuage + 1;
      T_flag[MUNE]=1;
    }else{
      digitalWrite(BLINK_LED, HIGH);//OFF
      if(0<(FuryGuage-6)) FuryGuage = FuryGuage - 6;
      if(0<FuryGuage) FuryGuage = FuryGuage - 1;
      T_flag[MUNE]=0;
    }

    //スカートタッチ検出
    if(50 < T_sense2){
        //フラグがまだ立っていなければ以下を実行
        if(T_flag[SKIRT]==0){
          if (PATTERN_MODE == 0)     PATTERN_MODE = 1;
          else if(PATTERN_MODE == 1) PATTERN_MODE = 2;
          else                       PATTERN_MODE = 0;

          //了解コール
          pixels.clear();
          pixels.show();
          delay(20);
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
