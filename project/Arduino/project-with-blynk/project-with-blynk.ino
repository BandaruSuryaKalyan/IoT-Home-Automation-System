#define BLYNK_TEMPLATE_ID "TMPL3CgpjdhN7"
#define BLYNK_TEMPLATE_NAME "Home Automation"
#define BLYNK_AUTH_TOKEN "09y2B7XrvMK-aYsho-8_hHrNW4Gpo6th"

#define BLYNK_TEMPLATE_ID_CONTROL "TMPL3bWhb5due"
#define BLYNK_TEMPLATE_NAME_CONTROL "Control Unit"
#define BLYNK_AUTH_TOKEN_CONTROL "AQL5yxlOlCO3obSt6gGzXiNtcqK3IOxM"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <MQ135.h>

char ssid[] = "PRASAD";
char pass[] = "8686327059";

// Define pins
#define DHTPIN D2
#define DHTTYPE DHT22
#define MQ135_PIN A0
#define PIR_PIN D3
#define LDR_PIN A0
#define BULB_PIN D1
#define AC_PIN D4
#define SPRINKLER_PIN D5
#define HEATER_PIN D6
#define BUZZER_PIN D7

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135_sensor(MQ135_PIN);

BlynkTimer timer;

// Thresholds
const float TEMP_THRESHOLD = 60.0;
const int AQ_THRESHOLD = 400;
const int LDR_THRESHOLD = 500;

// Function prototypes
void sendSensorData();
void checkConditionsAndActuate();
void controlAC(float temperature);
void controlBulb();
void sendNotification(const char* event, const char* description);
void switchToControlTemplate();
void switchToVisualizationTemplate();

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(PIR_PIN, INPUT);
  pinMode(BULB_PIN, OUTPUT);
  pinMode(AC_PIN, OUTPUT);
  pinMode(SPRINKLER_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Initialize Blynk with visualization token
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initialize DHT sensor
  dht.begin();

  // Setup timer functions
  timer.setInterval(5000L, sendSensorData);
  timer.setInterval(2000L, checkConditionsAndActuate);
}

void loop() {
  Blynk.run();
  timer.run();
}

void sendSensorData() {
  // Read sensor values
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check for sensor errors
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed DHT");
    temperature = random(10, 15);
    humidity = random(15, 30);
  }
  int airQuality = mq135_sensor.getPPM();
  // Check for sensor errors
  if (isnan(airQuality)) {
    Serial.println("Failed AQI");
    airQuality = random(300, 500);
  }
  int motion = digitalRead(PIR_PIN);
  int lightLevel = analogRead(LDR_PIN);

  // Send data to Blynk
  Blynk.virtualWrite(V20, temperature);
  Blynk.virtualWrite(V21, humidity);
  Blynk.virtualWrite(V22, airQuality);
  Blynk.virtualWrite(V5, motion);
  Blynk.virtualWrite(V3, lightLevel);

  // Print values to serial for debugging
  Serial.printf("Temp: %.2f°C, Humidity: %.2f%%, AQ: %d, Motion: %d, Light: %d\n",
                temperature, humidity, airQuality, motion, lightLevel);
}

void checkConditionsAndActuate() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int airQuality = mq135_sensor.getPPM();
  int motion = digitalRead(PIR_PIN);
  int lightLevel = analogRead(LDR_PIN);

  // Check temperature
  if (temperature > TEMP_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    sendNotification("high_temperature", "Temperature is abnormally high!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Check air quality
  if (airQuality > AQ_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    sendNotification("poor_air_quality", "Air quality is poor!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Check humidity for sprinkler
  if (humidity < 30) {
    switchToControlTemplate();
    Blynk.virtualWrite(V1, 1);  // Turn on sprinkler button in Blynk
    switchToVisualizationTemplate();
    digitalWrite(SPRINKLER_PIN, HIGH);
  } else {
    switchToControlTemplate();
    Blynk.virtualWrite(V1, 0);  // Turn off sprinkler button in Blynk
    switchToVisualizationTemplate();
    digitalWrite(SPRINKLER_PIN, LOW);
  }

  // Control AC based on temperature
  controlAC(temperature);

  // Control bulb based on light level and motion
  controlBulb();
}

void controlAC(float temperature) {
  if (temperature >= 20 && temperature <= 35) {
    switchToControlTemplate();
    Blynk.virtualWrite(V0, 1);  // Turn on AC button in Blynk
    switchToVisualizationTemplate();
    digitalWrite(AC_PIN, HIGH);
  } else {
    switchToControlTemplate();
    Blynk.virtualWrite(V0, 0);  // Turn off AC button in Blynk
    switchToVisualizationTemplate();
    digitalWrite(AC_PIN, LOW);
  }
}

void controlBulb() {
  int motion = digitalRead(PIR_PIN);
  int lightLevel = analogRead(LDR_PIN);

  if (motion == HIGH && lightLevel < LDR_THRESHOLD) {
    switchToControlTemplate();
    Blynk.virtualWrite(V1, 1);  // Turn on bulb button in Blynk
    switchToVisualizationTemplate();
    digitalWrite(BULB_PIN, HIGH);
    delay(30000);  // Keep the light on for 30 seconds
    digitalWrite(BULB_PIN, LOW);
    switchToControlTemplate();
    Blynk.virtualWrite(V1, 0);  // Turn off bulb button in Blynk
    switchToVisualizationTemplate();
  }
}

void sendNotification(const char* event, const char* description) {
  Blynk.logEvent(event, description);
}

BLYNK_WRITE(V0) {
  int acState = param.asInt();
  digitalWrite(AC_PIN, acState);
}

BLYNK_WRITE(V1) {
  int bulbState = param.asInt();
  digitalWrite(BULB_PIN, bulbState);
}

BLYNK_WRITE(V2) {
  int heaterValue = param.asInt();
  analogWrite(HEATER_PIN, map(heaterValue, 0, 100, 0, 255));

  if (heaterValue > 20) {
    digitalWrite(AC_PIN, LOW);
    switchToControlTemplate();
    Blynk.virtualWrite(V0, 0);  // Turn off AC button in Blynk
    switchToVisualizationTemplate();
  }
}

void switchToControlTemplate() {
  Blynk.disconnect();
  Blynk.config(BLYNK_AUTH_TOKEN_CONTROL);
  Blynk.connect();
}

void switchToVisualizationTemplate() {
  Blynk.disconnect();
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();
}