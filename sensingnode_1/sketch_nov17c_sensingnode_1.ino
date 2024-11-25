#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>
#include <esp_sleep.h>

#define DHTPIN 4  // Data pin connected to DHT11
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Structure for sending ESP-Now data
typedef struct {
  float temperature;
  float humidity;
  bool alert;  // To indicate if the message is an alert
} DataPacket;

DataPacket dataPacket;

// Broadcast address for ESP-Now
uint8_t broadcastAddress[] = {0x08, 0xB6, 0x1F, 0x28, 0x77, 0xF0};

// User-configurable settings
const unsigned long communicationInterval = 3500;  // Communicate every 10 seconds
float tempThresholdHigh = 30.0;  // High-temperature threshold
float tempThresholdLow = 20.0;   // Low-temperature threshold

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);  // Station mode required for ESP-Now
  WiFi.disconnect();    // Disconnect WiFi for ESP-Now use

  // Initialize ESP-Now
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-Now initialization failed");
    return;
  }

  // Add broadcast peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  // Channel 0 for broadcast
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }

  Serial.println("ESP-Now Slave Ready");

  // Initialize DHT sensor
  dht.begin();
}

void loop() {
  // Wait a little before reading sensor data
  delay(500);  // Wait for the sensor to stabilize

  // Retry mechanism for reading sensor data
  const int maxRetries = 5;  // Max retries before failing
  int retries = 0;
  
  while (retries < maxRetries) {
    dataPacket.temperature = dht.readTemperature();
    dataPacket.humidity = dht.readHumidity();
    
    // Check if the readings are valid
    if (!isnan(dataPacket.temperature) && !isnan(dataPacket.humidity)) {
      // Successfully read data
      break;
    }
    
    retries++;
    Serial.println("Retrying to read from DHT11...");
    delay(1000);  // Wait a little before retrying
  }

  if (retries == maxRetries) {
    Serial.println("Failed to read from DHT sensor!");
    enterDeepSleep();
    return;
  }

  // Check for alerts (temperature threshold)
    // Default: no alert
  if (dataPacket.temperature > tempThresholdHigh || dataPacket.temperature < tempThresholdLow) {
    dataPacket.alert = true;
  } else {
    dataPacket.alert = false;
  }

  // Send data via ESP-Now
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&dataPacket, sizeof(dataPacket));

  if (result == ESP_OK) {
    if (dataPacket.alert) {
      Serial.printf("ALERT sent: Temp = %.1f°C\n", dataPacket.temperature);
    } else {
      Serial.printf("Data sent from sensing node 1: Temp = %.1f°C, Humidity = %.1f%%\n", 
                    dataPacket.temperature, dataPacket.humidity);
    }
  } else {
    Serial.println("Failed to send data");
  }

  // Enter deep sleep
  enterDeepSleep();
}

void enterDeepSleep() {
  // Wake up after communication interval
  esp_sleep_enable_timer_wakeup(communicationInterval * 2000);  // Convert ms to µs
  Serial.println("Entering deep sleep...");
  esp_deep_sleep_start();
}
