#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <string.h>
#include "soc/rtc.h"
#include <rom/rtc.h>
#include "esp32/ulp.h"
#include "driver/rtc_io.h"

// 動作確認用点滅LEDピン
#define HEART_BEAT_PIN1   2
#define HEART_BEAT_PIN2   12

// PWM制御用の変数定義
#define LED_PIN          14
#define LEDC_CHANNEL_0    1
#define BASE_POWER_PIN   27
#define LEDC_TIMER_13_BIT  13 /* use 13 bit precission for LEDC timer */
#define LEDC_BASE_FREQ     30000 /* use 5000 Hz as a LEDC base frequency*/
#define BRIGHTNESS        128   // how bright the LED is

// Sleep 用
#define uS_TO_mS_FACTOR  1000  //Conversion factor for micro seconds to 10m seconds
#define uS_TO_100mS_FACTOR 100000   //Conversion factor for micro seconds to 100m seconds
#define sleep_Sec 600 // sleep sec

// ADC制御用の変数定義
#define ADC_PIN            34

// タイマー割込み用の変数定義
volatile int interruptCounter2;   /* 給水時割り込みカウンタ */
volatile long _1msecCounterAtWakeUp=0;  /* メイン割り込み回数カウンタ */
volatile int  _100msecCounterAtWaterServe=0;  /* 給水時割り込み回数カウンタ */
hw_timer_t *timer1 = NULL; /* ウェークアップ時割り込み制御用構造体 */
hw_timer_t *timer2 = NULL; /* 給水時割り込み制御用用構造体 */
portMUX_TYPE timerMux1=portMUX_INITIALIZER_UNLOCKED; 
portMUX_TYPE timerMux2=portMUX_INITIALIZER_UNLOCKED; 

// Deep Sleep 用タイムカウンタ
RTC_DATA_ATTR long bootCount = 0;
float cali_factor;

// wifi用の変数定義
const char* ssid = "ESP32-WiFi";
AsyncWebServer server(80);

const IPAddress ip(192,168,0,2);
const IPAddress subnet(255,255,255,0);

// ポンプ用の変数定義
RTC_DATA_ATTR int serve_times=1;   // １日の給水回数
RTC_DATA_ATTR int serve_time[24];  // 給水時刻
RTC_DATA_ATTR int next_serve_time;  // 次回の給水時刻
RTC_DATA_ATTR int next_serve_time_index;  // 次回の給水のインデックス
RTC_DATA_ATTR long waitCounts; // のこり待ち秒数
RTC_DATA_ATTR int serve_interval=0;
RTC_DATA_ATTR double serve_volume;
RTC_DATA_ATTR double serve_voltage;
int year,month,date,hours,minutes,seconds;
volatile boolean timer_go = false;

//---------------------------------------------------------------------------------
//                               関数定義
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//                            割込み時関数定義
//---------------------------------------------------------------------------------
void IRAM_ATTR onTimerAtWakeUp() {
  portENTER_CRITICAL_ISR(&timerMux1);
  // トータルカウンタをインクリメント
  _1msecCounterAtWakeUp++;
  portEXIT_CRITICAL_ISR(&timerMux1);
}
//------------------
// 給水時割込み関数
//------------------
void IRAM_ATTR onTimerAtWaterServe() {
  portENTER_CRITICAL_ISR(&timerMux2);
  // 割込みカウンタをインクリメント
  interruptCounter2++;
  // トータルカウンタをインクリメント
  _100msecCounterAtWaterServe++;
  portEXIT_CRITICAL_ISR(&timerMux2);
}
//-----------------
// LED点滅用の関数定義
//-----------------
void ULP_BLINK_RUN() {
  const ulp_insn_t  ulp_program[] = {
    M_LABEL(1),  // loop1 
    I_WR_REG(RTC_GPIO_OUT_REG, 26, 30, 9), // RTC_GPIO2,12 = 1
    I_MOVI(R0, 0),  // カウンタ(R0)を初期化
    M_LABEL(2),  // loop2 (点灯)
    I_DELAY(65535), // 待つ
    I_ADDI(R0,R0,1), // カウンタに１足す
    M_BL(2, 10), // R0<10ならloop2へもどる
    I_WR_REG(RTC_GPIO_OUT_REG, 26, 29, 0),// RTC_GPIO2,12 = 0       
    I_MOVI(R0, 0), // カウンタ(R0)を初期化
    M_LABEL(3), // loop3 (消灯)
    I_DELAY(65535), // 待つ
    I_ADDI(R0,R0,1),// カウンタに１足す
    M_BL(3, 200), // R0<200ならloop3へもどる
    M_BX(1) // loop1へもどる（無限ループ）
  };
  const gpio_num_t led_gpios[] = {
    GPIO_NUM_2,
    GPIO_NUM_12
  };
  for (size_t i = 0; i < sizeof(led_gpios) / sizeof(led_gpios[0]); ++i) {
    rtc_gpio_init(led_gpios[i]);
    rtc_gpio_set_direction(led_gpios[i], RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(led_gpios[i], 0);
  }
  size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  ulp_process_macros_and_load(0, ulp_program, &size);
  ulp_run(0);
}
void ULP_BLINK_WARING() {
  const ulp_insn_t  ulp_program[] = {
    M_LABEL(1),  // loop1 
    I_WR_REG(RTC_GPIO_OUT_REG, 26, 30, 9), // RTC_GPIO2,12 = 1
    I_MOVI(R0, 0),  // カウンタ(R0)を初期化
    M_LABEL(2),  // loop2 (点灯)
    I_DELAY(65535), // 待つ
    I_ADDI(R0,R0,1), // カウンタに１足す
    M_BL(2, 10), // R0<5ならloop2へもどる
    I_WR_REG(RTC_GPIO_OUT_REG, 26, 29, 0),// RTC_GPIO2,12 = 0       
    I_MOVI(R0, 0), // カウンタ(R0)を初期化
    M_LABEL(3), // loop3 (消灯)
    I_DELAY(65535), // 待つ
    I_ADDI(R0,R0,1),// カウンタに１足す
    M_BL(3, 30), // R0<30ならloop3へもどる
    M_BX(1) // loop1へもどる（無限ループ）
  };
  const gpio_num_t led_gpios[] = {
    GPIO_NUM_2,
    GPIO_NUM_12
  };
  for (size_t i = 0; i < sizeof(led_gpios) / sizeof(led_gpios[0]); ++i) {
    rtc_gpio_init(led_gpios[i]);
    rtc_gpio_set_direction(led_gpios[i], RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(led_gpios[i], 0);
  }
  size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  ulp_process_macros_and_load(0, ulp_program, &size);
  ulp_run(0);
}
void ULP_BLINK_STOP() {
  const ulp_insn_t  ulp_program[] = {
    I_WR_REG(RTC_GPIO_OUT_REG, 26, 29, 0),// RTC_GPIO2,12 = 0       
    I_HALT() // END COPROCESSOR
  };
  const gpio_num_t led_gpios[] = {
    GPIO_NUM_2,
    GPIO_NUM_12
  };
  for (size_t i = 0; i < sizeof(led_gpios) / sizeof(led_gpios[0]); ++i) {
    rtc_gpio_init(led_gpios[i]);
    rtc_gpio_set_direction(led_gpios[i], RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(led_gpios[i], 0);
  }
  size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  ulp_process_macros_and_load(0, ulp_program, &size);
  ulp_run(0);
}
//-----------------
// sort用の関数定義
//-----------------
void asc_sort(int a[], int size) {
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    int t = a[o];
                    a[o] = a[o+1];
                    a[o+1] = t;
                }
        }
    }
}//-----------------
// PWM制御
//-----------------
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty;
  if (value < valueMax){ 
    duty= (8191 / valueMax) * value;
  } else {
    duty= (8191 / valueMax) * valueMax;    
  }
  // write duty to LEDC
  ledcWrite(channel, duty);
}
void setup_pwm() {
  pinMode(LED_PIN, OUTPUT);      // set the pin mode
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);  
}
//-----------------
// 電源電圧の読みとり
//-----------------
double ReadVoltage(){
  const int nmx=5;
  double cnt=0.0;
  double reading,sx=0;
  int i;

  adcAttachPin(ADC_PIN);
  analogSetClockDiv(255); // 1338mS
  for (i=1;i<=nmx;i++) {
    reading = analogRead(ADC_PIN); // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
    if(reading > 0 && reading <= 4095) {
      sx = sx + reading;
      cnt = cnt + 1.0;
    }
  }
  if (cnt>0) {
    reading = sx/cnt;
    // Added an improved polynomial, use either, comment out as required
    return -0.000000000000016 * pow(reading,4) + 0.000000000118171 * pow(reading,3)- 0.000000301211691 * pow(reading,2)+ 0.001109019271794 * reading + 0.034143524634089;
  } else {
      return 0;    
  }
}
 
void CheckVoltageLow(){
  if (ReadVoltage() <= 2.0)  {
    // 点滅中止
    ULP_BLINK_STOP();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); // Sleep forever
    esp_deep_sleep_start();
  } else if (ReadVoltage() <= 2.3)  {
    ULP_BLINK_WARING();
  }
}
//-----------------
// HTML内プレースホルダー書き換え
//-----------------
String processor(const String& var)
{
  String  serve_mode_string;
  String  serve_time_string;
  String  message;
  int i;
  double voltage;
  
  if(var == "VOLT"){
    voltage = ReadVoltage()/2.0;
    message="電池一本あたりの電圧は" + String(voltage) + "Vです";
    return String(message);
  }
  if(var == "RESULT"){
      voltage = ReadVoltage()/2.0;
      if (voltage>1.4) {
        message="電圧は十分です。電池交換の必要はありません。(電池一本あたりの電圧測定結果は" + String(voltage) + "Vでした）";
      } else if (voltage<1.1)  { 
        message="<strong>新しい電池への交換をおすすめします</strong>（電池一本あたりの電圧測定結果は<mark>" + String(voltage) + "V</mark>でした）";
      } else if (voltage<1.3) {
        message="<strong>乾電池をお使いの場合には、新しい電池への交換をおすすめします。</strong>※ 充電式電池をお使いの場合には、交換の必要はありません。（電池一本あたりの電圧測定結果は<mark>" + String(voltage) + "V</mark>でした）";
      }
      return String(message);
  }
  if(var == "SERVE_MODE"){
    if        (serve_interval==0)  {serve_mode_string="毎日      ";
    } else if (serve_interval==1)  {serve_mode_string="2日に１度 ";
    } else if (serve_interval==2)  {serve_mode_string="3日に１度 ";
    } else if (serve_interval==3)  {serve_mode_string="4日に１度 ";
    } else if (serve_interval==4)  {serve_mode_string="5日に１度 ";
    } else if (serve_interval==5)  {serve_mode_string="6日に１度 ";
    } else if (serve_interval==6)  {serve_mode_string="7日に１度 ";
    } else if (serve_interval==9) {serve_mode_string="10日に１度";
    } else if (serve_interval==13) {serve_mode_string="14日に１度";
    } else if (serve_interval==20) {serve_mode_string="21日に１度";
    } else if (serve_interval==29) {serve_mode_string="30日に１度";
    } 
    return serve_mode_string;
  }
  if(var == "SERVE_TIME"){
    for(int i=0;i<serve_times;i++){  
      serve_time_string=serve_time_string + String(serve_time[i]) + ":00 " ;
    }
    return serve_time_string;
  }
  if(var == "WATER_VOLUME"){
    return String(serve_volume)+"秒@"+String(serve_voltage/2.0)+"V/cell";
  }
  if(var == "NEXT_TIME"){
    return String(waitCounts/(60.0*60.0))+"時間後の"+next_serve_time+":00";
  }
  return String();
}
//-----------------
// 水やり動作
//-----------------
void ServeWater(){
  double voltage;
  double voltage_compensation;
  // 電圧測定
  voltage = ReadVoltage();
  voltage_compensation = serve_voltage/voltage;
  // カウンタリセット
  timer2 = timerBegin(0, 80, true); // タイマー0,プリスケーラ80,インクリメント
  timerAttachInterrupt(timer2, &onTimerAtWaterServe, true);
  timerAlarmWrite(timer2, 100000, true); // 100msec間隔の割込
  portENTER_CRITICAL(&timerMux2);
  _100msecCounterAtWaterServe=0;
  portEXIT_CRITICAL(&timerMux2);
  // 給水時タイマー開始
  timerAlarmEnable(timer2); 
  // 給水開始
  setup_pwm();
  digitalWrite(BASE_POWER_PIN, HIGH);    
  ledcAnalogWrite(LEDC_CHANNEL_0, BRIGHTNESS);
  // 電圧ｘ時間分経過するまで待つ
  while(1){
    if(interruptCounter2>0){
      // 給水割込みカウンタをクリア
      portENTER_CRITICAL(&timerMux2);
      interruptCounter2--;
      portEXIT_CRITICAL(&timerMux2);
      // 給水終了時間に達したかどうかチェック
      if ((double)_100msecCounterAtWaterServe > (double)serve_volume*10.0*voltage_compensation) {
        Serial.println("給水ループからブレイクアウトします");
        break;
      }
    }
  }
  // 給水停止
  digitalWrite(BASE_POWER_PIN, LOW);    
  ledcAnalogWrite(LEDC_CHANNEL_0, 0);
  // 給水時タイマー停止
  timerAlarmDisable(timer2);     
}
//---------------------------------------------------------------------------------
//                                セットアップ
//---------------------------------------------------------------------------------
void setup(){
  
//タイマー0初期設定
  timer1 = timerBegin(0, 80, true); // タイマー0,プリスケーラ80,インクリメント
  timerAttachInterrupt(timer1, &onTimerAtWakeUp, true);
  timerAlarmWrite(timer1, 1000, true); // 1msec間隔の割込
  // タイマー開始
  timerAlarmEnable(timer1); 
  // カウンタをクリア
  portENTER_CRITICAL(&timerMux1);
  _1msecCounterAtWakeUp = 0;
  portEXIT_CRITICAL(&timerMux1);

// ピンモード
  Serial.begin(115200);
  pinMode(HEART_BEAT_PIN1,OUTPUT);
  pinMode(HEART_BEAT_PIN2,OUTPUT);
  pinMode(BASE_POWER_PIN,OUTPUT);

// RTCキャリブレーション
  const uint32_t cal_count = 1000;
  const float factor = (1 << 19) * 1000.0f;
  uint32_t cali_val;
  printf("calibrate");
  cali_val = rtc_clk_cal(RTC_CAL_RTC_MUX, cal_count);
  printf("%.3f kHz\n", factor / (float)cali_val);
  cali_factor = ( factor / (float)cali_val ) / 150.0;
  Serial.println(cali_factor);

// Sleepからの復帰かどうかを判定
  if (rtc_get_reset_reason(0)==5) return;  // すぐにループに飛ぶ
//
// ブラウザ入力モード
//
  timerAlarmDisable(timer1); 
  setup_pwm();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, subnet); // IPアドレス、ゲートウェイ、サブネットマスクの設定  
  WiFi.softAP(ssid);
  delay(500); 
  if(!SPIFFS.begin()){
       Serial.println("An Error has occurred while mounting SPIFFS");
       return;
  }
  Serial.println(WiFi.softAPIP());

  server.onNotFound([](AsyncWebServerRequest *request){
    serve_voltage = ReadVoltage(); // 電圧を測る
    request->send(SPIFFS, "/top.html", "text/html", false, processor);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // ページ遷移    
    serve_voltage = ReadVoltage(); // 電圧を測る
    request->send(SPIFFS, "/top.html", "text/html", false, processor);    
  });
  
  server.on("/top.html", HTTP_GET, [](AsyncWebServerRequest *request){
    serve_voltage = ReadVoltage(); // 電圧を測る
    // ページ遷移    
    request->send(SPIFFS, "/top.html", "text/html", false, processor);    
  });
  
  server.on("/base.css", HTTP_GET, [](AsyncWebServerRequest *request){
    // ページ遷移    
    request->send(SPIFFS, "/base.css", "text/css");
  });
  
  server.on("/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/jquery-3.4.1.min.js", "text/javascript");
  });
  
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    // 給水開始
    digitalWrite(BASE_POWER_PIN, HIGH);    
    ledcAnalogWrite(LEDC_CHANNEL_0, BRIGHTNESS);
    Serial.print("/start");
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    // 給水停止
    digitalWrite(BASE_POWER_PIN, LOW);    
    ledcAnalogWrite(LEDC_CHANNEL_0, 0);
    Serial.print("/stop");
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/serve_interval.html", HTTP_GET, [](AsyncWebServerRequest *request){
    // ページ遷移    
    request->send(SPIFFS, "/serve_interval.html", "text/html");
  });
  
  server.on("/serve_time.html", HTTP_GET, [](AsyncWebServerRequest *request){
    int paramsNr = request->params();
    serve_interval=0;            
    // 水やり日の間隔を格納しておく
    for(int i=0;i<paramsNr;i++){  
      AsyncWebParameter* p = request->getParam(i);
      if (p->name()=="interval") {
        serve_interval=p->value().toInt();            
      } 
    }
    // ページ遷移    
    request->send(SPIFFS, "/serve_time.html", "text/html");
  });
  
  server.on("/watervolume.html", HTTP_GET, [](AsyncWebServerRequest *request){
    int paramsNr = request->params();
    int j=0;
    int serve_time0[24];
    Serial.println(paramsNr);   
    // 水やり時刻を格納しておく
    for(int i=0;i<paramsNr;i++){  
        AsyncWebParameter* p = request->getParam(i);
        if (p->name()=="time"){
          serve_time0[j]=p->value().toInt();
          Serial.println(serve_time0[j]);
          j++;
        }
    }
    serve_times = j;
    // 昇順にソートしておく。
    int arry_length = serve_times;
    asc_sort(serve_time0, arry_length);
    for(int i=0;i<serve_times;i++){
      serve_time[i]=serve_time0[i];
      Serial.print("i=");
      Serial.println(i);
      Serial.print("serve_time0[i]=");    
      Serial.println(serve_time0[i]);    
    }
    // ページ遷移    
    serve_voltage = ReadVoltage(); // 直前にもう一度電圧を測る
    request->send(SPIFFS, "/watervolume.html", "text/html");    
  });
  
  server.on("/summary.html", HTTP_GET, [](AsyncWebServerRequest *request){
    int paramsNr = request->params();
    // 水やり量（＝ON秒数）を格納しておく    
    AsyncWebParameter* p = request->getParam(0);
    for(int i=0;i<paramsNr;i++){  
      Serial.println(paramsNr);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      if (p->name()=="watervolume") {      
        serve_volume = p->value().toDouble();
        Serial.println("serve_volume="+String(serve_volume));
      }
    }

    // プレースホルダーを変更してページ遷移    
    request->send(SPIFFS, "/summary.html", "text/html", false, processor);    
  });
  
  server.on("/timer_start.html", HTTP_GET, [](AsyncWebServerRequest *request){
    int paramsNr = request->params();
    boolean serve_today = false;
    // 現在時刻の把握    
    for(int i=0;i<paramsNr;i++){  
        AsyncWebParameter* p = request->getParam(i);
        if(p->name()=="year"){
            year = p->value().toInt();
        } else if(p->name()=="month"){
            month = p->value().toInt();
        } else if(p->name()=="date"){
            date = p->value().toInt();
        } else if(p->name()=="hours"){
            hours = p->value().toInt();
        } else if(p->name()=="min"){
            minutes = p->value().toInt();
        } else if(p->name()=="sec"){
            seconds = p->value().toInt();
        }
    }
    // 次回実行時刻の算出
/*    Serial.print("current time");
    Serial.println(hours);*/
    next_serve_time_index=0;      
/*    Serial.print("serve_time");
    Serial.println(serve_time[0]);*/
    next_serve_time_index=0;      
    for(int i=0;i<serve_times;i++){
      if(serve_time[i]>hours){
        next_serve_time_index=i;
        serve_today=true;
        break;
      }
    }
    next_serve_time=serve_time[next_serve_time_index];
    if (!serve_today){ // 翌日以降の場合
      waitCounts=24*60*60 + 
                 (long)serve_interval*24*60*60 + 
                 (long)next_serve_time*60*60 - 
                 (long)hours*60*60 - 
                 minutes*60 - 
                 seconds;
    } else { // 当日中の場合
      waitCounts=(long)next_serve_time*60*60 - ((long)hours*60*60+minutes*60+seconds);      
    }
    // タイマー状態へ移行    
    timer_go = true; 
    // プレースホルダーを変更してページ遷移    
    request->send(SPIFFS, "/timer_start.html", "text/html", false, processor); 
  });
  
  //ウェブサーバーループ
  server.begin();
  
  while(1){
     if(timer_go==true) break;
  }
  delay(1000);  // 最後のページを読み込むまで待つ
  WiFi.mode(WIFI_OFF); // Wifi Off

  // タイマー開始
  timerAlarmEnable(timer1); 
  // カウンタをクリア
  portENTER_CRITICAL(&timerMux1);
  _1msecCounterAtWakeUp = 0;
  portEXIT_CRITICAL(&timerMux1);
  // LED点滅開始
  ULP_BLINK_RUN(); 
}
//---------------------------------------------------------------------------------
//                                 メインループ
//---------------------------------------------------------------------------------
void loop(){
  int current_time=next_serve_time;
  long sleepCount;
  long last_msec;
  Serial.println("Wakeup!"); 
/*    for(int i=0;i<serve_times;i++){
      Serial.print("i=");
      Serial.println(i);
      Serial.print("serve_time[i]=");    
      Serial.println(serve_time[i]);    
    }
  Serial.print("bootCount:");
  Serial.println(bootCount); */
  // 作動時刻になったかどうか判定する
  if(bootCount >= waitCounts){
/*    Serial.print("★serve_time has come!:");
    Serial.println(next_serve_time); */
    // 次回作動時刻の設定
    current_time=next_serve_time;
    if (next_serve_time_index == serve_times - 1) { // 翌日以降の場合
/*      Serial.println("★next serve time is beyond 24:00 !");*/
      next_serve_time_index=0;        
      next_serve_time=serve_time[0];
      waitCounts=24*60*60 +
                 (long)serve_interval*24*60*60 + 
                 (long)next_serve_time*60*60 - 
                 (long)current_time*60*60;
    } else { // 当日中の場合
/*      Serial.println("★next serve time is brefore 24:00 !");*/
      next_serve_time_index++;
      next_serve_time=serve_time[next_serve_time_index];
      waitCounts=(long)next_serve_time*60*60 - 
                 (long)current_time*60*60;
    }
/*    Serial.print("★next_serve_time_index:");
    Serial.println(next_serve_time_index); 
    Serial.print("★next_serve_time:");
    Serial.println(next_serve_time); 
    Serial.print("★Start serving.. waitCounts: ");
    Serial.println(waitCounts);*/
    // 点滅中止
    ULP_BLINK_STOP();
    // 給水実施
    pinMode(BASE_POWER_PIN,OUTPUT);
    setup_pwm();
/*    Serial.println("★Enter ServeWater");*/
    ServeWater();
    // bootCountをリセット
    bootCount=0;
    // 点滅再開    
    ULP_BLINK_RUN();
  } 
  // 10スリープごとに電源電圧を確認
  if(bootCount % 10 == 0){
    CheckVoltageLow();
  }
  // 経過時間をbootCountに足す
  // 次のスリープ時間を計算する
  cali_factor  = 1.0;
  if(bootCount + sleep_Sec >= waitCounts){ // デフォルトスリープでは行き過ぎるケース
    last_msec = waitCounts*1000 - bootCount*1000 - _1msecCounterAtWakeUp;
    sleepCount=last_msec * (long)uS_TO_mS_FACTOR;
    bootCount=waitCounts;
  } else {
    sleepCount = ( sleep_Sec*1000 - _1msecCounterAtWakeUp ) * (long)uS_TO_mS_FACTOR;
    bootCount=bootCount+sleep_Sec;
  }
/*  Serial.print("★ bootCount:");
  Serial.println(bootCount); 
  Serial.print(" waitCounts:");
  Serial.println(waitCounts); 
  Serial.print(" sleepCount:");
  Serial.println(sleepCount); 
  Serial.print(" _1msecCounterAtWakeUp:");
  Serial.println(_1msecCounterAtWakeUp); */
  // Deep sleep にはいる
/*  Serial.println("enable sleep..."); */
  esp_sleep_enable_timer_wakeup(sleepCount);
/*  Serial.println("go to sleep..."); */
  ULP_BLINK_RUN(); 
  esp_deep_sleep_start();     
}
