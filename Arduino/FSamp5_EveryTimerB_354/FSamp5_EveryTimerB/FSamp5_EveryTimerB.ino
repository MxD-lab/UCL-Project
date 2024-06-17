/*
  Fsamp4のDACを手動で調整するコード
  2021_9/2（木） 記入者：船橋　佑　新Fsamp4基板（赤色）の動作確認に使用
  2021_9/24（金）記入者：船橋　佑　1チャンネルだけ自動でDAC調整するコード作成。二分法で行う。未完成である。
  2021_9/25（土）記入者：船橋　佑　1チャンネルだけ自動でDAC調整するコード作成が完成。原因はおそらくset_volt後にすぐReadAD2をしたから。間にdelay()を入れると正常に動作した。
  　　　　　　　　　　　　　　　　　 追記：3チャンネル同時自動調整コードが完成した。あとはコードの関数化と計測中のエラーをLED点灯で知らせるよう追加する予定。
  2021_9/27（月）記入者：船橋　佑　3チャンネル同時自動調整コードの関数化に成功。自動計測。キーボードでsを打つと計測終了。
  　　　　　　　　　　　　　　　　　 追記：LEDコード追加。調整中は点灯しており、各chごとに調整が終わると消灯する。
  2021_9/28（火）記入者：船橋　佑　コードをreadableにするべく整形した。具体的に、むやみにグローバル変数を使用せず、#defineで定義をなるべくした。また、3チャンネル分一気に調整するのではなく。調整と3チャンネルに行う処理を分けた。
  2021_9/29（水）記入者：船橋　佑　さらに整形。staticを使用することで、関数の中でしか使用しないが値を保持したい変数を関数の中で定義した。
                             追記：計測中に各チャンネルに対応したボタンを押すことで再調整するコード追加。
  2021_10/1（金）記入者：船橋　佑　現在未完成。エラー処理は正しく動いたが、今度は正常時の挙動がおかしくなった。
  2021_10_4（月）記入者：船橋　佑　完成。機能の説明。初期状態として、各チャンネルに対応したLEDランプが点灯。DAC調整が完了すると。LEDランプが消灯する。また、センサが死んでいた場合、そのチャンネルのLEDランプが点滅し、Serial.printでどのチャンネルが死んでいるか表示される。
  　　　　　　　　　　　　　　　　　 そして、調整終了後に再度調整したい場合は各チャンネルに対応したボタンを押すと行われる。
  2021_10_7（木）記入者：船橋　佑　setupの中がごちゃついてたので関数化して整理した。
  2021_11_8（月）記入者：船橋　佑　新たにシリアルからの入力で更正をできるようにしました。'1'と打つとch.1を、'2'と打つとch.2を、'3'と打つとch.3を、'a'と打つとすべてのチャンネルを更正します。
  2021_11_25（木）記入者：船橋　佑　Arduino nano Everyでの計測周期が900Hz程だったので、800Hzで割り込みできるよう変更。ハードウェアタイマー内のTCB0とTCB1を使用して割り込みしている。
  2021_12\_03（金）記入者：船橋　佑　計測プログラム内に余計な行があったため、削除しました。該当箇所の内容として、計測開始にch.1のLEDを点灯し、計測終了時にLEDを消灯するといったものです。

  2023_2_2（木）記入者：船橋　佑　ADCでの出力を荷重の値にした．

  2024/04/14　記入者：野間 春生　コンパイルには、以下を設定すること
    ターゲットを　Arduino Nano Every　とする
    Arduino-Nano-Every-Timer-Controller-B　ライブラリーをインストールする（https://github.com/Kees-van-der-Oord/Arduino-Nano-Every-Timer-Controller-B/tree/master）
    力変換する場合には、matrix[3][3]を校正値に合わせて書き換える

*/

//No350のセンサを使用．

//AD Converter 拡張
// 2021/07/12
// Haruo Noma

//2021_7/13に船橋がコメント追加
//2021_9/7に船橋がコメント追加

//#include <FSamp5.h>
#include "EveryTimerB.h"

#define Timer1 TimerB2

boolean istimer_flag = false;     //計測中を表すフラグ。trueの時にタイマー割り込みが発生し，メインループのコードが実行される
boolean isforce_flag = false;     //出力するデータを切り替えるフラグ．trueだと3次元荷重データを出力し，falseだとMEMS触覚センサの生データが出力される

long firstAD[3];   //offsetかける用の値
long ADmeasure[3];       //計測中にADから読み取ったアンプの出力の値を格納する配列
float changeLSB = 0.00004883;  // 5.0/1024/100をしてます．
float predF[3];

//校正処理で算出したマトリクス No.509
//float演算を減らすために小数第２位までを使用して後程まとめて100でわる．
int matrix[3][3] = {{2, 56, -68},
                     {-81,  31,  35},
                     {69,  44,  74}};

//[[ 0.01787384, 0.5614992 , -0.6825333 ],
//[-0.80829626, 0.31005788, 0.3553213 ],
//[ 0.6880116 , 0.4358145 , 0.7359737 ]

//計測中のDACの値を読み取る関数。リングバッファを使用
void Read_AD(long *a ) {
  int i;
  const int numReadings = 16;
  const int Shift = 4;    //上記numReafingsの分、シフトする量。numReafings=2^shift とする。
  static int readings[3][numReadings];      // the readings from the analog input
  static int readings2[3][numReadings];      // the readings from the analog input
  static int readIndex = 0;                 // the index of the current reading
  static int readIndex2 = 0;                 // the index of the current reading
  static int total[3] = {0, 0, 0};          // the running total
  static int total2[3] = {0, 0, 0};          // the running total

  for (i = 0; i < 3; i++) {   //アンプの出力OUTAがA2に入り、OUTCがA0に入っているため配列に入れる順番を逆さにして入れている（詳しくは回路図参照）
    total[i] = total[i] - readings[i][readIndex];          // subtract the last reading:
    readings[i][readIndex] = analogRead(i) & 0x03ff;
    total[i] = total[i] + readings[i][readIndex];          // add the reading to the total:
  }
  readIndex = readIndex + 1;                    // advance to the next position in the array:
  if (readIndex >= numReadings) {               // if we're at the end of the array...
    readIndex = 0;                              // ...wrap around to the beginning:
  }
  for (i = 0; i < 3; i++) {
    a[i] = total[i] >> Shift;                  // calculate the average:
  }
  //return (1);
}


//割り込み発生時に呼び出される関数。計測ループが回るようにフラグを変更
void flag_change(){
  istimer_flag = true;
}


void setup() {
  Serial.begin(115200);
  Timer1.initialize();                 //クロック初期化
  Timer1.attachInterrupt(flag_change);           //呼び出してくる関数を定義
  Timer1.setPeriod(1666);    //micro sec単位。約600Hzで計測できるようタイマー割り込みを行う。
  firstAD[0] = analogRead(0); firstAD[1] = analogRead(1); firstAD[2] = analogRead(2);

  
}


//計測用
void loop() {

  char c = Serial.read();        //キーボードからの文字入力を読み取る．

  if(c == 'n'){                        //キーボードでnを入力すると荷重計測モードになります．(newtonのn)
    isforce_flag = true;
  }
  if(c == 'r'){                        //キーボードでrを入力するとMEMS触覚センサの生データを出力します．(rawのr)
    isforce_flag = false;
  }
  if(c == 'o'){                        //再度オフセットします．ゼロではなくアルファベットのオーです．
    firstAD[0] = analogRead(0); firstAD[1] = analogRead(1); firstAD[2] = analogRead(2);
  }

 
  while(istimer_flag == true){

    if(isforce_flag == true){  //MEMS触覚センサの生データを荷重データに変換して出力します．
      Read_AD(ADmeasure);
      ADmeasure[0] = ADmeasure[0] - firstAD[0];
      ADmeasure[1] = ADmeasure[1] - firstAD[1];
      ADmeasure[2] = ADmeasure[2] - firstAD[2];
  
      predF[0] = ( ADmeasure[0]*matrix[0][0]  +  ADmeasure[1]*matrix[0][1]  +  ADmeasure[2]*matrix[0][2] ) * changeLSB;
      predF[1] = ( ADmeasure[0]*matrix[1][0]  +  ADmeasure[1]*matrix[1][1]  +  ADmeasure[2]*matrix[1][2] ) * changeLSB;
      predF[2] = ( ADmeasure[0]*matrix[2][0]  +  ADmeasure[1]*matrix[2][1]  +  ADmeasure[2]*matrix[2][2] ) * changeLSB;

      Serial.print(predF[0]); Serial.print(','); Serial.print(predF[1]); Serial.print(','); Serial.println(predF[2]);
      
    } else{ //MEMS触覚センサの生データをそのまま出力します．
      Read_AD(ADmeasure);
      ADmeasure[0] = ADmeasure[0] - firstAD[0];
      ADmeasure[1] = ADmeasure[1] - firstAD[1];
      ADmeasure[2] = ADmeasure[2] - firstAD[2];

      Serial.print(ADmeasure[0]); Serial.print(','); Serial.print(ADmeasure[1]); Serial.print(','); Serial.println(ADmeasure[2]);
      
    }
      
    istimer_flag = false;
  }
  
}
