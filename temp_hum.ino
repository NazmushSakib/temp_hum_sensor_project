// v1 For LCD Display

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "ANS-2";
const char* password = "Password_01";

const char* botToken = "8029735356:AAFkjWeoNbfEsAS5dc8Xz-fdVcIOnsGLDDw";
const char* chatId = "-1002757593763";

// I2C LCD SDA D2 (GPIO4) I2C Data Line
//I2C LCD SCL D1 (GPIO5) I2C Clock Line

#define DHTPIN D3
// #define DHTTYPE DHT11
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 20, 4);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 6 * 3600, 60000);

ESP8266WebServer server(80);
unsigned long lastMessageTime = 0;
const unsigned long normalInterval = 3600000; // 1 hour  3600000
const unsigned long alertInterval = 120000;   // 2 minutes   120000
bool inAlertMode = false;
bool paused = false;

const char* siteName = "Moghbazar ANS-2";
// const char* siteName = "Ramna";

void setup() {
  Serial.begin(9600);   //115200
  dht.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected!");
  delay(1000);
  lcd.clear();
  lcd.print("IP:");  
  lcd.print(WiFi.localIP());
  delay(1000);
  lcd.clear();

  

  timeClient.begin();

  server.on("/wake", HTTP_GET, []() {
    paused = false;
    server.send(200, "text/plain", "Resumed sending.");
  });

  server.on("/status", HTTP_GET, []() {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    String dateStr = getDateString();
    String timeStr = timeClient.getFormattedTime();
    String msg = "🌡 Temp: " + String(temp, 1) + "°C\n💧 Hum: " + String(hum, 1) + "%\n📅 " + dateStr + " 🕒 " + timeStr + "\n📍 Site: " + String(siteName);
    server.send(200, "text/plain", msg);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  if (paused) return;

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp) || isnan(hum)) {
    // Serial.println("DHT11 read failed");
    Serial.println("DHT22 read failed");
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error    ");
    return;
  }
  // ===Testing===
  Serial.println(temp);
  Serial.println(hum);
// ===Testing===
  timeClient.update();
  String timeStr = timeClient.getFormattedTime();
  String dateStr = getDateString();

  lcd.setCursor(0, 0);
  lcd.print(" T:");
  lcd.print(temp, 1);
  lcd.print((char)223);
  lcd.print("C H:");
  lcd.print(hum, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print(dateStr + " " + timeStr.substring(0, 5));

  lcd.setCursor(0, 2);
  lcd.print("WiFi:OK ");
  lcd.print(WiFi.RSSI());
  lcd.print("dBm");

  lcd.setCursor(0, 3);
  lcd.print("Site:");
  lcd.print(siteName);

  bool tempAlert = (temp > 29 || temp < 15);
  bool humAlert = (hum > 80 || hum < 43);
  inAlertMode = tempAlert || humAlert;

  unsigned long now = millis();
  unsigned long interval = inAlertMode ? alertInterval : normalInterval;

  if (now - lastMessageTime >= interval || lastMessageTime == 0) {
    String msg = "🌡 Temp: " + String(temp, 1) + "°C\n💧 Hum: " + String(hum, 1) + "%\n📅 " + dateStr + " 🕒 " + timeStr + "\n📍 Site: " + String(siteName);
    if (inAlertMode) msg = "🚨 ALERT!\n" + msg;
    sendTelegramMessage(msg);
    lastMessageTime = now;
  }

  delay(5000);
}

void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + String(botToken) + "/sendMessage?chat_id=" + String(chatId) + "&text=" + urlencode(message);
    if (http.begin(client, url)) {
      int httpCode = http.GET();
      Serial.print("Telegram status: ");
      Serial.println(httpCode);
      if (httpCode > 0) Serial.println(http.getString());
      else Serial.println("Telegram send failed.");
      http.end();
    } else {
      Serial.println("HTTP begin failed.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}

String getDateString() {
  time_t raw = timeClient.getEpochTime();
  struct tm* ti = localtime(&raw);
  char buf[20];
  strftime(buf, sizeof(buf), "%d-%b-%Y", ti);
  return String(buf);
}

String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += '%';
      encoded += char(code0 < 10 ? code0 + '0' : code0 - 10 + 'A');
      encoded += char(code1 < 10 ? code1 + '0' : code1 - 10 + 'A');
    }
  }
  return encoded;
}
