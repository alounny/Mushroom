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
String output, manualTimer = "", autoTimer = "";
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
const char *ssid =  "Dell";
const char *pass =  "12345678";

//พากสวนกานเชื่อมต่อ MQTT
const char* mqtt_server = "mqtt.mounoydev.com";//const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_user = "mn";
const char* mqtt_pass = "mn";
char* Device_key = "mrbox";
const char* topicInit = "outTopicllao";
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
int value = 0, eepWrite = 0, eepRead = 0, statusRL = 0;

//Todo: Test
struct ObRelay {
  char idDevice[3];
  char pin[2];
  char startTimer[8];
  char statusWork[1];
  char statusUse[1];
};
//Todo: End test

//พากสวนของ pubisher  ที่ส่งค่าไป Arduino เพื่อสั่งกาน
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(length);
  Serial.println("Message arrived [");
  Serial.println(topic);
  String tdata = (char*)payload;
  Serial.println("Message: " + String(tdata.length()));
  Serial.println("Message: " + tdata);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char* idDev = doc["idDev"];
  const char* pin = doc["pin"];
  const char* startT = doc["timer"];
  const char* stWork = doc["stwork"];
  const char* stUse = doc["stuse"];
  const char* event = doc["event"];

  ObRelay putRL;
  strcpy(putRL.idDevice, String(idDev).c_str());
  strcpy(putRL.pin, String(pin).c_str());
  strcpy(putRL.startTimer, String(startT).c_str());
  strcpy(putRL.statusWork, String(stWork).c_str());
  strcpy(putRL.statusUse, String(stUse).c_str());

  //Todo: Pin = D0
  if (String(pin).equals("D0") && String(idDev).equals("RL1")) {
    for (int i = 23; i < 173; i += sizeof(putRL)) {//i += 15 is the memory index of EEPROM use for stored object 'ObRelay' or use 'sizeof(putRL)' index for stored.
      if (String(event).equals("put"))
        if (InsertRLVal(23, i, (173 - sizeof(putRL)), putRL) >= 1)
          return;
        else if (String(event).equals("del")) {
          if (deleteRLVal(i, (char*)idDev, (char*)pin, (char*)startT, (char*)stWork, (char*)stUse) >= 1)
            return;
        }
    }
  }
  //Todo: Pin = D4
  else if (String(pin).equals("D4") && String(idDev).equals("RL2")) {
    for (int i = 174; i < 323; i += sizeof(putRL)) {
      if (String(event).equals("put"))
        if (InsertRLVal(174, i, (323 - sizeof(putRL)), putRL) >= 1)
          return;
        else if (String(event).equals("del")) {
          if (deleteRLVal(i, (char*)idDev, (char*)pin, (char*)startT, (char*)stWork, (char*)stUse) >= 1)
            return;
        }
    }
  }
  //Todo: Pin = D5
  else if (String(pin).equals("D5") && String(idDev).equals("RL3")) {
    for (int i = 324; i < 473; i += sizeof(putRL)) {
      if (String(event).equals("put"))
        if (InsertRLVal(324, i, (473 - sizeof(putRL)), putRL) >= 1)
          return;
        else if (String(event).equals("del")) {
          if (deleteRLVal(i, (char*)idDev, (char*)pin, (char*)startT, (char*)stWork, (char*)stUse) >= 1)
            return;
        }
    }
  } else
    Serial.println("Pin: " + String(pin));
  /*  if (data[0] == 'S' || data[0] == 's') {
      int out = 0, resualt = 0;
      hou = data.substring(2, 4).toInt();
      minut = data.substring(5, 7).toInt();
      sec = data.substring(8, 9).toInt();
      Serial.println("=========>minutes: " + String(hou) + " " + String(minut));
      if (sec + String(myRTC.seconds).toInt() >= 60) {
        sec = (sec + String(myRTC.seconds).toInt()) - 60;
        minut += 1;
      } else {
        sec = (sec + String(myRTC.seconds).toInt());
      }
      if ((minut + String(myRTC.minutes).toInt()) >= 60) {
        minut = (minut + String(myRTC.minutes).toInt()) - 60;
        out = 1;
      } else
        minut = minut + String(myRTC.minutes).toInt();

      if ((hou + String(myRTC.hours).toInt() + out) >= 24)
        hou = (hou + String(myRTC.hours).toInt() + out) - 24;
      else
        hou = (hou + String(myRTC.hours).toInt() + out);


      manualTimer = String(hou) + ":" + String(minut);
      RL = data[1];
      Serial.println("=========>Manual: " + manualTimer + ":" + String(sec));

      if (data[1] == '0') { //turn off light
        digitalWrite(relay1, LOW);
      } else if (data[1] == '1') {
        digitalWrite(relay1, HIGH);
      } else if (data[1] == '2') {
        digitalWrite(relay2, LOW);
      } else if (data[1] == '3') {
        digitalWrite(relay2, HIGH);
      } else if (data[1] == '4') {
        digitalWrite(relay3, LOW);
      } else if (data[1] == '5') {
        digitalWrite(relay3, HIGH);
      } else {
        Serial.println("Can not open");
      }
    }
    else {
      date = data.substring(0, 10);
      t = data.substring(11, 16);
      tstart = data.substring(17, 22);
      Delay = data.substring(23, 28);
      if (date != EEPROM_read(0, 10)) {
        EEPROM_write(0, date);
      }
      if (t != EEPROM_read(11, 16)) {
        EEPROM_write(11, t);
      }
      if (tstart != EEPROM_read(17, 22)) {
        EEPROM_write(17, tstart);
      }
      if (Delay != EEPROM_read(23, 28)) {
        EEPROM_write(23, Delay);
      }
    }*/
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
      client.publish(topicInit, "hello world Laos ");  //publish funtion to outTopiclao
      client.publish(topicWarning, "getGonfig");  //publish funtion to outTopiclao
      // ... and resubscribe
      client.subscribe("inTopiclao");
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
  //สั่งให้ไฟดับก่อน
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  dht1.begin();
  dht2.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  myRTC.setDS1302Time(00, 55, 16, 06, 26, 3, 2021);
  // อ่านึ่าทีบบันทืกใน EEPROM
  //  date = EEPROM_read(0, 9);
  //  t = EEPROM_read(11, 15);
  //  tstart = EEPROM_read(17, 21);
  //  Delay = EEPROM_read(24, 28);

  //Todo: Start Test

  //Todo: End Test
}
int a = 0, i = 1, j = 23;
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  //  Control();
  client.loop();
  unsigned long now = millis();
  //Todo: Chack Connection
  if (now - timer >= 10000) {
    timer = now;
    client.publish(topicWarning, "Connected");
  }
  //Todo: Start Test
  if (j <= 158) {
    ObRelay relay;
    EEPROM.get(j, relay);
    j += sizeof(relay);
    Serial.println("Device: " + String(relay.idDevice));
    Serial.println("Pin: " + String(relay.pin));
    Serial.println("Timer: " + String(relay.startTimer));
    Serial.println("Status Work: " + String(relay.statusWork));
    Serial.println("Status Use: " + String(relay.statusUse));
    Serial.println("=======> Timer: " + String(i) + " Index: " + String(j));
    i += 1;
  }
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
    //    String strtm = String(Device_key) + "" +  Sensor1() + "," + Sensor2();
    //    client.publish(topicInit, string2char(strtm));      //PUSH DATA TO MQTT
    //    SerialMon.println(string2char(strtm));
    //    Clock();
    //
    //    //  JustOne = false;
    //    // last_time = millis();
  }
  //
  //Serial.println("DHT1 :"+h1+","+t1+","+hi1);
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
    return String(humidity) + "," + String(temperature) + "," + String(heatindex);
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
    return String(humidity) + "," + String(temperature) + "," + String(heatindex);
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
  Serial.println("Now: " + String(String(myRTC.hours).toInt()) + ":" + String(String(myRTC.minutes).toInt()));
  if (RL != "") {
    //Manual Controll================>
    if (RL == "1") {
      if (manualTimer != String(String(myRTC.hours).toInt()) + ":" + String(String(myRTC.minutes).toInt()) || sec > String(myRTC.seconds).toInt()) {
        digitalWrite(relay1, HIGH);
      } else {
        digitalWrite(relay1, LOW);
        manualTimer = "";
        RL = "";
      }
    } else if (RL == "0") {
      digitalWrite(relay1, LOW);
      manualTimer = "";
      RL = "";
    }
  } else {
    //Auto Controll================>
    if (date == String(myRTC.dayofmonth) + ":" + String(myRTC.month) + ":" + String(myRTC.year) && t == String(myRTC.hours) + ":" + String(myRTC.minutes) || statusRL == 1) {
      //      if (statusRL == 0) {
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
      //
      //      if (.......) {
      //        if (autoTimer == (String(myRTC.hours) + ":" + String(myRTC.minutes))) {
      //          //Todo: Relay is LOW
      //          statusRL = 0;
      //          digitalWrite(relay1, statusRL);
      //        } else {
      //          //Todo: Relay is HIGH
      //          statusRL = 1;
      //          digitalWrite(relay1, statusRL);
      //        }
      //      } else {
      //        ..........
      //      }
    }

    //  if(RL=='1' && manualTimer==(String(myRTC.hours) + ":" + String(myRTC.minutes)));
    //  if (t == (String(myRTC.hours) + ":" + String(myRTC.minutes)) {
    //    digitalWrite(relay1, HIGH);
    //  }else if(t != (String(myRTC.hours) + ":" + String(myRTC.minutes)){
    //    digitalWrite(relay1, LOW);
    //    S1 == 1;
    //  //  }
    delay(1000);
    return datetime;
  }
}
String EEPROM_write(int index, String val) {
  EEPROM.put(index, val);
  EEPROM.commit();
}
String EEPROM_read(int index, int endid) {
  String Time;
  for (int i = index; i < endid; ++i)
  {
    Time += char(EEPROM.read(i));
  }
  return Time;
}

int InsertRLVal(int beginIndex, int startIndex, int endIndex, ObRelay putRL) {
  struct ObRelay getRL;
  EEPROM.get(startIndex, getRL);
  char idDev[3], pin[2];
  String(getRL.idDevice).toCharArray(idDev, 4);
  String(getRL.pin).toCharArray(pin, 3);
  if (String(idDev).equals("000") && String(pin).equals("00")) {
    EEPROM.put(startIndex, putRL);
    if (EEPROM.commit() == true) {
      Serial.println("Put index: " + String(startIndex));
      return startIndex;
    } else return 0;
  } else if (startIndex >= endIndex && !String(idDev).equals(" ") && !String(pin).equals("00")) {
    //Todo: If the EEPROM don't have empty stored between index 'beginIndex' - 'endIndex' is save the data to first index of pinD0 or index 'beginIndex';
    EEPROM.put(beginIndex, putRL);
    if (EEPROM.commit() == true) {
      Serial.println("Put index: " + String(beginIndex));
      return beginIndex;
    } else return 0;
  } else {
    return 0;
  }
}

int deleteRLVal(int index, char* idDevice, char* pins, char* startT, char* stWork, char* stUse) {
  ObRelay delRL;
  strcpy(delRL.idDevice, String("000").c_str());
  strcpy(delRL.pin, String("00").c_str());
  strcpy(delRL.startTimer, String("00000000").c_str());
  strcpy(delRL.statusWork, String("0").c_str());
  strcpy(delRL.statusUse, String("0").c_str());

  ObRelay getRL;
  EEPROM.get(index, getRL);
  char idDev[3], pin[2], timer[8], stW[1], stU[1];
  String(getRL.idDevice).toCharArray(idDev, 4);
  String(getRL.pin).toCharArray(pin, 3);
  String(getRL.startTimer).toCharArray(timer, 9);
  String(getRL.statusWork).toCharArray(stW, 1);
  String(getRL.statusUse).toCharArray(stU, 1);

  if (String(idDev).equals(idDevice) && String(pin).equals(pins) && String(timer).equals(startT) && String(stW).equals(stWork) && String(stU).equals(stUse)) {
    EEPROM.put(index, delRL);
    if (EEPROM.commit() == true) {
      Serial.println("Delete index: " + String(index));
      return index;
    } else return 0;
  } else {
    return 0;
  }
}
