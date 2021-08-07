#include <ESP8266WiFi.h>
#include <MyRealTimeClock.h>
#include <PubSubClient.h>
#include<EEPROM.h>

#include<ArduinoJson.h>
#include "DHT.h"
//  DHT22
#define DHTPIN1 D7
#define DHTTYPE DHT22

#define DHTPIN2 D6
#define DHTTYPE DHT22
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);
//EEPROM
String RL = "";// keb kha khong relay khrng fan
String data;
String date = "", t = "", tstart = "", Delay = "";
int hou = 0, minut = 0, sec = 0, statuRL = 0; //Todo: The 'statuRL' use for chack the Relay1 is 'HIGH' or 'LOW' when work on auto mode
// การปะกาดโตเปี่ยนไวใข้ในสวนของ DHT22
String h1, t1, hi1, h2, t2, hi2;

//ขา pin ต่างๆ2
#define relay1 D0
#define relay2 D4
#define relay3 D5


//สวนของ Clock module
MyRealTimeClock myRTC(5, 4, 0);
String datetime;

//Connect wifi
const char *ssid =  "tnp1999";
const char *pass =  "12345678";

//พากสวนกานเชื่อมต่อ MQTT
const char* mqtt_server = "mqtt.mounoydev.com";//const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_user = "mn";
const char* mqtt_pass = "mn";
char* Device_key = "mroom";
const char* topicOut = "inTopiclao";
const char* topicInit = "inTopiclao";
const char* topicWarning = "warnTop";

WiFiClient espClient;
PubSubClient client(espClient);
#define SerialMon Serial
unsigned long last_time = 0;
boolean JustOne = true;
uint32_t lastReconnectAttempt = 0;
// มาจาก mounoy_test
unsigned long lastMsg = 0, timer = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

//Todo: Test
String autoTimer = "";

struct ObManual {
  String pin;
  int timer;
  int statusUse;
};

struct ObManual mHumidity;
struct ObManual mFanIn;
struct ObManual mFanOut;

struct ObSettingAuto {
  char id[4];
  char pin[2];
  char value[2];
  char statusUse[1];
};

struct ObRelay {
  char id[4];
  char pin[2];
  char startTimer[4];
  char statusWork[1];
  char statusUse[1];
};
//Todo: End test

//พากสวนของ pubisher  ที่ส่งค่าไป Arduino เพื่อสั่งกาน
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived: [");
  Serial.println("Topic: " + String(topic));
  String tdata = (char*)payload;
  Serial.println("Length: " + String(tdata.length()));
  Serial.println("Data: " + tdata + "\n]");

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char* actionKey = doc["key"];
  int key = String(actionKey).toInt();
  if (key == 0) { //Todo: if key=1 is set the manual config for three Relay_1_2_3
    const char* pin = doc["pin"];
    const char* val = doc["val"];
    const char* stUse = doc["stuse"];
    if (String(pin).equals("D0")) {
      mHumidity.pin = String(pin);
      mHumidity.timer = String(val).toInt();
      mHumidity.statusUse = String(stUse).toInt();
      if (mHumidity.timer == 1)
        digitalWrite(relay1, HIGH);
      else
        digitalWrite(relay1, LOW);
    } else if (String(pin).equals("D4")) {
      mFanIn.pin = String(pin);
      mFanIn.timer = String(val).toInt();
      mFanIn.statusUse = String(stUse).toInt();
      if (mFanIn.timer == 1)
        digitalWrite(relay2, HIGH);
      else
        digitalWrite(relay2, LOW);
    } else if (String(pin).equals("D5")) {
      mFanOut.pin = String(pin);
      mFanOut.timer = String(val).toInt();
      mFanOut.statusUse = String(stUse).toInt();
      if (mFanOut.timer == 1)
        digitalWrite(relay3, HIGH);
      else
        digitalWrite(relay3, LOW);
    }
  }
  else if (key == 1) { //Todo: if key=1 is set the setting auto config for three Relay_1
    const char* id = doc["id"];
    const char* pin = doc["pin"];
    const char* val = doc["val"];
    const char* stUse = doc["stuse"];

    struct ObSettingAuto putAuto;
    strcpy(putAuto.id, String(id).c_str());
    strcpy(putAuto.pin, String(pin).c_str());
    strcpy(putAuto.value, String(val).c_str());
    strcpy(putAuto.statusUse, String(stUse).c_str());

    if (String(pin).equals("D0")) {
      EEPROM.put(0, putAuto);
      if (EEPROM.commit() == true)
        Serial.println("Put auto setting at index: 0-" + String(sizeof(putAuto)));
      int statusWork = String(stUse).toInt();
      if (statusWork == 0) {
        digitalWrite(relay1, LOW);
      } else {
        digitalWrite(relay1, HIGH);
      }
    } else  Serial.println("Auto setting not put");/*else if (String(pin).equals("D4")) {
      EEPROM.put(9, putAuto);
      if (EEPROM.commit() == true)
        Serial.println("Put auto setting at index: 9-" + String(9 + sizeof(putAuto)));
    } else if (String(pin).equals("D5")) {
      EEPROM.put(18, putAuto);
      if (EEPROM.commit() == true)
        Serial.println("Put auto setting at index: 18-" + String(18 + sizeof(putAuto)));
    }*/
  }
  else if (key == 2) { //Todo: if key=2 is set the setting list config for three Relay_1_2_3
    const char* id = doc["id"];
    const char* pin = doc["pin"];
    const char* startT = doc["timer"];
    const char* stWork = doc["stwork"];
    const char* stUse = doc["stuse"];
    const char* event = doc["event"];
    struct ObRelay putRL;
    strcpy(putRL.id, String(id).c_str());
    strcpy(putRL.pin, String(pin).c_str());
    strcpy(putRL.startTimer, String(startT).c_str());
    strcpy(putRL.statusWork, String(stWork).c_str());
    strcpy(putRL.statusUse, String(stUse).c_str());

    //Todo: Pin = D0
    if (String(pin).equals("D0")) {
      for (int i = 31; i < 151; i += sizeof(putRL)) {//i += 12 is the memory index of EEPROM use for stored object 'ObRelay' or use 'sizeof(putRL)' index for stored.
        if (String(event).equals("put")) {
          if (InsertRLVal(31, i, (151 - sizeof(putRL)), putRL) >= 1)//Todo: If return values more than 0 is save completed
            return;
        } else if (String(event).equals("del")) {
          if (deleteRLVal(i, (char*)id, (char*)pin) >= 1)
            return;
        }
      }
    }
    //Todo: Pin = D4
    else if (String(pin).equals("D4")) {
      for (int i = 151; i < 271; i += sizeof(putRL)) {
        if (String(event).equals("put")) {
          if (InsertRLVal(151, i, (271 - sizeof(putRL)), putRL) >= 1)
            return;
        } else if (String(event).equals("del")) {
          if (deleteRLVal(i, (char*)id, (char*)pin) >= 1)
            return;
        }
      }
    }
    //Todo: Pin = D5
    else if (String(pin).equals("D5")) {
      for (int i = 271; i < 391; i += sizeof(putRL)) {
        if (String(event).equals("put")) {
          if (InsertRLVal(271, i, (391 - sizeof(putRL)), putRL) >= 1)
            return;
        } else if (String(event).equals("del")) {
          if (deleteRLVal(i, (char*)id, (char*)pin) >= 1)
            return;
        }
      }
    } else
      Serial.println("Pin: " + String(pin) + " not match in bord");
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//พากสวนการเชื่อมภานจช้มูน
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {   //   boolean connect(const char* id, const char* user, const char* pass);
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe(topicInit);
      client.publish(topicOut, "hello world Laos");  //publish funtion to outTopiclao
      client.publish(topicWarning, "getGonfig");  //publish funtion to outTopiclao
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

char* string2char(String command) {
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay1, OUTPUT);
  //สั่งให้ไฟดับก่อน `
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  dht1.begin();
  dht2.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //  myRTC.setDS1302Time(00, 37, 16, 6, 5, 8, 2021);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  //  Control();
  client.loop();
  unsigned long now = millis();

  //Todo: Start Test
  if (now - timer >= 60000) {
    timer = now;

    //    client.publish(topicWarning, "{\"mPin\":"+mHumidity.pin+",\"mTimer\":"+mHumidity.timer+",\"mStatus\":"+mHumidity.statusUse+"}");

    //    String manualFanIn="{\"mPin\":"+mFanIn.pin+",\"mTimer\":"+mFanIn.timer+",\"mStatus\":"+mFanIn.statusUse+"}";
    //    client.publish(topicWarning, manualFanIn);
    //
    //    String manualFanOut="{\"mPin\":"+mFanOut.pin+",\"mTimer\":"+mFanOut.timer+",\"mStatus\":"+mFanOut.mFanOut+"}";
    //    client.publish(topicWarning, manualFanOut);
  }

  //  resetEEPROM(0,512);//Todo: Reset EEPROM
  Clock();
  ChackTimerForWork();
  //Todo: End Test

  if (now - lastMsg >= 10000) {
    //    lastMsg = now;
    //    if (Sensor1() == "nan" && Sensor2() == "nan")
    //      client.publish(topicWarning, "Sensor 1 and Sensor 2 not connect");
    //    else if (Sensor1() == "nan")
    //      client.publish(topicWarning, "Sensor 1 not connect");
    //    else if (Sensor2() == "nan")
    //      client.publish(topicWarning, "Sensor 2 not connect");
    //
    //    String strtm = String(Device_key) + "," +  Sensor1() + "," + Sensor2();
    //    client.publish(topicOut, string2char(strtm));      //PUSH DATA TO MQTT
    //    SerialMon.println(string2char(strtm));

    //
    //    //  JustOne = false;
    //    // last_time = millis();
  }
  //
  Sensor1();
  //  Serial.println("DHT1=>humidity: "+h1+", temperature: "+t1+", heatindex: "+hi1);
  //Serial.println("DHT2  : "+h2+","+t2+","+hi2);
}
String Sensor1() {
  delayMicroseconds(2000);
  float humidity = dht1.readHumidity();
  float temperature = dht1.readTemperature();
  float heatindex = dht1.computeHeatIndex(temperature, humidity, false);
  h1 += " " + String(humidity);
  t1 += " " + String(temperature);
  hi1 += " " + String(heatindex);
  if (isnan(humidity) || isnan(temperature) || isnan(heatindex))
    return "nan";
  else
    return String(humidity) + "," + String(temperature);
}
String Sensor2() {
  delayMicroseconds(2000);
  float humidity = dht2.readHumidity();
  float temperature = dht2.readTemperature();
  float heatindex = dht2.computeHeatIndex(temperature, humidity, false);
  h2 += " " + String(humidity);
  t2 += " " + String(temperature);
  hi2 += " " + String(heatindex);
  if (isnan(humidity) || isnan(temperature) || isnan(heatindex))
    return "nan";
  else
    return String(humidity) + "," + String(temperature);
}
void Control() {
  float inhumidity = dht1.readHumidity();
  float temperature2 = dht2.readTemperature();
  float temperature1 = dht1.readTemperature();
  if (inhumidity <= 95.00 || isnan(inhumidity)) {

    digitalWrite(relay2, LOW);
    // SerialMon.println("low " + String(inhumidity));
  }
  else {
    digitalWrite(relay2, HIGH);
    SerialMon.println("high " + String(inhumidity));
  }
  if (temperature1 > temperature2 && (temperature1 - temperature2) > 2) {
    digitalWrite(relay2, HIGH);
    Serial.println("open RElay2");
  } else if (temperature2 > temperature1 && (temperature2 - temperature1) > 2) {
    digitalWrite(relay3, HIGH);
    Serial.println("open RElay3");
  } else {
    digitalWrite(relay3, LOW);
    digitalWrite(relay3, LOW);
  }

}
String Clock() {
  myRTC.updateTime();
  datetime = String(myRTC.dayofmonth) + "/" + String(myRTC.month)
             + "/" + String(myRTC.year) + " " + String(myRTC.hours) + ":" + String(myRTC.minutes) + ":" + String(myRTC.seconds);
  Serial.println(datetime);
  //  Serial.println("Now: " + String(String(myRTC.hours).toInt()) + ":" + String(String(myRTC.minutes).toInt()));
  if (RL != "") {

  } else {
    //Auto Controll================>
    //        int out = 0 , hu = String(tstart.substring(0, 2).toInt()), mn = String(tstart.substring(3, 5).toInt());
    //        if ((mn + String(myRTC.minutes).toInt()) >= 60) {
    //          mn = (mn + String(myRTC.minutes).toInt()) - 60;
    //          out = 1;
    //        } else
    //          mn = mn + String(myRTC.minutes).toInt();
    //
    //        if ((hu + String(myRTC.hours).toInt() + out) >= 24)
    //          hu = (hu + String(           / r / myRTC.hours).toInt() + out) - 24;
    //        else
    //          hu = (hu + String(myRTC.hours).toInt() + out);
    //
    //        autoTimer = hu + ":" + mn;
    //      }
    delay(1000);
    return datetime;
  }
}

void ChackTimerForWork() {
  struct ObSettingAuto getSettAuto;
  EEPROM.get(0, getSettAuto);
  int stAutoUse = String(getSettAuto.statusUse).substring(0, 1).toInt();
  int timeNow = (String(myRTC.hours).toInt() * 60) + String(myRTC.minutes).toInt();
  if (mHumidity.statusUse == 1) { /*Manual Work Of Relay 1*/
    Serial.println("Manual work: " + String(timeNow));
    if (mHumidity.timer > timeNow) {
      Serial.println("Manual Relay 1 is ON");
      digitalWrite(relay1, HIGH);
    } else {
      Serial.println("Manual Relay 1 is OFF");
      digitalWrite(relay1, LOW);
      mHumidity.timer = 0;
      mHumidity.statusUse = 0;
    }
    FanWork();
  }
  else if (stAutoUse == 1) { /*Work For Auto Of Humidity: (stAutoUse: 1 is ON or True, 0 is OFF or False)*/
    //Todo: get auto values
    String pin = String(getSettAuto.pin).substring(0, 2);
    int valAuto = String(getSettAuto.value).substring(0, 2).toInt();
    if (stAutoUse == 1 && pin == "D0") {
      int humidity = dht1.readHumidity();
      Serial.println("Humidity 1 is: " + String(humidity) + ", Humidity 2 is: " + String(dht2.readHumidity()));
      Serial.println("Temperature 1 is: " + String(dht1.readTemperature()) + ", Temperature 2 is: " + String(dht2.readTemperature()));
      if (humidity <= valAuto) //Todo: Open Relay 1
        digitalWrite(relay1, HIGH);
      else //Todo: Close Relay 1
        digitalWrite(relay1, LOW);
    }
    //Todo: call the funtion (/*Work For Setting Of Humidity*/)
    FanWork();
  }
  else {/*Work For Setting Of Humidity*/
    Serial.println("Work of Setting");
    struct ObRelay getRelay1;
    int timeNow = (String(myRTC.hours).toInt() * 60) + String(myRTC.minutes).toInt();
    for (int i = 31; i < 151; i += sizeof(getRelay1)) {
      EEPROM.get(i, getRelay1);
      String pin = String(getRelay1.pin).substring(0, 2);
      int timeWork = String(getRelay1.startTimer).substring(0, 4).toInt();
      int stWork = String(getRelay1.statusWork).substring(0, 1).toInt();
      int stUse = String(getRelay1.statusUse).substring(0, 1).toInt();
      if (pin.equals("D0")) {
        if (timeWork == timeNow) { /*Work Of Relay 1 if timeWork=Now*/
          if (stWork == 1  && stUse == 1)  /*Open Relay 1 if stWork=1, (1 is ON)*/
            digitalWrite(relay1, HIGH);
          else /*Close Relay 1 if stWork=0, (0 is OFF)*/
            digitalWrite(relay1, LOW);
        }
      }
    }
    //Todo: call the funtion (/*Work For Setting Of Humidity*/)
    FanWork();
  }
}
void FanWork() {/*Work Of Setting*/
  int timeNow = (String(myRTC.hours).toInt() * 60) + String(myRTC.minutes).toInt();
  if (mFanIn.statusUse == 1) { /*Work Of Manual Relay 2*/
    Serial.println("Manual work: " + String(timeNow));
    if (mFanIn.timer > timeNow) {
      Serial.println("Manual Relay 1 is ON");
      digitalWrite(relay2, HIGH);
    } else {
      Serial.println("Manual Relay 1 is OFF");
      digitalWrite(relay2, LOW);
      mFanIn.timer = 0;
      mFanIn.statusUse = 0;
    }
  }
  else if ( mFanOut.statusUse == 1 ) { /*Work Of Manual Relay 3*/
    Serial.println("Manual work: " + String(timeNow));
    if (mFanOut.timer > timeNow) {
      Serial.println("Manual Relay 1 is ON");
      digitalWrite(relay3, HIGH);
    } else {
      Serial.println("Manual Relay 1 is OFF");
      digitalWrite(relay3, LOW);
      mFanOut.timer = 0;
      mFanOut.statusUse = 0;
    }
  }
  else { /*Setting Work Relay 2 and 3*/
    struct ObRelay getRelay23;
    int timeNow = (String(myRTC.hours).toInt() * 60) + String(myRTC.minutes).toInt();
    Serial.println("Time Now: " + String(timeNow));
    for (int i = 151; i < 391; i += sizeof(getRelay23)) {
      EEPROM.get(i, getRelay23);
      String pin = String(getRelay23.pin).substring(0, 2);
      int timeWork = String(getRelay23.startTimer).substring(0, 4).toInt();
      int stWork = String(getRelay23.statusWork).substring(0, 1).toInt();
      int stUse = String(getRelay23.statusUse).substring(0, 1).toInt();
      if (pin.equals("D4")) {
        if (timeWork == timeNow) {//Todo: Relay 2 is work if the setTimer=Now
          if (stWork == 1 && stUse == 1) { /*Open Relay 2 if stWork=1, (1 is ON);*/
            digitalWrite(relay2, HIGH);
            Serial.println("Relay 2 is ON");
          }
          else { /*Open Relay 2 if stWork=0, (0 is OFF)*/
            digitalWrite(relay2, LOW);
            Serial.println("Relay 2 is OFF");
          }
        }
      }
      else if (pin.equals("D5")) {
        if (timeWork == timeNow) {//Todo: Relay 3 is work if the setTimer=Now
          if (stWork == 1 && stUse == 1) { /*Open Relay 3 if stWork=1, (1 is ON);*/
            digitalWrite(relay3, HIGH);
            Serial.println("Relay 3 is ON");
          }
          else { /*Open Relay 3 if stWork=0, (0 is OFF)*/
            digitalWrite(relay3, LOW);
            Serial.println("Relay 3 is OFF");
          }
        }
      }
    }
  }
}

int InsertRLVal(int beginIndex, int startIndex, int endIndex, ObRelay putRL) {
  struct ObRelay getRL;
  EEPROM.get(startIndex, getRL);
  String id = String(getRL.id), pin = String(getRL.pin).substring(0, 2);
  if (id == "" || (id.equals(String(putRL.id)) && pin.equals(String(putRL.pin).substring(0, 2)))) {
    EEPROM.put(startIndex, putRL);
    if (EEPROM.commit() == true) {
      Serial.println("Put ID: " + id + ", Start Index: " + String(startIndex) + ", End index: " + String(sizeof(putRL) + startIndex) + ", Size: " + String(sizeof(putRL)));
      return startIndex;
    } else return 0;
  } else if (startIndex >= endIndex) {
    //Todo: If the EEPROM don't have empty stored between index 'beginIndex' - 'endIndex' is save the data to first index of pinD0 or index 'beginIndex';
    EEPROM.put(beginIndex, putRL);
    if (EEPROM.commit() == true) {
      Serial.println("Put start index: " + String(beginIndex) + ", End index: " + String(sizeof(putRL) + beginIndex) + ", Size: " + String(sizeof(putRL)));
      return beginIndex;
    } else return 0;
  } else {
    return 0;
  }
}

int deleteRLVal(int index, char* ids, char* pins) {
  struct ObRelay getRL;
  EEPROM.get(index, getRL);
  String id = String(getRL.id), pin = String(getRL.pin).substring(0, 2);
  if (id.equals(ids) && pin.equals(pins)) {
    if (resetEEPROM(index, (index + sizeof(getRL))) == true) {
      Serial.println("Delete ID: " + id + ", Pin: " + String(pin) + ", Start Index: " + String(index) + ", End Index: " + (index + sizeof(getRL)));
      return index;
    } else return 0;
  } else {
    Serial.println("Not Delete ID: " + id + ", Pin: " + String(pin));
    return 0;
  }
}

bool resetEEPROM(int startIndex, int endIndex) {
  bool result = false;
  for (int i = startIndex; i <= endIndex; i++) {
    EEPROM.write(i, 0);
    result = EEPROM.commit();
  }
  return result;
}
