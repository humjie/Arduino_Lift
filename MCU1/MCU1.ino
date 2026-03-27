#include <SoftwareSerial.h>

// ---------- Pins ----------
#define encoderPinA 2
#define encoderPinB 3
#define M1 5          // PWM
#define M2 11         // Direction
#define limitswitch 6 // Lobby call at Level 2
#define pushbutton 7  // Lobby call at Level 1
#define RxD 9
#define TxD 10
#define led1 12
#define led2 13

SoftwareSerial master(RxD, TxD);

// ---------- Encoder / Lift Position ----------
volatile long encoderCount = 0;

// !!! CHANGE THIS to match your encoder !!!
const long COUNTS_PER_REV = 360;      // example only
const long LEVEL1_POS = 0;            // 0 degree
const long LEVEL2_POS = 2 * COUNTS_PER_REV; // 720 degree = 2 rev

const long FLOOR_TOLERANCE = 20;      // acceptable arrival tolerance

// ---------- Request / System State ----------
bool busy = false;              // only one request at a time
bool waitingDoorClose = false;  // waiting MCU2 to confirm door closed
int requestedFloor = 0;         // 1 or 2
String rxBuffer = "";

// ---------- Function Prototypes ----------
void encoderISR();
int getCurrentFloor();
void handleLobbyButtons();
void handleUART();
void startRequest(int floor);
void moveLiftToFloor(int floor);
void moveMotorToTarget(long targetPos);
void stopMotor();
void setMotorUp(int pwm);
void setMotorDown(int pwm);
void blinkLED(int ledPin, int times);
void sendToMCU2(const char *msg);

void setup() {
  Serial.begin(9600);
  master.begin(9600);

  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);

  pinMode(limitswitch, INPUT);  
  pinMode(pushbutton, INPUT);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(encoderPinA), encoderISR, RISING);

  // Default reset state: lift at Level 1, door closed
  encoderCount = LEVEL1_POS;
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  stopMotor();

  Serial.println("MCU1 ready");
}

void loop() {
  handleUART();

  if (!busy) {
    handleLobbyButtons();
  }
}

// ---------- ISR ----------
void encoderISR() {
  // Determine direction using encoder B
  if (digitalRead(encoderPinB) == HIGH) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

// ---------- Floor Detection ----------
int getCurrentFloor() {
  long pos = encoderCount;

  if (abs(pos - LEVEL1_POS) <= FLOOR_TOLERANCE) {
    return 1;
  }
  if (abs(pos - LEVEL2_POS) <= FLOOR_TOLERANCE) {
    return 2;
  }
  return 0; // between floors
}

// ---------- Read Lobby Buttons ----------
void handleLobbyButtons() {
  if (digitalRead(pushbutton) == HIGH) {
    startRequest(1);
  } 
  else if (digitalRead(limitswitch) == HIGH) {
    startRequest(2);
  }
}

// ---------- UART Handling ----------
void handleUART() {
  while (master.available()) {
    char c = master.read();

    if (c == '\n') {
      rxBuffer.trim();
      Serial.print("Received from MCU2: ");
      Serial.println(rxBuffer);

      if (!busy) {
        if (rxBuffer == "REQ1") {
          startRequest(1);
        } 
        else if (rxBuffer == "REQ2") {
          startRequest(2);
        }
      }

      if (waitingDoorClose && rxBuffer == "DOOR_CLOSED") {
        waitingDoorClose = false;
        moveLiftToFloor(requestedFloor);
      }

      rxBuffer = "";
    } 
    else {
      rxBuffer += c;
    }
  }
}

// ---------- Start Processing a Request ----------
void startRequest(int floor) {
  if (busy) return;

  busy = true;
  requestedFloor = floor;

  if (floor == 1) digitalWrite(led1, HIGH);
  if (floor == 2) digitalWrite(led2, HIGH);

  int currentFloor = getCurrentFloor();

  Serial.print("Current floor = ");
  Serial.println(currentFloor);
  Serial.print("Requested floor = ");
  Serial.println(floor);

  // If already on requested floor, just ask MCU2 to open the door
  if (currentFloor == floor) {
    sendToMCU2("OPEN");
    if (floor == 1) blinkLED(led1, 2);
    if (floor == 2) blinkLED(led2, 2);

    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);

    busy = false;
    requestedFloor = 0;
    return;
  }

  // Otherwise, ensure door is closed before moving
  sendToMCU2("CLOSE");
  waitingDoorClose = true;
}

// ---------- Lift Motion ----------
void moveLiftToFloor(int floor) {
  if (floor == 1) {
    sendToMCU2("DOWN");
    moveMotorToTarget(LEVEL1_POS);
    sendToMCU2("AT1");
  } 
  else if (floor == 2) {
    sendToMCU2("UP");
    moveMotorToTarget(LEVEL2_POS);
    sendToMCU2("AT2");
  }

  // At arrival, open door
  sendToMCU2("OPEN");

  if (floor == 1) blinkLED(led1, 2);
  if (floor == 2) blinkLED(led2, 2);

  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);

  busy = false;
  requestedFloor = 0;
}

void moveMotorToTarget(long targetPos) {
  long startPos = encoderCount;
  long totalTravel = abs(targetPos - startPos);

  if (totalTravel == 0) {
    stopMotor();
    return;
  }

  long phase1End = totalTravel / 4;        // first 25% = 180°
  long phase2End = (totalTravel * 3) / 4;  // middle 50% = 360°

  while (abs(encoderCount - targetPos) > FLOOR_TOLERANCE) {
    long travelled = abs(encoderCount - startPos);

    int pwmVal;
    if (travelled < phase1End) {
      pwmVal = 77;   // 30% of 255
    } 
    else if (travelled < phase2End) {
      pwmVal = 230;  // 90% of 255
    } 
    else {
      pwmVal = 77;   // 30% of 255
    }

    if (encoderCount < targetPos) {
      // Ascending = CCW
      setMotorUp(pwmVal);
    } else {
      // Descending = CW
      setMotorDown(pwmVal);
    }
  }

  stopMotor();

  // Snap exactly to floor position if you want logical clean state
  encoderCount = targetPos;
}

// ---------- Motor Control ----------
void setMotorUp(int pwm) {
  digitalWrite(M2, HIGH);   // direction
  analogWrite(M1, pwm);     // speed
}

void setMotorDown(int pwm) {
  digitalWrite(M2, LOW);    // direction
  analogWrite(M1, pwm);     // speed
}

void stopMotor() {
  analogWrite(M1, 0);
}

// ---------- LED ----------
void blinkLED(int ledPin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, LOW);
    delay(250);
    digitalWrite(ledPin, HIGH);
    delay(250);
  }
  digitalWrite(ledPin, LOW);
}

// ---------- UART Send ----------
void sendToMCU2(const char *msg) {
  master.println(msg);
  Serial.print("Sent to MCU2: ");
  Serial.println(msg);
}