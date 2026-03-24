# Line Following Car

### Autonomous ground robot — 4-sensor IR array with 90° turn & junction detection

![Status](https://img.shields.io/badge/status-complete-brightgreen)
![Platform](https://img.shields.io/badge/platform-Arduino%20Uno-blue)
![Language](https://img.shields.io/badge/code-C%20%2F%20Python-orange)
![Degree](https://img.shields.io/badge/B.Tech%20Mini%20Project-NIET%202021--22-navy)

---

## Overview

A two-wheeled autonomous ground robot that follows a black line on a white surface using a **4-sensor IR optical array** and an L293D dual H-bridge motor driver. The robot implements a two-tier feedback algorithm: simple line following using the inner sensor pair (SL, SR) and sharp 90° turns using the outer pair (SLL, SRR) — with a junction counter for T and + intersection handling.

Originally built as a **hardware-only circuit** (no microcontroller), where IR sensor outputs directly drive the L293D inputs. The Arduino implementation in this repo adds PWM speed control, serial debugging, and the junction counter logic documented in the project report.

> Project report: `docs/project_report.pdf`

---

## Algorithm

The navigation strategy is split into two tiers (as described in the project report):

### Tier 1 — Junction detection (all 4 sensors go white)
When all four sensors detect white simultaneously, the robot is at a T or + junction. A counter variable cycles through: **Go Straight → Turn Left → Turn Right → repeat.**

### Tier 2 — 90° turn detection (outer sensor pairs)
| Condition          | Meaning                  | Response         |
|--------------------|--------------------------|------------------|
| SLL + SL → white   | 90° left bend ahead      | Pivot LEFT in place |
| SR + SRR → white   | 90° right bend ahead     | Pivot RIGHT in place |

Pivot continues until both outer sensors (SLL, SRR) return to black, then resumes.

### Tier 3 — Simple line following (inner sensors SL, SR)
| SL    | SR    | Action                               |
|-------|-------|--------------------------------------|
| ON    | ON    | Forward — full speed both motors     |
| ON    | OFF   | Curve right — slow left, full right  |
| OFF   | ON    | Curve left  — full left, slow right  |
| OFF   | OFF   | Stop — line lost                     |

> Speed on curves: inner wheel at 100/255 PWM, outer at 200/255 — avoids jerky motion vs. full stop.

---

## Sensor Array

```
Front of robot (↑ direction of travel)

|←── 5 cm ──→|←── 1cm ──→|←─── 6 cm ───→|←── 1cm ──→|←── 5 cm ──→|
            SLL            SL              SR            SRR

Ideal position: black line sits between SL and SR (1 cm either side)
```

---

## Hardware

| Component          | Specification                               |
|--------------------|---------------------------------------------|
| IR Sensors         | IR LED + Photodiode combo (×4)              |
| Motor Driver       | L293D Dual H-Bridge IC                      |
| Motors             | DC Gear Motors 6–9V (×2)                   |
| Power              | 2× ICR-18650 3.7V Li-Ion in series = 7.4V  |
| Chassis            | 2-wheel differential + caster front wheel  |
| Switch             | ON/OFF rocker                               |

---

## Project Structure

```
line-follower-car/
├── src/
│   ├── line_follower.ino    ← Arduino sketch (C) — 4-sensor algorithm
│   └── simulate.py          ← Python simulation — test logic without hardware
├── hardware/
│   └── wiring.md            ← L293D pin map, sensor layout, wiring table
├── docs/
│   └── project_report.pdf   ← B.Tech mini project report (NIET, 2021-22)
└── README.md
```

---

## Getting Started

### Run the Python simulation (no hardware needed)

```bash
python3 src/simulate.py
```

Runs 11 scenarios covering straight, curves, 90° turns, and junctions:

```
Step 06: 90° LEFT TURN — SLL+SL both go white
  SLL:□(700)  SL:□(700)  SR:■(300)  SRR:■(300)
  Action     : 90° PIVOT LEFT
  Left motor : REV @ 200/255 PWM
  Right motor: FWD @ 200/255 PWM
```

### Upload to Arduino

1. Wire components per [`hardware/wiring.md`](hardware/wiring.md)
2. Open `src/line_follower.ino` in Arduino IDE
3. Select **Board → Arduino Uno** and correct COM port
4. Upload and open Serial Monitor at **9600 baud**
5. Calibrate `LINE_THRESHOLD` (default 500) to match your track and lighting

---

## Results (from project report)

- Successfully tracked straight segments, smooth curves, and 90° left/right turns
- Junction counter correctly cycled through straight, left, and right at T/+ intersections
- Limitation: high-speed runs caused overshooting at 90° bends due to motor momentum — resolved by reducing speed at turn entry

---

## Future Improvements

- Add PID control for smoother proportional correction (replace binary slow/full)
- Increase to 6–8 sensors for more reliable + junction detection
- Add IR-based speed reduction trigger before known 90° bends

---

## Author

**Pradhyuman Kumar**
B.Tech Mechanical Engineering (Automation & Industry 4.0)
Noida Institute of Engineering & Technology, Greater Noida — 2021-22
Supervisor: Mr. Prateek Gupta

[LinkedIn](https://linkedin.com/in/pradhyuman-kumar-626854236) · [GitHub](https://github.com/pradhyuman511)

---

## License

MIT License — free to use, modify, and share with attribution.
