#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#define LED_GREEN 27
#define LED_YELLOW 14
#define LED_RED 12
#define BUZZER 25
#define MQ2_ANALOG_PIN 34
#define FLAME_DIGITAL_PIN 35
#define BUTTON_PIN 4
const int relay = 33;
bool relayState = LOW;
const int r1 = 16;  // Chân đầu vào GPIO cho nút nhấn
int r1State = 0;
LiquidCrystal_I2C lcd(0x3f, 16, 2);
#define chatId "1104099590"
#define TELEGRAM_BOT_TOKEN "6140390959:AAERfDSE7x_wVSpNNiGdUcK9mz-6yhzbBw0"

// Create a WiFiClientSecure object
WiFiClientSecure client;

// Initialize the UniversalTelegramBot object with the WiFiClientSecure object
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);
int botRequestDelay = 200;
unsigned long lastTimeBotRan;
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i = 0; i < numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/ON để bật thiết bị chữa cháy \n";
      welcome += "/OFF để tắt thiết bị chữa cháy \n";
      welcome += "/state để kiểm tra trạng thái thiết bị chữa cháy \n";
      welcome += "/Gas để hiển thị nồng độ khí gas hiện tại \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/ON") {
      bot.sendMessage(chat_id, "Thiết bị chữa cháy đã được bật", "");
      relayState = HIGH;
      digitalWrite(relay, relayState);
    }

    if (text == "/OFF") {
      bot.sendMessage(chat_id, "Thiết bị chữa cháy đã được tắt", "");
      relayState = LOW;
      digitalWrite(relay, relayState);
    }

    if (text == "/state") {
      if (digitalRead(relay)) {
        bot.sendMessage(chat_id, "Thiết bị chữa cháy đang được bật", "");
      } else {
        bot.sendMessage(chat_id, "Thiết bị chữa cháy đang được tắt", "");
      }
    }
    if (text == "/Gas") {
      float gasValue = analogRead(MQ2_ANALOG_PIN);                                      // đọc giá trị nồng độ khí gas
      bot.sendMessage(chat_id, "Nồng độ khí gas hiện tại là " + String(gasValue), "");  // gửi tin nhắn với nồng độ khí gas hiện tại
    }
  }
}




void setup() {
  Serial.begin(115200);
#ifdef ESP8266
  client.setInsecure();
#endif
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(r1, INPUT);  // Thiết lập chân đầu vào cho nút nhấn
  pinMode(relay, OUTPUT);
  pinMode(FLAME_DIGITAL_PIN, INPUT);
  pinMode(MQ2_ANALOG_PIN, INPUT);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("disconnect");

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");

  lcd.clear();
  lcd.print("connected");
  digitalWrite(LED_GREEN, LOW);
  bot.sendMessage(chatId, "connected \n");
  bot.sendMessage(chatId, "click /start để bắt đầu");
  delay(3000);
  digitalWrite(LED_GREEN, HIGH);
}

void loop() {

  r1State = digitalRead(r1);

  // Nếu nút nhấn được nhấn, đảo trạng thái của đèn và gửi thông báo về Telegram
  if (r1State == HIGH) {
    relayState = !relayState;
    digitalWrite(relay, relayState);
    bot.sendMessage(chatId, "Trạng thái của thiết bị đã chuyển: " + String(relayState), "");
    delay(500);  // Chờ 500ms tránh nhận nhiều tín hiệu từ nút nhấn cùng lúc
  }

  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();

    int gasLevel = analogRead(MQ2_ANALOG_PIN);
    int flameDetected = digitalRead(FLAME_DIGITAL_PIN);

    lcd.setCursor(0, 1);
    lcd.print("Gas: ");
    lcd.print(gasLevel);
    lcd.print("   ");

    lcd.setCursor(0, 0);
    lcd.print("Flame: ");
    lcd.print(flameDetected);
    lcd.print("   ");

    String message = "";
          digitalWrite(LED_GREEN, HIGH);
    if (gasLevel > 800) {
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_GREEN, LOW);
      message = "Gas level is high!";
    } else {
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, HIGH);
    }

    if (flameDetected == LOW) {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
      message = "Flame detected!";
    } else {
      digitalWrite(LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
      digitalWrite(LED_GREEN, HIGH);
    }

    if (!message.equals("")) {
      bot.sendMessage(chatId, message);
    }

    if (digitalRead(BUTTON_PIN) == HIGH) {
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      lcd.clear();
      lcd.print("disconnect");
      delay(1000);
      ESP.restart();
    }
  }
}
