# Hardware Wiring — Line Following Car

## Components (as used in the actual build)

| Component              | Specification                        |
|------------------------|--------------------------------------|
| Chassis                | 2-wheel differential + caster front  |
| DC Gear Motors         | 6–9V (×2, left & right)              |
| Motor Driver IC        | L293D Dual H-Bridge                  |
| IR Optical Sensors     | IR LED + Photodiode combo (×4)       |
| Power Supply           | 2× ICR-18650 Li-Ion 3.7V in series = 7.4V |
| Switch                 | ON/OFF rocker switch                 |

---

## Sensor Array Layout

```
Front of robot (direction of travel ↑)

|←── 5 cm ──→|← 1cm →|←── 6 cm ──→|← 1cm →|←── 5 cm ──→|
            SLL        SL            SR        SRR

In ideal position: the black line sits between SL and SR
(each is 1 cm from the line edge)
```

| Sensor | Position     | Purpose                          |
|--------|--------------|----------------------------------|
| SLL    | Outer Left   | Detects 90° left turn conditions |
| SL     | Inner Left   | Primary line tracking            |
| SR     | Inner Right  | Primary line tracking            |
| SRR    | Outer Right  | Detects 90° right turn conditions|

---

## L293D Pin Mapping (Arduino version)

```
L293D IC (DIP-16)
──────────────────────────────────────────────────────
Pin 1  (EN1/ENA) → Arduino D5 (PWM) — Left motor enable
Pin 2  (IN1)     → Arduino D2       — Left motor direction A
Pin 3  (OUT1)    → Left Motor terminal A
Pin 6  (OUT2)    → Left Motor terminal B
Pin 7  (IN2)     → Arduino D3       — Left motor direction B
Pin 8  (Vs)      → 7.4V battery supply (motor power)
Pin 9  (EN2/ENB) → Arduino D6 (PWM) — Right motor enable
Pin 10 (IN3)     → Arduino D7       — Right motor direction A
Pin 11 (OUT3)    → Right Motor terminal A
Pin 14 (OUT4)    → Right Motor terminal B
Pin 15 (IN4)     → Arduino D8       — Right motor direction B
Pin 16 (Vss)     → 5V logic supply
Pins 4,5,12,13   → GND
```

---

## IR Sensor Wiring (all 4 sensors)

```
Sensor  | VCC | GND | OUT (analog)
────────|─────|─────|─────────────
SLL     | 5V  | GND | Arduino A0
SL      | 5V  | GND | Arduino A1
SR      | 5V  | GND | Arduino A2
SRR     | 5V  | GND | Arduino A3
```

---

## L293D Motor Truth Table

| IN_A | IN_B | Motor direction |
|------|------|-----------------|
|  1   |  0   | Forward         |
|  0   |  1   | Reverse         |
|  0   |  0   | Coast (stop)    |
|  1   |  1   | Brake           |

---

## Sensor Reading Logic

| Analog reading | Threshold | Surface detected |
|----------------|-----------|-----------------|
| < 500          | LOW       | Black line (IR absorbed) |
| ≥ 500          | HIGH      | White surface (IR reflected) |

> Calibration note: Threshold of 500 is a starting value. Adjust in
> `line_follower.ino` based on your room lighting and sensor height.
> Recommended sensor clearance: 5–10 mm from track surface.

---

## Hardware-Only Mode (Original Build — No Microcontroller)

The car was originally built as a **pure hardware circuit**:

```
IR Sensor OUT  ──→  L293D Input pins directly
EN1, EN2       ──→  Tied HIGH (motors always enabled)
```

The digital output of each sensor (HIGH on white, LOW on black) directly
drives the corresponding L293D motor channel, creating analog feedback
with no code. The Arduino version was added to enable PWM speed control,
serial monitoring, and the junction counter logic.
