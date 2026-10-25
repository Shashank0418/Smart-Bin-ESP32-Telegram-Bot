#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Servo.h> // 🔥 Added Servo Library

// ===== WiFi =====
const char* ssid = "SmartBIN";
const char* password = "12345678";

// ===== Telegram =====
#define BOT_TOKEN "***********"
#define CHAT_ID "**********"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ===== Ultrasonic Pins =====
#define TRIG_PIN 23
#define ECHO_PIN 22
int obstacleDistance = 20;

// ===== Servo =====
#define SERVO_PIN 13
Servo binServo;

// ===== Motor Pins =====
#define IN1_LEFT 19
#define IN2_LEFT 18
#define ENA 21

#define IN1_RIGHT 5
#define IN2_RIGHT 4
#define ENB 2

int baseSpeed = 70;

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IN1_LEFT, OUTPUT);
  pinMode(IN2_LEFT, OUTPUT);
  pinMode(IN1_RIGHT, OUTPUT);
  pinMode(IN2_RIGHT, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // 🔥 Servo Setup
  binServo.attach(SERVO_PIN);
  binServo.write(0); // Start closed

  stopMotors();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  client.setInsecure();
}

// ================= LOOP =================
void loop() {

  int newMessages = bot.getUpdates(bot.last_message_received + 1);

  for (int i = 0; i < newMessages; i++) {
    String text = bot.messages[i].text;
    String chat_id = String(bot.messages[i].chat_id);

    if (chat_id == CHAT_ID) {
      handleCommand(text);
    }
  }

  delay(500);
}

// ================= ULTRASONIC =================
long getDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
 
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0) return 400;

  return duration * 0.034 / 2;
}

// ================= OBSTACLE =================
void checkObstacle() {

  long distance = getDistance();

  if (distance > 0 && distance < obstacleDistance) {

    Serial.println("Obstacle Detected!");
    stopMotors();

    bot.sendMessage(CHAT_ID, "Obstacle detected 🚧", "");

    // Wait until obstacle clears
    while (getDistance() < obstacleDistance) {
      delay(200);
    }

    bot.sendMessage(CHAT_ID, "Path cleared ✅", "");
  }
}

// ================= MOTOR FUNCTIONS =================

void moveForward(int duration_ms) {

  unsigned long startTime = millis();

  while (millis() - startTime < duration_ms) {

    checkObstacle();

    digitalWrite(IN1_LEFT, LOW);
    digitalWrite(IN2_LEFT, HIGH);

    digitalWrite(IN1_RIGHT, HIGH);
    digitalWrite(IN2_RIGHT, LOW);

    analogWrite(ENA, baseSpeed);
    analogWrite(ENB, baseSpeed);
  }

  stopMotors();
}

void moveBackward(int duration_ms) {

  unsigned long startTime = millis();

  while (millis() - startTime < duration_ms) {

    checkObstacle();

    digitalWrite(IN1_LEFT, HIGH);
    digitalWrite(IN2_LEFT, LOW);

    digitalWrite(IN1_RIGHT, LOW);
    digitalWrite(IN2_RIGHT, HIGH);

    analogWrite(ENA, baseSpeed);
    analogWrite(ENB, baseSpeed);
  }

  stopMotors();
}

void turnLeft(int duration_ms) {

  unsigned long startTime = millis();

  while (millis() - startTime < duration_ms) {

    checkObstacle();

    digitalWrite(IN1_LEFT, LOW);
    digitalWrite(IN2_LEFT, LOW);

    digitalWrite(IN1_RIGHT, HIGH);
    digitalWrite(IN2_RIGHT, LOW);

    analogWrite(ENA, 0);
    analogWrite(ENB, baseSpeed);
  }

  stopMotors();
}

void turnRight(int duration_ms) {

  unsigned long startTime = millis();

  while (millis() - startTime < duration_ms) {

    checkObstacle();

    digitalWrite(IN1_LEFT, LOW);
    digitalWrite(IN2_LEFT, HIGH);

    digitalWrite(IN1_RIGHT, LOW);
    digitalWrite(IN2_RIGHT, LOW);

    analogWrite(ENA, baseSpeed);
    analogWrite(ENB, 0);
  }

  stopMotors();
}

void stopMotors() {

  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1_LEFT, LOW);
  digitalWrite(IN2_LEFT, LOW);
  digitalWrite(IN1_RIGHT, LOW);
  digitalWrite(IN2_RIGHT, LOW);
}

// ================= TELEGRAM COMMANDS =================

void handleCommand(String cmd) {

  cmd.toLowerCase();

  if (cmd == "come to kitchen") {

    bot.sendMessage(CHAT_ID, "Coming to kitchen 🚗", "");

    moveForward(3000);
    turnLeft(1000);
    moveForward(3000);
    turnRight(1000);
    moveForward(1500);

    // Open lid
    binServo.write(90);
    delay(2000);

    // Close lid
    binServo.write(0);
  }

  else if (cmd == "go back") {

    bot.sendMessage(CHAT_ID, "Returning to start 🏠", "");

    // 🔄 Reverse path
    moveBackward(1500);
    turnLeft(1000);
    moveBackward(3000);
    turnRight(1000);
    moveBackward(3000);
  }

  else if (cmd == "stop") {

    stopMotors();
    bot.sendMessage(CHAT_ID, "Stopped 🛑", "");
  }
}


