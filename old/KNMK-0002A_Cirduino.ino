/*!
 * KNMK-0002A Cirduino向けソースコード 1.1
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
 * 1.1：電源を入れてすぐにお胸タッチをするとLED3のみ脈動しないのを修正。その他コードを少しだけ見やすく編集
 */

#include <ADCTouch.h>
#include <Adafruit_NeoPixel.h>

#define LED0 13  // 通常の単色LED
#define LED1 0   // 基板上の表示に対応
#define LED2 1
#define LED3 2
#define LED4 3
#define LED5 4
#define LED6 5
#define LED7 6
#define RGBLED_NUM 7 // RGBLEDの数
#define RGBLED_PIN 6 // NeoPixel接続ピン

#define KAMI  0   //タッチ判別ID
#define MUNE  1
#define SKIRT 2

#define KAMI_P  A1 //タッチセンシングのポート設定（基板により違うので注意）
#define MUNE_P  A0
#define SKIRT_P A2

Adafruit_NeoPixel pixels(RGBLED_NUM, RGBLED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t BRT_LEVEL=1;//明るさ段階レベル0-3　起動時は1から開始
uint8_t BRT_VAL[]={3,10,20,30};//明るさ段階に応じた明るさ詳細値　お好みで変更を。
uint8_t BRT_RND[]={40,65,165,255};//ランダムでこの明るさに変更する（きらきら効果）段階ごとに設定

uint8_t H_raw[RGBLED_NUM];//LEDごとの色環 0-255
uint8_t S_raw[RGBLED_NUM];//LEDごとの彩度 0-255
uint8_t V_raw[RGBLED_NUM];//LEDごとの明るさ 0-255
uint8_t FuryGuage=0;//0-255 お胸を触り続けると溜まるゲージ

int   T_offset[3];  //タッチセンスの初期値
bool  T_flag[3];    //タッチセンスフラグ

int PATTERN_MODE = 0;//点灯パターン 0-2


void setup() {
  pinMode(LED0, OUTPUT);
  digitalWrite(LED0, LOW);//ON
//  Serial.begin(19200);
  pixels.begin();

  //タッチセンシングの初期値を登録
  T_offset[KAMI] = ADCTouch.read(KAMI_P, 500);
  T_offset[MUNE] = ADCTouch.read(MUNE_P, 500);
  T_offset[SKIRT] = ADCTouch.read(SKIRT_P, 500);

  //初期化
  for(int i=0;i<RGBLED_NUM;i++){
    H_raw[i]=100;
    S_raw[i]=0;
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


      //色環（H_tmp)計算　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
      //上にてランダムに作った色を持ってくる
      H_tmp = H_raw[i];
      //ゲーミングモード時は色環を高速で回す
      if(PATTERN_MODE==2) H_tmp = H;
      //お胸を触り続けるとゲージが溜まり色環がシフトする
      H_tmp = min( H_tmp, (255-FuryGuage+7));//


      //明るさ（V_tmp)計算　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
      //怒り時は強制的に最大輝度になる
      V_tmp = max( V_raw[i], FuryGuage);
      if(PATTERN_MODE==2)  V_tmp = max( BRT_RND[BRT_LEVEL], FuryGuage);//ゲーミングモード時は設定した明るさレベルに応じた値をここで読み込み


      //彩度（S_tmp)計算　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
      //上にてランダムに生成したS_rawを拾ってくる
      S_tmp = S_raw[i];
      //ゲーミングモード時には上の計算を無視してとにかく彩度MAX
      if(PATTERN_MODE==2)S_tmp = 255;
      //怒り時はゲージの値を優先して拾う →ゲージに応じて徐々に彩度MAXへ
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
          digitalWrite(LED0, LOW);//ON
            
          //明るさ確認コール
          for(int i=0;i<2;i++){
            if(BRT_LEVEL==0){
              pixels.setPixelColor(LED4, pixels.ColorHSV(10000,225,20));
            }
            if(BRT_LEVEL==1){
              pixels.setPixelColor(LED4, pixels.ColorHSV(10000,225,20));
              pixels.setPixelColor(LED7, pixels.ColorHSV(10000,225,20));
            }
            if(BRT_LEVEL==2){
              pixels.setPixelColor(LED4, pixels.ColorHSV(10000,205,40));
              pixels.setPixelColor(LED3, pixels.ColorHSV(10000,205,40));
              pixels.setPixelColor(LED7, pixels.ColorHSV(10000,205,40));
              pixels.setPixelColor(LED6, pixels.ColorHSV(10000,205,40));
            }
            if(BRT_LEVEL==3){
              pixels.setPixelColor(LED4, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED3, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED2, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED7, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED6, pixels.ColorHSV(10000,205,60));
              pixels.setPixelColor(LED5, pixels.ColorHSV(10000,205,60));
            }
            pixels.show();
            delay(100);
            pixels.clear();
            pixels.show();
            delay(50);
          }
          digitalWrite(LED0, HIGH);//OFF         
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
      digitalWrite(LED0, LOW);//ON
      if(254<FuryGuage){//怒りがマックスになったら脈動させる
        delay(200);
        FuryGuage=230;
      }      
      if((FuryGuage+3)<255) FuryGuage = FuryGuage + 2;
      if((FuryGuage)<255) FuryGuage = FuryGuage + 1;
      T_flag[MUNE]=1;
    }else{
      digitalWrite(LED0, HIGH);//OFF
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
