#include <Arduino.h>

// ======================================================
// FS-iA6B i-BUS
// ======================================================

HardwareSerial IBusSerial(2);

#define IBUS_RX_PIN 16

// ======================================================
// YKD2608MH STEP / DIR
// ======================================================

// PAN
#define PAN_STEP_PIN 25
#define PAN_DIR_PIN  26

// TILT
#define TILT_STEP_PIN 14
#define TILT_DIR_PIN  12

// ======================================================
// SETTINGS
// ======================================================

#define DEAD_BAND 40

// Maximum step rate
// Start LOW for testing
#define MAX_STEP_RATE 1000.0

// Minimum useful movement speed
#define MIN_STEP_RATE 80.0

// Step pulse width
#define STEP_PULSE_US 5

// Reverse direction if required
#define PAN_REVERSE  false
#define TILT_REVERSE false

// ======================================================
// i-BUS DATA
// ======================================================

uint16_t channels[14];

uint8_t ibusBuffer[32];
uint8_t ibusIndex = 0;

unsigned long lastIBusTime = 0;

// ======================================================
// STEPPER STATE
// ======================================================

unsigned long panLastStep = 0;
unsigned long tiltLastStep = 0;

bool panStepState = false;
bool tiltStepState = false;

unsigned long panPulseStart = 0;
unsigned long tiltPulseStart = 0;

// ======================================================
// READ i-BUS
// ======================================================

bool readIBus()
{
  while (IBusSerial.available())
  {
    uint8_t b = IBusSerial.read();

    if (ibusIndex == 0)
    {
      if (b != 0x20)
        continue;

      ibusBuffer[ibusIndex++] = b;
    }
    else if (ibusIndex == 1)
    {
      if (b != 0x40)
      {
        ibusIndex = 0;
        continue;
      }

      ibusBuffer[ibusIndex++] = b;
    }
    else
    {
      ibusBuffer[ibusIndex++] = b;

      if (ibusIndex >= 32)
      {
        ibusIndex = 0;

        // Check checksum
        uint16_t checksum = 0xFFFF;

        for (int i = 0; i < 30; i++)
        {
          checksum -= ibusBuffer[i];
        }

        uint16_t receivedChecksum =
          ibusBuffer[30] |
          (ibusBuffer[31] << 8);

        if (checksum != receivedChecksum)
        {
          return false;
        }

        // Extract 14 channels
        for (int ch = 0; ch < 14; ch++)
        {
          channels[ch] =
            ibusBuffer[2 + ch * 2] |
            (ibusBuffer[3 + ch * 2] << 8);
        }

        lastIBusTime = millis();

        return true;
      }
    }
  }

  return false;
}

// ======================================================
// CONVERT RC VALUE TO SPEED
// ======================================================

float rcToSpeed(uint16_t rcValue)
{
  int error = (int)rcValue - 1500;

  // Deadband
  if (abs(error) <= DEAD_BAND)
    return 0;

  // Remove deadband
  if (error > 0)
    error -= DEAD_BAND;
  else
    error += DEAD_BAND;

  // Limit range
  error = constrain(error, -460, 460);

  float magnitude =
    abs(error) / 460.0;

  float speed =
    MIN_STEP_RATE +
    magnitude *
    (MAX_STEP_RATE - MIN_STEP_RATE);

  if (error < 0)
    speed = -speed;

  return speed;
}

// ======================================================
// STEP MOTOR FUNCTION
// ======================================================

void runStepper(
  int stepPin,
  int dirPin,
  float speed,
  unsigned long &lastStep,
  bool reverseDirection)
{
  unsigned long now = micros();

  // No movement
  if (speed == 0)
  {
    digitalWrite(stepPin, LOW);
    return;
  }

  bool direction = speed > 0;

  if (reverseDirection)
    direction = !direction;

  digitalWrite(dirPin, direction ? HIGH : LOW);

  float absSpeed = abs(speed);

  unsigned long stepInterval =
    (unsigned long)(1000000.0 / absSpeed);

  if (now - lastStep >= stepInterval)
  {
    lastStep = now;

    digitalWrite(stepPin, HIGH);

    delayMicroseconds(STEP_PULSE_US);

    digitalWrite(stepPin, LOW);
  }
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  // i-BUS
  IBusSerial.begin(
    115200,
    SERIAL_8N1,
    IBUS_RX_PIN,
    -1
  );

  // Pan
  pinMode(PAN_STEP_PIN, OUTPUT);
  pinMode(PAN_DIR_PIN, OUTPUT);

  // Tilt
  pinMode(TILT_STEP_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN, OUTPUT);

  digitalWrite(PAN_STEP_PIN, LOW);
  digitalWrite(TILT_STEP_PIN, LOW);

  digitalWrite(PAN_DIR_PIN, LOW);
  digitalWrite(TILT_DIR_PIN, LOW);

  Serial.println();
  Serial.println("================================");
  Serial.println("FlySky FS-i6 + FS-iA6B + ESP32");
  Serial.println("Pan/Tilt Manual Controller");
  Serial.println("================================");
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  // Read i-BUS
  readIBus();

  // ----------------------------------------------------
  // FAILSAFE
  // ----------------------------------------------------

  if (millis() - lastIBusTime > 300)
  {
    // Stop both motors
    digitalWrite(PAN_STEP_PIN, LOW);
    digitalWrite(TILT_STEP_PIN, LOW);

    return;
  }

  // ----------------------------------------------------
  // CH1 = PAN
  // CH2 = TILT
  // ----------------------------------------------------

  uint16_t ch1 = channels[0];
  uint16_t ch2 = channels[1];

  float panSpeed =
    rcToSpeed(ch1);

  float tiltSpeed =
    rcToSpeed(ch2);

  // ----------------------------------------------------
  // RUN MOTORS
  // ----------------------------------------------------

  runStepper(
    PAN_STEP_PIN,
    PAN_DIR_PIN,
    panSpeed,
    panLastStep,
    PAN_REVERSE
  );

  runStepper(
    TILT_STEP_PIN,
    TILT_DIR_PIN,
    tiltSpeed,
    tiltLastStep,
    TILT_REVERSE
  );

  // ----------------------------------------------------
  // DEBUG
  // ----------------------------------------------------

  static unsigned long lastPrint = 0;

  if (millis() - lastPrint > 200)
  {
    lastPrint = millis();

    Serial.print("CH1 Pan: ");
    Serial.print(ch1);

    Serial.print("   Pan Speed: ");
    Serial.print(panSpeed);

    Serial.print("   CH2 Tilt: ");
    Serial.print(ch2);

    Serial.print("   Tilt Speed: ");
    Serial.println(tiltSpeed);
  }
}