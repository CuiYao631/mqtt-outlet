/*
 * @Author: CuiYao
 * @Date: 2023-09-04 10:37:44
 * @Last Modified by:   CuiYao
 * @Last Modified time: 2023-09-04 10:37:44
 * Wi-Fi智能插座 使用 MQTT 协议通讯
 */

#include <ESP8266WiFi.h>
#include <EasyButton.h>
#include <Ticker.h>
#include <ESP8266httpUpdate.h>
#include "PubSubClient.h" //加载MQTT头文件
#include "wifi_info.h"    //加载WIFI

#define LED_PIN 2 // 引脚定义
#define BTN_PIN 0 // Button PIN

const char *mqttServer = "192.168.100.191"; // 服务器地址
const int mqttPort = 1883;                  // 服务器端口
const int duration = 5000;                  // Button长按触发时间

const char *msgTopic = "msgTopic";                           // 消息主题
const char *inUseTopic = "inUseTopic";                       // 使用
const char *wattsTopic = "wattsTopic";                       // 瓦数
const char *voltsTopic = "voltsTopic";                       // 电压
const char *amperesTopic = "amperesTopic";                   // 电流
const char *totalConsumptionTopic = "totalConsumptionTopic"; // 状态修改主题
const char *onlineTopic = "onlineTopic";                     // 在线通知主题
const char *updateBinTopic = "updateBinTopic";               // OTA 升级通知 

String upUrl = "http://bin.bemfa.com/b/1BcOWMxMWI1Y2U4ZDk5MjVjZjkzZDRhMmU2MDIzMDM5ZDk=outlet001.bin";

int ON_OFF = 0; // 1:ON/0:OFF

Ticker ticker;                      // 初始化定时器
WiFiClient espClient;               // WIFI初始化
PubSubClient mqttClient(espClient); // MQTT初始化
EasyButton button(BTN_PIN);         // 开关初始化

void setup()
{
  pinMode(LED_PIN, OUTPUT);                            // 设置引脚为输出模式
  Serial.begin(115200);                                // 初始化串口
  ticker.attach(0.1, buttonRead);                      // 定时器
  wifi_connect();                                      // 初始化wifi
  button.onPressed(onPressed);                         // 定义按键单按事件回调
  button.onPressedFor(duration, onPressedForDuration); // 定义按键长按事件回调
  mqttClient.setServer(mqttServer, mqttPort);          // 设置MQTT服务器和端口号
  connectMQTTServer();                                 // 连接MQTT服务器
  mqttClient.setCallback(callback);                    // 设置回调函数
  digitalWrite(LED_PIN, HIGH);                         // 默认为关闭
  Serial.println("Initialization completed (^_^) ");
}

// 单击事件函数
void onPressed()
{
  if (ON_OFF == 1)
  {
    Serial.println("off");
    ON_OFF = 0;
    mqttClient.publish(msgTopic, "0");
    mqttClient.publish(inUseTopic, "0");
    OFF();
  }
  else
  {
    Serial.println("on");
    ON_OFF = 1;
    mqttClient.publish(msgTopic, "1");
    mqttClient.publish(inUseTopic, "1");
    ON();
  }
}
// 长按事件函数
void onPressedForDuration()
{
  // 清楚WIFI
  wifi_reset();

}

// MQTT回调函数
void callback(char *topic, byte *payload, unsigned int length)
{
  String msg = "";
  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  Serial.print("Msg:");
  Serial.println(msg);
  if (msg=="ota_update"){ //如果通知升级
      updateBin();
  }

  if (msg == "1")
  {
    Serial.println("on");
    ON_OFF = 1;
    ON();
  }
  if (msg == "0")
  {
    Serial.println("off");
    ON_OFF = 0;
    OFF();
  }
  msg = "";
}

// 连接MQTT服务器
void connectMQTTServer()
{
  // 根据ESP8266的MAC地址生成客户端ID（避免与其它ESP8266的客户端ID重名）
  String clientId = "esp8266-" + WiFi.macAddress();

  String willMsg = clientId + "lost connection";
  // 连接MQTT服务器
  if (mqttClient.connect(clientId.c_str(), onlineTopic, 1, false, willMsg.c_str()))
  { // 设置mqtt主题的id
    // 连接成功后就订阅主题 、通知设备已上线
    mqttClient.publish(onlineTopic, "1");
    mqttClient.subscribe(msgTopic); // 订阅主题
    mqttClient.subscribe(updateBinTopic); // 订阅更新主题
    Serial.print("订阅主题成功！！");
  }
  else
  {
    Serial.print("mqttClient-state");
    Serial.println(mqttClient.state());
    delay(3000);
  }
}

// 打开灯泡
void ON()
{
  digitalWrite(LED_PIN, LOW);
}
// 关闭灯泡
void OFF()
{
  digitalWrite(LED_PIN, HIGH);
}

// 检测按键状态(定时器)
void buttonRead()
{
  button.read();
}
void loop()
{
  if (mqttClient.connected())
  {                    // 如果设备成功连接服务器
    mqttClient.loop(); // 保持客户端心跳
  }
  else
  {                      // 如果设备未能成功连接服务器
    connectMQTTServer(); // 则尝试连接服务器
  }
}


//当升级开始时，打印日志
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

//当升级结束时，打印日志
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

//当升级中，打印日志
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

//当升级失败时，打印日志
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
//OTA升级程序
void updateBin(){
  Serial.println("start update");    
  
  ESPhttpUpdate.onStart(update_started);//当升级开始时
  ESPhttpUpdate.onEnd(update_finished); //当升级结束时
  ESPhttpUpdate.onProgress(update_progress); //当升级中
  ESPhttpUpdate.onError(update_error); //当升级失败时
  
   
  t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, upUrl);
  Serial.println(ret);
  Serial.println("测试2");  
  switch(ret) {
    case HTTP_UPDATE_FAILED:      //当升级失败
        Serial.println("[update] Update failed.");
        break;
    case HTTP_UPDATE_NO_UPDATES:  //当无升级
        Serial.println("[update] Update no Update.");
        break;
    case HTTP_UPDATE_OK:         //当升级成功
        Serial.println("[update] Update ok.");
        break;
  }
}

