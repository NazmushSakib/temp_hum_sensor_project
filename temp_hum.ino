// For Oled Display with refined alert msg

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WebServer.h>


// const char* ssid = "ANS-2";  //Mogbazar
// const char* ssid = "TP-ANS-2.4";  //Ramna
const char* ssid = "ANS CTG 2.4G";  //CTG
// const char* ssid = "ANS KHL 2.4G"; //khulna
const char* password = "Password_01";

const char* botToken = "8029735356:AAFkjWeoNbfEsAS5dc8Xz-fdVcIOnsGLDDw";
const char* chatId = "-1002757593763";

// const char* siteName = "Moghbazar ANS-1";
// const char* siteName = "KHL-Khalishpur";
// const char* siteName = "KHL-Metro";
const char* siteName = "CTG-Nandankanon";
// const char* siteName = "DHK-Ramna";


/*
| Component      | NodeMCU Pin | GPIO Number | Notes         |
| -------------- | ----------- | ----------- | ------------- |
| **DHT22 Data** | **D4**      | **GPIO2**   | safer than D3 |
| **OLED SDA**   | **D2**      | **GPIO4**   | I²C data      |
| **OLED SCL**   | **D1**      | **GPIO5**   | I²C clock     |
Site name
wifi name
wifi password
*/

#define DHTPIN 2         // D4 = GPIO2
// #define DHTPIN 15         // D8 = GPIO15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 16x16 thermometer icon
const unsigned char PROGMEM iconTemp[] = {
  0b00000110, 0b00000000,
  0b00000110, 0b00000000,
  0b00000110, 0b00000000,
  0b00000110, 0b00000000,
  0b00000110, 0b00000000,
  0b00000110, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00001111, 0b00000000,
  0b00000110, 0b00000000,
  0b00000110, 0b00000000
};

// 16x16 humidity droplet
const unsigned char PROGMEM iconHum[] = {
  0b00000100, 0b00000000,
  0b00001110, 0b00000000,
  0b00011111, 0b00000000,
  0b00111111, 0b10000000,
  0b01111111, 0b11000000,
  0b11111111, 0b11100000,
  0b11111111, 0b11100000,
  0b11111111, 0b11100000,
  0b11111111, 0b11100000,
  0b01111111, 0b11000000,
  0b00111111, 0b10000000,
  0b00011111, 0b00000000,
  0b00001110, 0b00000000,
  0b00000100, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};

// 16x16 WiFi icon
const unsigned char PROGMEM iconWiFi[] = {
  0b00000000, 0b00000000,
  0b00011111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111000, 0b01111000,
  0b11100000, 0b00011100,
  0b11000011, 0b10001100,
  0b00000111, 0b11000000,
  0b00001100, 0b01100000,
  0b00011000, 0b00110000,
  0b00010000, 0b00010000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 6 * 3600, 60000);

ESP8266WebServer server(80);

unsigned long lastMessageTime = 0;
const unsigned long normalInterval = 3600000;  // 1 hour
const unsigned long alertInterval = 120000;    // 2 minutes
bool inAlertMode = false;
bool paused = false;


// for display page switching
unsigned long lastPageSwitch = 0;
bool showPage1 = true;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // OLED init
  if (!display.begin(0x3C, true)) {  // 0x3C is default I2C address
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");

  timeClient.begin();

  // Webserver endpoints
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
    Serial.println("DHT22 read failed");
    return;
  }

  timeClient.update();
  String timeStr = timeClient.getFormattedTime();
  String dateStr = getDateString();

  // ==== PAGE SWITCHING ====
  if (millis() - lastPageSwitch > (showPage1 ? 4000 : 2000)) {
    showPage1 = !showPage1;
    lastPageSwitch = millis();
  }

  
  display.clearDisplay();
  if (showPage1) {
    // === Page 1: Temp & Hum with icons ===
    display.drawBitmap(0, 0, iconTemp, 16, 16, SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(20, 0);
    display.print("T:");
    display.print(temp, 1);
    display.print((char)247);  // degree symbol
    display.print("C");

    display.drawBitmap(0, 25, iconHum, 16, 16, SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(20, 25);
    display.print("H:");
    display.print(hum, 0);
    display.print("%");

    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print(dateStr);
    display.print(" ");
    display.print(timeStr.substring(0, 5));

  } else {
    // === Page 2: WiFi + Site with icons ===
    display.drawBitmap(0, 0, iconWiFi, 16, 16, SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(20, 0);
    display.print("WiFi: ");
    display.print(WiFi.SSID());

    display.setCursor(20, 15);
    display.print("IP: ");
    display.print(WiFi.localIP());

    display.setCursor(20, 30);
    display.print("RSSI: ");
    display.print(WiFi.RSSI());
    display.println("dBm");

    display.setCursor(0, 50);
    display.print("Site: ");
    display.print(siteName);
  }
  display.display();


 // === ALERT LOGIC ===

bool tempHigh = temp > 29;
bool tempLow  = temp < 17;

bool humHigh  = hum > 85;
bool humLow   = hum < 39;

bool tempAlert = tempHigh || tempLow;
bool humAlert  = humHigh || humLow;

inAlertMode = tempAlert || humAlert;

// Build reason string
String reason = "";

if (tempHigh) {
  if (reason != "") reason += " and ";
  reason += "Temperature exceeds upper limit";
}

if (tempLow) {
  if (reason != "") reason += " and ";
  reason += "Temperature below lower limit";
}

if (humHigh) {
  if (reason != "") reason += " and ";
  reason += "Humidity exceeds upper limit";
}

if (humLow) {
  if (reason != "") reason += " and ";
  reason += "Humidity below lower limit";
}

unsigned long now = millis();
unsigned long interval = inAlertMode ? alertInterval : normalInterval;

if (now - lastMessageTime >= interval || lastMessageTime == 0) {

  String msg = "🌡 Temp: " + String(temp, 1) + "°C\n";
  msg += "💧 Hum: " + String(hum, 1) + "%\n";
  msg += "📅 " + dateStr + " 🕒 " + timeStr + "\n";
  msg += "📍 Site: " + String(siteName);

  if (inAlertMode) {
    msg = "🚨 ALERT!\nCause: " + reason + "\n\n" + msg;
  }

  sendTelegramMessage(msg);
  lastMessageTime = now;
}

  delay(500);
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
      http.end();
    }
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
