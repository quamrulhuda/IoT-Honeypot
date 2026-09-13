#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN"; 
const char* CHAT_ID   = "YOUR_TELEGRAM_CHAT_ID";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiServer honeypotServer(23);
const int SINKHOLE_LED = LED_BUILTIN; 

void show_normal_text_screen(String deviceIP) {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("[ STATUS: WAITING ]");
  display.println("---------------------");
  
  display.setCursor(0, 18);
  display.print("Device IP: ");
  display.setCursor(0, 28);
  display.println(deviceIP);
  
  display.setCursor(0, 42);
  display.print("Attacker IP: ");
  display.setCursor(0, 52);
  display.println("NO ATTACK DETECTED");
  
  display.display(); 
}

void draw_robot_eyes_alert_screen(String attackerIP) {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 0);
  display.println("!! ATTACK DETECTED !!");
  display.println("---------------------");
  
  display.fillCircle(35, 34, 10, SSD1306_WHITE);
  display.fillCircle(35, 34, 3, SSD1306_BLACK);
  display.fillTriangle(15, 20, 55, 20, 55, 28, SSD1306_BLACK); 
  
  display.fillCircle(93, 34, 10, SSD1306_WHITE);
  display.fillCircle(93, 34, 3, SSD1306_BLACK);
  display.fillTriangle(113, 20, 73, 20, 73, 28, SSD1306_BLACK); 
  
  display.setCursor(0, 55);
  display.print("ATK IP: ");
  display.println(attackerIP);
  
  display.display();
}

void send_telegram_alert(String attackerIP) {
  X509List cert; 
  WiFiClientSecure client;
  
  client.setInsecure(); 
  client.setBufferSizes(512, 512); 
  
  Serial.print("\n🔒 Establishing HTTPS connection with Telegram server...");
  
  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("\n❌ Error! Secure handshake with Telegram failed.");
    return;
  }
  Serial.println(" Success!");
  
  String message = "⚠️ [HONEYPOT ALERT] Unauthorized network scan detected! Attacker IP: " + attackerIP;
  
  message.replace(" ", "%20"); 
  message.replace("[", "%5B");
  message.replace("]", "%5D");
  message.replace("!", "%21");

  String url = "/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message;
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.telegram.org\r\n" +
               "User-Agent: ESP8266-Honeypot\r\n" +
               "Connection: close\r\n\r\n");
               
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  Serial.println("📡 Alert successfully sent to Telegram cloud.");
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  pinMode(SINKHOLE_LED, OUTPUT);
  digitalWrite(SINKHOLE_LED, HIGH);

  Wire.begin(14, 12); 
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Wire.begin(12, 14); 
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      for(;;); 
    }
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Honeypot Booting...");
  display.display();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(SINKHOLE_LED, !digitalRead(SINKHOLE_LED));
  }
  
  digitalWrite(SINKHOLE_LED, HIGH); 
  honeypotServer.begin();
  
  show_normal_text_screen(WiFi.localIP().toString());
}

void loop() {
  WiFiClient attackerClient = honeypotServer.available();
  
  if (attackerClient) {
    String attackerIP = attackerClient.remoteIP().toString();
    
    digitalWrite(SINKHOLE_LED, LOW); 
    draw_robot_eyes_alert_screen(attackerIP);
    
    Serial.println("\n#################################################");
    Serial.println("⚠️  [SECURITY ALERT] MALICIOUS NETWORK SCAN DETECTED!");
    Serial.print("🚨 TARGET ATTACKER IP : ");
    Serial.println(attackerIP);
    Serial.println("#################################################");
    
    attackerClient.println("\n=============================================");
    attackerClient.println("       Welcome to Smart-Cam Linux Node v4.19  ");
    attackerClient.println("=============================================");
    attackerClient.print("camera-login: ");
    delay(100);
    
    send_telegram_alert(attackerIP);
    attackerClient.stop();
    
    delay(5000); 
    digitalWrite(SINKHOLE_LED, HIGH); 
    
    show_normal_text_screen(WiFi.localIP().toString());
    Serial.println("\n🔒 Session closed. Listening active for the next scan...\n");
  }
}
