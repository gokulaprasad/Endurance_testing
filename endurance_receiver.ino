#include <ESP8266WiFi.h>
#include <espnow.h>

// Structure example to receive data
typedef struct struct_message {
  int senderId;
  int noOfCycles;
  int cyclesPerMinute;
  int cycleCount;
  int remainingCycleCount;
} struct_message;

struct_message receivedData;

void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  Serial.print("Board ID: ");
  Serial.print(receivedData.senderId);
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("No of Cycles: ");
  Serial.println(receivedData.noOfCycles);
  Serial.print("Cycles Per Minute: ");
  Serial.println(receivedData.cyclesPerMinute);
  Serial.print("Cycle Count: ");
  Serial.println(receivedData.cycleCount);
  Serial.print("Remaining Cycle Count: ");
  Serial.println(receivedData.remainingCycleCount);
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the receive callback function
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  // Nothing to do here. All the work is done in the onDataRecv callback function.
}
