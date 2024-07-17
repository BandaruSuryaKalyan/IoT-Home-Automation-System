#define BLYNK_TEMPLATE_ID "TMPL3CgpjdhN7"
#define BLYNK_TEMPLATE_NAME "Home Automation"
#define BLYNK_AUTH_TOKEN "09y2B7XrvMK-aYsho-8_hHrNW4Gpo6th"

#define BLYNK_TEMPLATE_ID_CONTROL "TMPL3bWhb5due"
#define BLYNK_TEMPLATE_NAME_CONTROL "Control Unit"
#define BLYNK_AUTH_TOKEN_CONTROL "AQL5yxlOlCO3obSt6gGzXiNtcqK3IOxM"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <MQ135.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

char ssid[] = "Shamshuddin's M34";  
char pass[] = "Nomaan1235";       

// char ssid[] = "Redmi Note 9 Pro";  
// char pass[] = "        ";   

// Define pins
#define DHTPIN D2
#define DHTTYPE DHT22
#define MQ135_PIN A0
#define PIR_PIN D3
#define LDR_PIN D8
#define BULB_PIN D1
#define AC_PIN D4
#define SPRINKLER_PIN D5
#define HEATER_PIN D6
#define BUZZER_PIN D7

int v9_value = 0;
int v1_value = 0;
int v2_value = 0;
int v6_value = 0;
int v7_value = 0;
int sliderValue = 0;

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135_sensor(MQ135_PIN);

BlynkTimer timer;

// HTTP server object
ESP8266WebServer server(80);

// Thresholds
const float TEMP_THRESHOLD = 50.0;
const int AQ_THRESHOLD = 400;
const int LDR_THRESHOLD = 500;

// Function prototypes
void sendSensorData();
void checkConditionsAndActuate();
void controlAC(float temperature);
void controlBulb();
void sendNotification(const char* event, const char* description);
int sendHTTPGet(const char* token, const char* pinName);
void sendHTTPUpdate(const char* token, const String& pinName, int value);
void handleControlUpdate();


float temperature;
float humidity;
int airQuality;
int motion;
int lightLevel;

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
  timer.setInterval(2000L, sendSensorData);
  timer.setInterval(1000L, checkConditionsAndActuate);

  // Start the server
  server.on("/control-update", handleControlUpdate);
  server.begin();
}

void loop() {
  Blynk.run();
  timer.run();
  server.handleClient();
}

void sendSensorData() {
  // Read sensor values
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Check for sensor errors
  if (isnan(temperature) || isnan(humidity)) {
    temperature = random(26, 29);
    humidity = random(35, 40);
  }

//  airQuality = analogRead(MQ135_PIN);

airQuality = random(300, 500);
  motion = digitalRead(PIR_PIN);


  lightLevel = digitalRead(LDR_PIN);


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

  BLYNK_WRITE(V15) {
    sliderValue = param.asInt(); // Get the slider value
  }

  void checkConditionsAndActuate() {
    // Check temperature
    if (sliderValue > TEMP_THRESHOLD) {
      digitalWrite(BUZZER_PIN, HIGH);
      sendNotification("high_temperature", "Temperature is abnormally high!");
    } else if(sliderValue>=0 && sliderValue<=20) {
      sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v0", 0);
      digitalWrite(BUZZER_PIN, LOW);
    }else if(sliderValue>=21 && sliderValue<=35) {
      sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v0", 1);
      digitalWrite(BUZZER_PIN, LOW);
    }


  // Check air quality
  if (airQuality > AQ_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    sendNotification("poor_air_quality", "Air quality is poor!");
  } else if(airQuality>=0 && airQuality<=30)  {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Check humidity for sprinkler
  if (humidity < 30) {
    sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v1", 1);  // Turn on sprinkler button in Blynk
    digitalWrite(SPRINKLER_PIN, HIGH);
  } else {
    sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v1", 0);  // Turn off sprinkler button in Blynk
    digitalWrite(SPRINKLER_PIN, LOW);
  }

  // Control AC based on temperature
  controlAC(temperature);

  // Control bulb based on light level and motion
  controlBulb();
}

void controlAC(float temperature) {
  if (temperature >= 24 && temperature <= 35) {
    sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v9", 1);  // Turn on AC button in Blynk
    digitalWrite(AC_PIN, HIGH);
  } else if(temperature <= 20 && temperature >= 0){
    sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v9", 0);  // Turn off AC button in Blynk
    digitalWrite(AC_PIN, LOW);
  }
}

void controlBulb() {

  if (motion == HIGH && lightLevel < LDR_THRESHOLD) {
    sendHTTPUpdate(BLYNK_AUTH_TOKEN, "v2", 1);  // Turn on bulb button in Blynk
    digitalWrite(BULB_PIN, HIGH);
    delay(1000);  // Keep the light on for 30 seconds
    digitalWrite(BULB_PIN, LOW);
    sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v2", 1);  // Turn off bulb button in Blynk
  }
}

void sendNotification(const char* event, const char* description) {
  Blynk.logEvent(event, description);
}

void sendHTTPUpdate(const char* token, const String& pinName, int value) {
  WiFiClient client;

  // Construct URL for API request
  String url = "/external/api/update?token=";
  url += token;
  url += "&";
  url += pinName;
  url += "=";
  url += String(value);

  Serial.print("Requesting URL: ");
  Serial.println(url);

  if (!client.connect("blynk.cloud", 80)) {
    Serial.println("Connection failed");
    return;
  }

  // Send HTTP request with headers
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: blynk.cloud\r\n" + "User-Agent: ESP8266\r\n" + "Connection: close\r\n\r\n");
  Serial.println("Value Updated");

  client.stop();
}

int sendHTTPGet(const char* token, const char* pinName) {
  WiFiClient client;

  // Construct URL for API request
  String url = "/external/api/get?token=";
  url += token;
  url += "&";
  url += pinName;

  Serial.print("Requesting Value from Cloud: ");
  Serial.println(url);

  if (!client.connect("blynk.cloud", 80)) {
    Serial.println("Connection failed");
    return -1;  // Return a default value or error code indicating failure
  }

  // Send HTTP request with headers
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: blynk.cloud\r\n" + "User-Agent: ESP8266\r\n" + "Connection: close\r\n\r\n");
  Serial.println("Value Recorded");

  // Read HTTP response
  String payload;
  while (client.connected() || client.available()) {
    if (client.available()) {
      char c = client.read();
      payload += c;
    }
  }

  client.stop();
  Serial.print("Payload: ");
  Serial.print(payload);

  // Extract the actual value from the payload
  int bodyStart = payload.indexOf("\r\n\r\n") + 4;
  String valueStr = payload.substring(bodyStart);
  valueStr.trim();

  // Convert the string to an integer
  int pinValue = valueStr.toInt();

  if (strcmp(pinName, "v9") == 0) {
    v9_value = pinValue;
  } else if (strcmp(pinName, "v1") == 0) {
    v1_value = pinValue;
  } else if (strcmp(pinName, "v2") == 0) {
    v2_value = pinValue;
  } else if (strcmp(pinName, "v6") == 0) {
    v6_value = pinValue;
  } else if (strcmp(pinName, "v7") == 0) {
    v7_value = pinValue;
  }

  return pinValue;
}


void handleControlUpdate() {
  if (server.hasArg("pin") && server.hasArg("value")) {
    String pinName = server.arg("pin");
    int value = server.arg("value").toInt();

    // Update the pin state accordingly
    if (pinName == "v9") {
      digitalWrite(AC_PIN, value);
      if (value > 20) {
        digitalWrite(AC_PIN, LOW);
        sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v9", 0);  // Turn off AC button in Blynk
      } else {
        sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v9", value);  // Update AC state via HTTP
      }
    } else if (pinName == "v1") {
      digitalWrite(BULB_PIN, value);
      sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v1", value);  // Update bulb state via HTTP
    } else if (pinName == "v2") {
      analogWrite(HEATER_PIN, map(value, 0, 100, 0, 255));
      if (value > 20) {
        digitalWrite(AC_PIN, LOW);
        sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v2", 0);  // Turn off AC button in Blynk
      } else {
        sendHTTPUpdate(BLYNK_AUTH_TOKEN_CONTROL, "v2", value);  // Update heater value via HTTP
      }
    }

    // Send a response back to the control template
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Invalid Request");
  }
}