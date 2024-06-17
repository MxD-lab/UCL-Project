#include "Mouse.h"
//最初の生データ
int firstRead[3] = { 0, 0, 0 };
//生データ
int sensorValue[3] = { 0, 0, 0 };
//電圧
float sensorVolt[3] = { 0, 0, 0 };
//センサマトリクス
//float sensorMatrix[3][3] = { { 0.02787849, 0.7439311 , -1.4808451 },
// { -1.0677772 ,  0.48485976,  0.8420289 }, { 0.64206654,  1.0900601 ,  1.6909394 } };
float sensorMatrix[3][3] = { {  0.08546697, 0.7792828 , -0.8264685 }, 
  {-0.7212511 , 0.42828566, 0.40850416}, { 0.468209 , 1.2410394 , 0.87812704}};
//3軸力
float force[3] = { 0, 0, 0 };
int mousePressed = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Mouse.begin();
  firstRead[0] = analogRead(A0);
  firstRead[1] = analogRead(A1);
  firstRead[2] = analogRead(A2);
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.print(analogRead(A0));
//  Serial.print(",");
//  Serial.print(analogRead(A1));
//  Serial.print(",");
//  Serial.println(analogRead(A2));


  sensorValue[0] = analogRead(A0) - firstRead[0];
  sensorValue[1] = analogRead(A1) - firstRead[1];
  sensorValue[2] = analogRead(A2) - firstRead[2];

  //電圧変換
  sensorVolt[0] = sensorValue[0] * 5.0 / 1023;
  sensorVolt[1] = sensorValue[1] * 5.0 / 1023;
  sensorVolt[2] = sensorValue[2] * 5.0 / 1023;

  //力方向算出
  force[0] = sensorVolt[0] * sensorMatrix[0][0] + sensorVolt[1] * sensorMatrix[0][1] + sensorVolt[2] * sensorMatrix[0][2];
  force[1] = sensorVolt[0] * sensorMatrix[1][0] + sensorVolt[1] * sensorMatrix[1][1] + sensorVolt[2] * sensorMatrix[1][2];
  force[2] = sensorVolt[0] * sensorMatrix[2][0] + sensorVolt[1] * sensorMatrix[2][1] + sensorVolt[2] * sensorMatrix[2][2];

  if(force[1]<0.3){force[1]*=10;}

  Serial.print(force[0]);
  Serial.print(",");
  Serial.print(force[1]);
  Serial.print(",");
  Serial.println(force[2]);

  //上下左右方向
  if (abs(force[0]) > 0.2 | abs(force[1]) > 0.2) {
//    Mouse.move(force[1] * 10, force[0] * -10, 0);
  //クリック判定
  } else if (force[2] > 1.0 && mousePressed == 0) {
//    Mouse.press();
    mousePressed = Mouse.isPressed();
  } else if (force[2] < 1.0 && mousePressed == 1) {
//    Mouse.release();
    mousePressed = Mouse.isPressed();
  }
  delay(1);
}