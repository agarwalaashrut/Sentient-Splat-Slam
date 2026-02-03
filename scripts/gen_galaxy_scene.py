import json
import math
import random
# ---------------- CONFIG ----------------
OUT_FILE = "assets/test_scenes/galaxy.json"
N_POINTS = 60000
SEED = 42

RADIUS = 5.0
ARMS = 5
ARM_SPREAD = 0.25
THICKNESS = 0.18

CORE_FRAC = 0.18
HAZE_FRAC = 0.20
# ----------------------------------------


def clamp01(x):
    return max(0.0, min(1.0, x))


def lerp(a, b, t):
    return a + (b - a) * t


def lerp3(c0, c1, t):
    return [lerp(c0[i], c1[i], t) for i in range(3)]


rng = random.Random(SEED)

gaussians = []

n_core = int(N_POINTS * CORE_FRAC)
n_haze = int(N_POINTS * HAZE_FRAC)
n_arms = N_POINTS - n_core - n_haze

# ---------- CORE ----------
core_color = [1.0, 0.95, 0.85]
arm_color = [0.25, 0.6, 1.0]

for _ in range(n_core):
    r = RADIUS * (rng.random() ** 2.8) * 0.25
    theta = rng.random() * 2 * math.pi
    phi = math.acos(2 * rng.random() - 1)

    x = r * math.sin(phi) * math.cos(theta)
    y = r * math.sin(phi) * math.sin(theta)
    z = r * math.cos(phi) * 0.6

    t = clamp01(1.0 - r / (RADIUS * 0.25))
    color = lerp3(arm_color, core_color, t)

    gaussians.append({
        "position": [x, y, z],
        "color": color
    })

# ---------- SPIRAL ARMS ----------
for _ in range(n_arms):
    arm = rng.randrange(ARMS)
    arm_offset = 2 * math.pi * arm / ARMS

    r = RADIUS * (rng.random() ** 0.65)
    k = 4.5
    angle = k * math.log(r / (RADIUS * 0.02)) + arm_offset
    angle += rng.gauss(0.0, ARM_SPREAD)

    x = r * math.cos(angle)
    y = r * math.sin(angle)
    z = rng.gauss(0.0, THICKNESS * (0.4 + 0.6 * r / RADIUS))

    fade = 0.4 + 0.6 * (1.0 - r / RADIUS)
    color = [clamp01(c * fade) for c in arm_color]

    gaussians.append({
        "position": [x, y, z],
        "color": color
    })

# ---------- HAZE ----------
haze_color = [0.8, 0.3, 1.0]

for _ in range(n_haze):
    r = RADIUS * (rng.random() ** 0.9) * 1.2
    angle = rng.random() * 2 * math.pi

    x = r * math.cos(angle) + rng.gauss(0, RADIUS * 0.02)
    y = r * math.sin(angle) + rng.gauss(0, RADIUS * 0.02)
    z = rng.gauss(0, THICKNESS * 2.5)

    d = math.sqrt(x*x + y*y + z*z)
    t = clamp01(1.0 - d / (RADIUS * 1.3))
    color = [clamp01(c * (t ** 1.7)) for c in haze_color]

    gaussians.append({
        "position": [x, y, z],
        "color": color
    })

rng.shuffle(gaussians)

# ---------- WRITE JSON ----------
with open(OUT_FILE, "w") as f:
    json.dump({"gaussians": gaussians}, f, separators=(",", ":"))

print(f"Wrote {len(gaussians)} gaussians to {OUT_FILE}")
