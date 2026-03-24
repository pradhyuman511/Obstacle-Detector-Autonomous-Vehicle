/*
 * Line Following Car — Pradhyuman Kumar
 * NIET, Greater Noida | B.Tech Mechanical Engineering (2021-22)
 * Supervisor: Mr. Prateek Gupta
 * ================================================================
 *
 * HARDWARE (as built):
 *   - 4x IR Optical Sensors (SLL, SL, SR, SRR)
 *     Sensor spacing: SL↔SR = 6 cm (ideal: line sits between them)
 *                     SLL↔SL = 5 cm | SR↔SRR = 5 cm
 *   - L293D Motor Driver IC (dual H-bridge)
 *   - 2x DC Gear Motors (6–9V)
 *   - 2x 3.7V Li-Ion 18650 cells in series = 7.4V supply
 *   - Caster wheel (front balance)
 *   - ON/OFF rocker switch
 *
 * ORIGINAL BUILD NOTE:
 *   The car was built as a hardware-only circuit — IR sensor outputs
 *   wired directly to L293D inputs with no microcontroller.
 *   This Arduino sketch implements the same 4-sensor feedback algorithm
 *   described in the project report for code-level control, PWM speed
 *   tuning, and serial debugging.
 *
 * SENSOR LOGIC:
 *   Sensor reads LOW  → on BLACK line  (IR absorbed)
 *   Sensor reads HIGH → on WHITE surface (IR reflected)
 *
 * ALGORITHM (from project report):
 *   Two-tier strategy:
 *   1. Simple line following (when outer sensors SLL & SRR are on black)
 *   2. 90° turn algorithm    (when SLL+SL or SR+SRR go white)
 */

// ── Pin Definitions ──────────────────────────────────────────────────────────

// 4 IR Sensor inputs (left to right: SLL, SL, SR, SRR)
const int S_LL = A0;   // Outer Left  — detects 90° left turns
const int S_L  = A1;   // Inner Left  — main line tracking
const int S_R  = A2;   // Inner Right — main line tracking
const int S_RR = A3;   // Outer Right — detects 90° right turns

// L293D Motor Driver — Left Motor
const int MOTOR_L_IN1 = 2;
const int MOTOR_L_IN2 = 3;
const int MOTOR_L_EN  = 5;   // PWM

// L293D Motor Driver — Right Motor
const int MOTOR_R_IN3 = 7;
const int MOTOR_R_IN4 = 8;
const int MOTOR_R_EN  = 6;   // PWM

// ── Speed Settings ────────────────────────────────────────────────────────────
const int SPEED_FULL  = 200;   // Full forward speed
const int SPEED_HALF  = 100;   // Reduced speed for inner wheel during curve
const int SPEED_TURN  = 200;   // Speed during 90° pivot turn
const int SPEED_STOP  = 0;

// ── Sensor threshold ──────────────────────────────────────────────────────────
// Analog value below which sensor detects the BLACK line
// Tune this based on your ambient light and sensor height (5–10 mm recommended)
const int LINE_THRESHOLD = 500;

// ── Junction counter (from project report) ────────────────────────────────────
// Used to select correct path at T / + junctions
int junctionCounter = 0;

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_L_IN1, OUTPUT);
  pinMode(MOTOR_L_IN2, OUTPUT);
  pinMode(MOTOR_L_EN,  OUTPUT);
  pinMode(MOTOR_R_IN3, OUTPUT);
  pinMode(MOTOR_R_IN4, OUTPUT);
  pinMode(MOTOR_R_EN,  OUTPUT);

  pinMode(S_LL, INPUT);
  pinMode(S_L,  INPUT);
  pinMode(S_R,  INPUT);
  pinMode(S_RR, INPUT);

  Serial.println("Line Follower Car — 4-Sensor System Initialized.");
  Serial.println("Sensors: SLL | SL | SR | SRR");
  delay(1000);
}

// ── Main Loop ─────────────────────────────────────────────────────────────────
void loop() {
  bool sLL = (analogRead(S_LL) < LINE_THRESHOLD);   // true = on black line
  bool sL  = (analogRead(S_L)  < LINE_THRESHOLD);
  bool sR  = (analogRead(S_R)  < LINE_THRESHOLD);
  bool sRR = (analogRead(S_RR) < LINE_THRESHOLD);

  // Debug: print sensor states
  Serial.print("SLL:"); Serial.print(sLL);
  Serial.print(" SL:"); Serial.print(sL);
  Serial.print(" SR:"); Serial.print(sR);
  Serial.print(" SRR:"); Serial.println(sRR);

  // ── TIER 1: Check for 90° turn condition ─────────────────────────────────
  // 90° LEFT turn: both SL and SLL go white (robot overshooting left edge)
  if (!sL && !sLL) {
    Serial.println(">> 90° LEFT TURN detected");
    turn90Left();
    return;
  }

  // 90° RIGHT turn: both SR and SRR go white (robot overshooting right edge)
  if (!sR && !sRR) {
    Serial.println(">> 90° RIGHT TURN detected");
    turn90Right();
    return;
  }

  // ── TIER 2: All sensors white → T-junction or + junction ─────────────────
  if (!sLL && !sL && !sR && !sRR) {
    Serial.println(">> JUNCTION detected (T or +)");
    handleJunction();
    return;
  }

  // ── TIER 3: Simple line following ─────────────────────────────────────────
  // Condition: outer sensors (SLL, SRR) on black → simple line mode

  if (sL && sR) {
    // Ideal position: both inner sensors on line → go straight
    moveForward();
    Serial.println(">> FORWARD (ideal)");
  }
  else if (sL && !sR) {
    // SR drifted off line → robot needs to turn RIGHT
    // Slow left motor (inner), keep right motor full
    curveRight();
    Serial.println(">> CURVE RIGHT (SR off line)");
  }
  else if (!sL && sR) {
    // SL drifted off line → robot needs to turn LEFT
    // Slow right motor (inner), keep left motor full
    curveLeft();
    Serial.println(">> CURVE LEFT (SL off line)");
  }
  else {
    // Fallback: both inner sensors off line, outers still on → stop
    stopMotors();
    Serial.println(">> STOP (line lost)");
  }

  delay(10);
}

// ── Simple Line Following Functions ──────────────────────────────────────────

void moveForward() {
  setMotor(MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_EN, true,  SPEED_FULL);
  setMotor(MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_EN, true,  SPEED_FULL);
}

void curveRight() {
  // Left motor slows (inner wheel), right stays full (outer wheel)
  setMotor(MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_EN, true, SPEED_HALF);
  setMotor(MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_EN, true, SPEED_FULL);
}

void curveLeft() {
  // Right motor slows (inner wheel), left stays full (outer wheel)
  setMotor(MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_EN, true, SPEED_FULL);
  setMotor(MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_EN, true, SPEED_HALF);
}

void stopMotors() {
  setMotor(MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_EN, true, SPEED_STOP);
  setMotor(MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_EN, true, SPEED_STOP);
}

// ── 90° Turn Functions ────────────────────────────────────────────────────────
// Strategy from project report:
// Pivot in place: one motor forward, other reverse, at full speed
// Hold until both outer sensors (SLL, SRR) return to black surface

void turn90Left() {
  // Left motor REVERSE, Right motor FORWARD → pivot left
  setMotor(MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_EN, false, SPEED_TURN);
  setMotor(MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_EN, true,  SPEED_TURN);

  // Hold turn until outer sensors re-acquire black line
  while (true) {
    bool ll = (analogRead(S_LL) < LINE_THRESHOLD);
    bool rr = (analogRead(S_RR) < LINE_THRESHOLD);
    if (ll && rr) break;
    delay(5);
  }
  stopMotors();
  delay(50);
}

void turn90Right() {
  // Left motor FORWARD, Right motor REVERSE → pivot right
  setMotor(MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_EN, true,  SPEED_TURN);
  setMotor(MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_EN, false, SPEED_TURN);

  // Hold turn until outer sensors re-acquire black line
  while (true) {
    bool ll = (analogRead(S_LL) < LINE_THRESHOLD);
    bool rr = (analogRead(S_RR) < LINE_THRESHOLD);
    if (ll && rr) break;
    delay(5);
  }
  stopMotors();
  delay(50);
}

// ── Junction Handler (T / + junction) ────────────────────────────────────────
// Uses counter variable to select path at junctions (as described in report)
// Counter 0 = go straight | 1 = turn left | 2 = turn right

void handleJunction() {
  stopMotors();
  delay(100);

  if (junctionCounter == 0) {
    Serial.println("   Junction: GO STRAIGHT");
    moveForward();
    delay(200);   // Cross the junction
  }
  else if (junctionCounter == 1) {
    Serial.println("   Junction: TURN LEFT");
    turn90Left();
  }
  else {
    Serial.println("   Junction: TURN RIGHT");
    turn90Right();
    junctionCounter = -1;   // Reset cycle
  }

  junctionCounter++;
}

// ── Low-level Motor Driver ────────────────────────────────────────────────────
// forward=true → forward direction | forward=false → reverse direction

void setMotor(int pinA, int pinB, int pinEn, bool forward, int speed) {
  digitalWrite(pinA, forward ? HIGH : LOW);
  digitalWrite(pinB, forward ? LOW  : HIGH);
  analogWrite(pinEn, speed);
}
