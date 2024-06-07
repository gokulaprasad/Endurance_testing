/*
   @Project: Endurance Lab
   @Version: V3.2
   @Author: Gokulaprasad
   @Date: 25-04-2024
*/

/********************** Library Files **********************/
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <Wire.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <SoftwareSerial.h>

#define SENDER_ID 1

/********************** Macro Definition **********************/
#define I2C_Addr            0x20 // I2C Address of PCF8574-board: 0x20 - 0x27
#define RELAY_MODULE        D4
#define LED_Red             D5
#define LED_Yellow          D6
#define LED_Green           D7
#define BUZZER              D0
#define MILLIS_PER_MINUTE   60000


#define SPIFFS_TOTAL_CYCLE_COUNT      "/totalCycleCount.txt"
#define SPIFFS_REMAINING_CYCLE_COUNT  "/remainingCycleCount.txt"
#define SPIFFS_CPM                    "/cpm.txt"
#define SPIFFS_IS_FRESH_START         "/isFreshStart.txt"

//Pins initialization to connect with raspberry PI

SoftwareSerial mySerial(D2, D3);  // RX TX

/********************** Local variables Declaration **********************/
unsigned long timeNow;
unsigned long currentTime;

String cyclesPerMinute_str;
String noOfCycles_str;
int noOfCycles;
int cyclesPerMinute;

const byte rows = 4; // Number of Rows
const byte columns = 4; // Number of Columns

unsigned long singleCycleTime;
int remainingCycleCount;
boolean startFlag = false;
boolean processStartFlag = false;

int isFreshStart = 1;
int cycle = 0;
int cycleCount = 0;
File file;

/********************** LCD Configuration **********************/
LiquidCrystal_I2C lcd(0x3F, 20, 4);

/********************** Keypad Configuration **********************/
//Layout of the Keys on Keypad
char KeyPadLayout[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

//Wiring of PCF8574-IO-Pins
byte PinsLines[rows] = {3, 2, 1, 0}; //  ROWS Pins
byte PinsColumns[columns] = {7, 6, 5, 4}; //  COLUMNS Pins

//Initialise KeyPad
Keypad_I2C i2cKeypad(makeKeymap(KeyPadLayout), PinsLines, PinsColumns, rows, columns, I2C_Addr);

/********************** ESP-NOW Configuration **********************/
// MAC address of the receiver ESP8266
uint8_t receiverMACAddress[] = {0xc8, 0xc9, 0xa3, 0x06, 0x4d, 0xee}; // Update with your receiver's MAC address

typedef struct struct_message {
  int senderId; // Add senderId to the structure
  int noOfCycles;
  int cyclesPerMinute;
  int cycleCount;
  int remainingCycleCount;
  bool processEnded;
} struct_message;


//{“Sender_ID”:XX,”No_of_cycles”:XX,”Cycles_per_minute”:XX,”Cycle_Count”:XX,”Remaining_Cycle_Count”:XX}

struct_message myData;

void sendData() {
  // Send data via ESP-NOW
  esp_now_send(receiverMACAddress, (uint8_t *) &myData, sizeof(myData));
}

/********************** Setup Configuration **********************/
void setup() {
  
  Serial.begin(115200);
  mySerial.begin(9600);

  pinMode(RELAY_MODULE, OUTPUT);
  pinMode(LED_Red, OUTPUT);
  pinMode(LED_Yellow, OUTPUT);
  pinMode(LED_Green, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(RELAY_MODULE, HIGH);
  digitalWrite(LED_Red, HIGH);

  lcd.init();
  lcd.begin(20, 4);
  lcd.backlight();

  Serial.println(F("--- Begin: Check Connection ..."));
  Wire.begin(4, 5); // Init I2C-Bus, GPIO4-Data, GPIO5 Clock
  Wire.beginTransmission(I2C_Addr); // Try to establish connection
  if (Wire.endTransmission() != 0) // No Connection established
    Serial.print("    NO ");
  else
    Serial.print("    ");

  Serial.print(F("Device found on"));
  Serial.print(F(" (0x"));
  Serial.print(I2C_Addr, HEX);
  Serial.println(F(")."));
  Serial.println(F("--- End: Check Connection"));

  i2cKeypad.begin(); // Start i2cKeypad

  isFreshStart = 1;

  if (!SPIFFS.begin()) {
    Serial.println(F("> Failed to mount file system..."));
    return;
  }

  if (SPIFFS.exists(SPIFFS_IS_FRESH_START)) {
    Serial.println(F("> File \"isFreshStart\" found..."));
    file = SPIFFS.open(SPIFFS_IS_FRESH_START, "r");
    isFreshStart = file.parseInt();
    file.close();
  }

  if (isFreshStart) {
    Serial.println(F("> \"isFreshStart\" is true - Starting from base..."));
    welcomeMsg();

    noOfCycles = getTotalCyclesFromKeypad().toInt();
    cyclesPerMinute = getCyclesPerMinuteFromKeypad().toInt();

    startProcessMsg();
  } else {
    Serial.println(F("> \"isFreshStart\" is false - Resuming from last Stop position..."));
    checkStoredData();
  }

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println(F("> Error initializing ESP-NOW"));
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverMACAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

/********************** Main Loop - Business Logic **********************/


void loop() {
  
  singleCycleTime = MILLIS_PER_MINUTE / cyclesPerMinute;

  startFlag = true; // Will get these value from push button.
  if (startFlag == true) {
    currentTime = millis();
  }

  if (isFreshStart) {
    cycleCount = 0;
    cycle = 1;
  } else {
    cycleCount = noOfCycles - remainingCycleCount;
  }

  while (cycleCount < noOfCycles) {
    if (millis() >= (timeNow + singleCycleTime)) {
      timeNow += singleCycleTime;

      digitalWrite(RELAY_MODULE, LOW);
      digitalWrite(LED_Yellow, HIGH);
      delay(2000); // Relay Active Time

      cycleCount++;
      remainingCycleCount = noOfCycles - cycleCount;
      isFreshStart = (remainingCycleCount == 0) ? 1 : 0;

      storeValuesInSPFFS();
      myData.senderId = SENDER_ID;
      myData.noOfCycles = noOfCycles;
      myData.cyclesPerMinute = cyclesPerMinute;
      myData.cycleCount = cycleCount;
      myData.remainingCycleCount = remainingCycleCount;
      sendData();
    }
    cycle++;

    digitalWrite(RELAY_MODULE, HIGH);
    digitalWrite(LED_Yellow, LOW);
    delay(singleCycleTime);

    serialLog();
    lcdLog();
  }

  isFreshStart = 1;
  // Storing isFreshStart into SPFFS
  file = SPIFFS.open(SPIFFS_IS_FRESH_START, "w");
  if (file) {
    file.println(isFreshStart);
    file.close();
  } else {
    Serial.println(F("> Failed to open \"isFreshStart\" file for writing..."));
  }

  endMsg();

  // Add an infinite loop to stop further execution
  while (true) {
    // Optionally, you can put the ESP8266 in deep sleep mode to save power
    // ESP.deepSleep(0); // Deep sleep mode indefinitely
  }
}


String getTotalCyclesFromKeypad() {
  lcd.clear();
  int counter = 0;

  lcd.setCursor(0, 1); // Column, Row
  lcd.print("Enter Total Cycles:");
  lcd.setCursor(0, 2); // Column, Row
  lcd.print("Press * after entry");

  char keyRead;
  while (1) {
    keyRead = i2cKeypad.getKey();
    Serial.print("Entered Key : ");
    Serial.println(keyRead);

    if (keyRead == '1' || keyRead == '2' || keyRead == '3' || keyRead == '4' || keyRead == '5' || keyRead == '6' || keyRead == '7' || keyRead == '8' || keyRead == '9' || keyRead == '0') {
      lcd.setCursor(counter, 0);
      lcd.print(keyRead);
      counter++;
      noOfCycles_str.concat(keyRead);

      Serial.println("NO OF CYLCES:");
      Serial.println(noOfCycles_str);
      Serial.println("---------------");
    } else if (keyRead == '*') {
      break;
    }
  }

  delay(300);
  lcd.clear();

  return noOfCycles_str;
}

String getCyclesPerMinuteFromKeypad() {
  lcd.clear();
  int counter = 0;

  lcd.setCursor(0, 1); // Column, Row
  lcd.print("Enter CPM:");
  lcd.setCursor(0, 2); // Column, Row
  lcd.print("Press * after entry");

  char keyRead;
  while (1) {
    keyRead = i2cKeypad.getKey();
    Serial.print("Entered Key : ");
    Serial.println(keyRead);

    if (keyRead == '1' || keyRead == '2' || keyRead == '3' || keyRead == '4' || keyRead == '5' || keyRead == '6' || keyRead == '7' || keyRead == '8' || keyRead == '9' || keyRead == '0') {
      lcd.setCursor(counter, 0);
      lcd.print(keyRead);
      counter++;
      cyclesPerMinute_str.concat(keyRead);

      Serial.println("CPM:");
      Serial.println(cyclesPerMinute_str);
      Serial.println("---------------");
    } else if (keyRead == '*') {
      break;
    }
  }

  delay(300);
  lcd.clear();

  return cyclesPerMinute_str;
}

void welcomeMsg() {
  lcd.setCursor(0, 0); // Column, Row
  lcd.print("Welcome to");
  lcd.setCursor(0, 1); // Column, Row
  lcd.print("Test Machine");

  Serial.println("---------------");
  Serial.println("Welcome to Test Machine");
  Serial.println("---------------");
}

void startProcessMsg() {
  lcd.setCursor(0, 0); // Column, Row
  lcd.print("Starting Process...");

  Serial.println("Starting Process...");
}

void endMsg() {
  lcd.setCursor(0, 0); // Column, Row
  lcd.print("Process Complete.");
  digitalWrite(LED_Red, LOW);
  digitalWrite(LED_Green, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(5000);
  digitalWrite(BUZZER, LOW);
}

void checkStoredData() {
  // Total Cycles
  if (SPIFFS.exists(SPIFFS_TOTAL_CYCLE_COUNT)) {
    Serial.println(F("> File \"totalCycleCount\" found..."));
    file = SPIFFS.open(SPIFFS_TOTAL_CYCLE_COUNT, "r");
    noOfCycles = file.parseInt();
    file.close();
  }

  // CPM
  if (SPIFFS.exists(SPIFFS_CPM)) {
    Serial.println(F("> File \"cpm\" found..."));
    file = SPIFFS.open(SPIFFS_CPM, "r");
    cyclesPerMinute = file.parseInt();
    file.close();
  }

  // Remaining Cycles
  if (SPIFFS.exists(SPIFFS_REMAINING_CYCLE_COUNT)) {
    Serial.println(F("> File \"remainingCycleCount\" found..."));
    file = SPIFFS.open(SPIFFS_REMAINING_CYCLE_COUNT, "r");
    remainingCycleCount = file.parseInt();
    file.close();
  }
}

void storeValuesInSPFFS() {
  // Storing Total Cycles into SPFFS
  file = SPIFFS.open(SPIFFS_TOTAL_CYCLE_COUNT, "w");
  if (file) {
    file.println(noOfCycles);
    file.close();
  } else {
    Serial.println(F("> Failed to open \"totalCycleCount\" file for writing..."));
  }

  // Storing CPM into SPFFS
  file = SPIFFS.open(SPIFFS_CPM, "w");
  if (file) {
    file.println(cyclesPerMinute);
    file.close();
  } else {
    Serial.println(F("> Failed to open \"cpm\" file for writing..."));
  }

  // Storing Remaining Cycles into SPFFS
  file = SPIFFS.open(SPIFFS_REMAINING_CYCLE_COUNT, "w");
  if (file) {
    file.println(remainingCycleCount);
    file.close();
  } else {
    Serial.println(F("> Failed to open \"remainingCycleCount\" file for writing..."));
  }

  // Storing isFreshStart into SPFFS
  file = SPIFFS.open(SPIFFS_IS_FRESH_START, "w");
  if (file) {
    file.println(isFreshStart);
    file.close();
  } else {
    Serial.println(F("> Failed to open \"isFreshStart\" file for writing..."));
  }
}

void serialLog() {
  Serial.print("> Total No. of Cycles : ");
  Serial.println(noOfCycles);

  Serial.print("Current Cycle Count: ");
  Serial.println(cycleCount);

  Serial.print("Remaining Cycles: ");
  Serial.println(remainingCycleCount);

  Serial.print("Single Cycle Time (ms): ");
  Serial.println(singleCycleTime);

  Serial.println("---------------");
}

void lcdLog() {
  lcd.clear();
  // Logs in LCD Display
  lcd.setCursor(0, 1); // Column, Row
  lcd.print("   ENDURANCE LAB   ");

  lcd.setCursor(0, 0); // Column, Row
  lcd.print("T.CYC: ");
  lcd.setCursor(6, 0);
  lcd.print(noOfCycles);
  lcd.setCursor(13, 0);
  lcd.print("|CPM:");
  lcd.setCursor(18, 0);
  lcd.print(cyclesPerMinute);

  lcd.setCursor(0, 3); // Column, Row
  lcd.print("CYC COMPLETED: ");
  lcd.setCursor(15, 3);
  lcd.print(cycleCount);

  lcd.setCursor(0, 2); // Column, Row
  lcd.print("CYC REMAINING: ");
  lcd.setCursor(15, 2);
  lcd.print(remainingCycleCount);
}


void sendDataToRaspberry() {
  mySerial.println(noOfCycles);
  mySerial.println(cycleCount);
  mySerial.println(remainingCycleCount);
  mySerial.println(singleCycleTime); // Replace this with your actual data
  delay(1000); // Send data every second
  
}

