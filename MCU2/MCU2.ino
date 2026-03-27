#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <I2CKeyPad.h>
#include <SoftwareSerial.h>

// --- I2C Multiplexer ---
#define TCA_ADDR 0x70               // Standard I2C address for TCA9548A multiplexer
#define CH_KEYPAD 0                // Channel 0
#define CH_OLED   1                // Channel 1

// --- Pin Definitions ---
#define BUZZER_PIN 4
#define OPTO_SWITCH_PIN 3

#define dirPin 13   // Direction pin
#define stepPin 12  // Step pin
#define M0_Pin 6    // Microstep select pin M0
#define M1_Pin 7    // Microstep select pin M1
#define en_Pin 8    // Enable Stepper Driver
#define RxD 9
#define TxD 10

SoftwareSerial slave(RxD, TxD);    // Acts as a serial communication device to MCU-1

char commandchar;
String command;

// --- Stepper Setup ---
const int stepsPerRev = 200;       
const int stepsFor270Deg = 150;    // 270 degrees = 200 * (270/360) 

// --- Hardware Setup ---
Adafruit_SSD1306 oled(128, 32, &Wire, -1);
I2CKeyPad keypad(0x20);            // I2C address for keypad
char keys[] = { '1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D' };

// --- State Variables ---
enum LiftState { STATIONARY, GOING_UP, GOING_DOWN };
enum DoorState { CLOSED, OPENING, OPEN, CLOSING };

LiftState liftState = STATIONARY;
DoorState doorState = CLOSED;

int currentLevel = 1;
unsigned long doorOpenTimer = 0;
const unsigned long DOOR_WAIT_TIME = 5000; // 5 seconds

unsigned long alarmTimer = 0;
bool alarmActive = false;

// --- I2C Multiplexer Function ---
void select(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

void setup() {
  Wire.begin(); 
  Serial.begin(9600);                // Hardware serial for debugging to PC
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(OPTO_SWITCH_PIN, INPUT_PULLUP); 
  
  pinMode(RxD, INPUT);               
  pinMode(TxD, OUTPUT);              
  slave.begin(9600);                 // Start communication with MCU-1
  Serial.println("setup");

  // Setup Stepper driver pins
  pinMode(en_Pin, OUTPUT);    
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(M0_Pin, OUTPUT);
  pinMode(M1_Pin, OUTPUT);

  // Enable driver & set microstepping mode
  digitalWrite(en_Pin, LOW);
  digitalWrite(M0_Pin, LOW);
  digitalWrite(M1_Pin, LOW);
  
  // Initialize Keypad
  select(CH_KEYPAD);
  keypad.begin(); 
  keypad.loadKeyMap(keys);

  // Initialize OLED
  select(CH_OLED);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  oled.setFont(&FreeSans9pt7b);
  oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay(); 
  oled.display();
  
  updateOLED(); 
}

void loop() {
  Serial.println("handlemcu1");
  handleMCU1Communication();
  Serial.println("handlekeypad");
  handleKeypad();
  Serial.println("handledoor");
  handleDoorStateMachine();
  Serial.println("handlealarm");
  handleAlarm();
}

// --- Keypad Logic ---
void handleKeypad() {
  select(CH_KEYPAD);
  
  if (keypad.isPressed()) {
    Serial.println("ispressed");
    char key = keypad.getChar();
    Serial.println(key);
    if (key == '1') {
      if (liftState == STATIONARY && currentLevel != 1) {
        slave.println("L"); // PROTOCOL UPDATE: 'L' = move to level 1
      }
    } 
    else if (key == '2') {
      if (liftState == STATIONARY && currentLevel != 2) {
        slave.println("U"); // PROTOCOL UPDATE: 'U' = move to level 2
      }
    } 
    else if (key == 'A') {
      Serial.println("A");
      alarmActive = true;
      alarmTimer = millis();
      digitalWrite(BUZZER_PIN, HIGH);
    } 
    else if (key == '#') { 
      if (liftState == STATIONARY) {
        if (doorState == CLOSED) {
          doorState = OPENING;
        } else if (doorState == OPEN) {
          doorOpenTimer = millis(); 
        }
      }
    } 
    else if (key == '*') { 
      if (liftState == STATIONARY && doorState == OPEN) {
        bool isBlocked = digitalRead(OPTO_SWITCH_PIN) == LOW; 
        if (!isBlocked) {
          doorState = CLOSING; 
        }
      }
    }
  }
}

// --- Door State Machine ---
void handleDoorStateMachine() {
  bool isBlocked = digitalRead(OPTO_SWITCH_PIN) == LOW; 

  switch (doorState) {
    case CLOSED:
      break;

    case OPENING:
      openDoor();
      doorState = OPEN;
      doorOpenTimer = millis(); 
      break;

    case OPEN:
      if (isBlocked) {
        doorOpenTimer = millis();
      } else {
        if (millis() - doorOpenTimer >= DOOR_WAIT_TIME) {
          doorState = CLOSING;
        }
      }
      break;

    case CLOSING:
      bool interrupted = closeDoorWithInterrupt();
      if (interrupted) {
        doorState = OPENING; 
      } else {
        doorState = CLOSED;
        slave.println("D"); // PROTOCOL UPDATE: 'D' = door closed ACK
      }
      break;
  }
}

// --- Stepper Motor Logic ---
void openDoor() {
  digitalWrite(dirPin, LOW); 
  for (int i = 0; i < stepsFor270Deg; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(2500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(2500);
  }
}

bool closeDoorWithInterrupt() {
  digitalWrite(dirPin, HIGH); 
  
  for (int i = 0; i < stepsFor270Deg; i++) {
    bool isBlocked = digitalRead(OPTO_SWITCH_PIN) == LOW; 
    
    if (isBlocked) {
      digitalWrite(dirPin, LOW); 
      for (int j = 0; j < i; j++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(2500);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(2500);
      }
      return true; 
    }
    
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(2500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(2500);
  }
  return false; 
}

// --- OLED Display Logic ---
void updateOLED() {
  select(CH_OLED);
  oled.clearDisplay();
  oled.setCursor(0, 20); 
  
  if (liftState == STATIONARY) {
    oled.print("Level: ");
    oled.print(currentLevel);
  } else if (liftState == GOING_DOWN) {
    oled.print("Going down");
  } else if (liftState == GOING_UP) {
    oled.print("Going up");
  }
  
  oled.display();
}

// --- MCU-1 Communication Logic ---
void handleMCU1Communication() {
  if (slave.available()) {
    commandchar = slave.read();
    Serial.println(commandchar);
    command = String(commandchar);
    
    LiftState previousState = liftState;
    
    if (command == "U") {
      liftState = GOING_UP;
    } else if (command == "D") {
      liftState = GOING_DOWN;
    } else if (command == "1") {
      liftState = STATIONARY;
      currentLevel = 1;
    } else if (command == "2") {
      liftState = STATIONARY;
      currentLevel = 2;
    } else if (command == "O") {
      if (liftState == STATIONARY) {
        if (doorState == CLOSED) {
          doorState = OPENING;
        } else if (doorState == OPEN) {
          doorOpenTimer = millis();
        }
      }
    }
    
    if (liftState != previousState) {
      updateOLED();
    }
  }
}

// --- Non-Blocking Alarm Logic ---
void handleAlarm() {
  if (alarmActive) {
    if (millis() - alarmTimer >= 1000) {
      digitalWrite(BUZZER_PIN, LOW); 
      alarmActive = false;
    }
  }
}