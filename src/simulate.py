"""
Line Following Car — 4-Sensor Logic Simulation
================================================
Author     : Pradhyuman Kumar
Project    : B.Tech Mini Project, NIET Greater Noida (2021-22)
Supervisor : Mr. Prateek Gupta

Simulates the exact two-tier algorithm documented in the project report:
  Tier 1 — 90° turn detection (outer sensor pairs going white)
  Tier 2 — Simple line following (inner sensors SL, SR)
  Tier 3 — Junction handling with counter variable

Sensor array layout (front of robot):
  |←5cm→|←1cm→|  6cm  |←1cm→|←5cm→|
  SLL         SL       SR         SRR

Run:
    python3 simulate.py
"""

LINE_THRESHOLD = 500

SPEED_FULL = 200
SPEED_HALF = 100
SPEED_TURN = 200
SPEED_STOP = 0


# ── Data Structures ───────────────────────────────────────────────────────────

class SensorArray:
    """4-sensor IR array: SLL, SL, SR, SRR."""

    def __init__(self, sll, sl, sr, srr):
        self.SLL = sll
        self.SL  = sl
        self.SR  = sr
        self.SRR = srr

    def on_line(self, val):
        return val < LINE_THRESHOLD

    @property
    def sLL(self): return self.on_line(self.SLL)
    @property
    def sL(self):  return self.on_line(self.SL)
    @property
    def sR(self):  return self.on_line(self.SR)
    @property
    def sRR(self): return self.on_line(self.SRR)

    def __repr__(self):
        def st(v, on): return f"{'■' if on else '□'}"
        return (f"  SLL:{st(self.SLL,self.sLL)}({self.SLL:3d})  "
                f"SL:{st(self.SL,self.sL)}({self.SL:3d})  "
                f"SR:{st(self.SR,self.sR)}({self.SR:3d})  "
                f"SRR:{st(self.SRR,self.sRR)}({self.SRR:3d})"
                f"   [■=on line  □=off line]")


class MotorState:
    def __init__(self):
        self.left_speed  = 0
        self.right_speed = 0
        self.left_dir    = "FWD"
        self.right_dir   = "FWD"
        self.action      = "STOP"

    def set(self, action, l_speed, r_speed, l_dir="FWD", r_dir="FWD"):
        self.action      = action
        self.left_speed  = l_speed
        self.right_speed = r_speed
        self.left_dir    = l_dir
        self.right_dir   = r_dir

    def __repr__(self):
        return (f"  Action     : {self.action}\n"
                f"  Left motor : {self.left_dir} @ {self.left_speed}/255 PWM\n"
                f"  Right motor: {self.right_dir} @ {self.right_speed}/255 PWM")


# ── Decision Engine ───────────────────────────────────────────────────────────

class LineFollowerController:
    """Implements the two-tier algorithm from the project report."""

    def __init__(self):
        self.junction_counter = 0

    def decide(self, s: SensorArray) -> MotorState:
        m = MotorState()

        # ── TIER 1: All sensors white → T / + junction ───────────────────
        # Must check this BEFORE 90° pairs, because at a junction
        # all 4 sensors go white simultaneously
        if not s.sLL and not s.sL and not s.sR and not s.sRR:
            return self._handle_junction(m)

        # ── TIER 2: 90° turn detection ────────────────────────────────────
        # Both SL and SLL white (SRR still on black) → hard left turn
        if not s.sL and not s.sLL:
            m.set("90° PIVOT LEFT", SPEED_TURN, SPEED_TURN, "REV", "FWD")
            return m

        # Both SR and SRR white (SLL still on black) → hard right turn
        if not s.sR and not s.sRR:
            m.set("90° PIVOT RIGHT", SPEED_TURN, SPEED_TURN, "FWD", "REV")
            return m

        # ── TIER 3: Simple line following ─────────────────────────────────
        if s.sL and s.sR:
            m.set("FORWARD (ideal position)", SPEED_FULL, SPEED_FULL)
        elif s.sL and not s.sR:
            # SR off white gap → curve right (slow left/inner, full right/outer)
            m.set("CURVE RIGHT (SR off line)", SPEED_HALF, SPEED_FULL)
        elif not s.sL and s.sR:
            # SL off white gap → curve left (full left/outer, slow right/inner)
            m.set("CURVE LEFT (SL off line)", SPEED_FULL, SPEED_HALF)
        else:
            m.set("STOP (line lost)", SPEED_STOP, SPEED_STOP)

        return m

    def _handle_junction(self, m: MotorState) -> MotorState:
        c = self.junction_counter % 3
        if c == 0:
            m.set(f"JUNCTION #{self.junction_counter+1} → GO STRAIGHT",
                  SPEED_FULL, SPEED_FULL)
        elif c == 1:
            m.set(f"JUNCTION #{self.junction_counter+1} → TURN LEFT (pivot)",
                  SPEED_TURN, SPEED_TURN, "REV", "FWD")
        else:
            m.set(f"JUNCTION #{self.junction_counter+1} → TURN RIGHT (pivot)",
                  SPEED_TURN, SPEED_TURN, "FWD", "REV")
        self.junction_counter += 1
        return m


# ── Simulation Scenarios ──────────────────────────────────────────────────────

def run_simulation():
    """
    Replay sensor readings from common track scenarios and print decisions.
    Values are raw analog (0–1023): < 500 = on black line.
    """
    # (SLL, SL, SR, SRR, label)
    scenarios = [
        # ── Simple line following ─────────────────────────────────────────
        (300, 300, 300, 300, "Ideal position — all sensors on line"),
        (300, 300, 700, 300, "SR drifts off → curve right"),
        (300, 700, 300, 300, "SL drifts off → curve left"),

        # ── Gradual curve ─────────────────────────────────────────────────
        (300, 300, 600, 300, "Gentle right curve (SR slightly off)"),
        (300, 600, 300, 300, "Gentle left curve (SL slightly off)"),

        # ── 90° turns ─────────────────────────────────────────────────────
        (700, 700, 300, 300, "90° LEFT TURN — SLL+SL both go white"),
        (300, 300, 700, 700, "90° RIGHT TURN — SR+SRR both go white"),

        # ── Junction scenarios ────────────────────────────────────────────
        (700, 700, 700, 700, "T-junction / + junction (all white) — counter=0 → straight"),
        (700, 700, 700, 700, "T-junction / + junction (all white) — counter=1 → left"),
        (700, 700, 700, 700, "T-junction / + junction (all white) — counter=2 → right"),

        # ── End of track ──────────────────────────────────────────────────
        (700, 700, 700, 700, "End of track — stop"),
    ]

    ctrl = LineFollowerController()

    print("=" * 62)
    print("  LINE FOLLOWING CAR — 4-SENSOR ALGORITHM SIMULATION")
    print("  Based on B.Tech Mini Project Report — NIET, 2021-22")
    print("  Author: Pradhyuman Kumar")
    print("=" * 62)
    print("\nSensor array:  SLL ←5cm→ SL ←6cm→ SR ←5cm→ SRR\n")

    for i, (sll, sl, sr, srr, label) in enumerate(scenarios, 1):
        sensors = SensorArray(sll, sl, sr, srr)
        motor   = ctrl.decide(sensors)

        print(f"Step {i:02d}: {label}")
        print(sensors)
        print(motor)
        print()

    print("=" * 62)
    print("Simulation complete.")


if __name__ == "__main__":
    run_simulation()
