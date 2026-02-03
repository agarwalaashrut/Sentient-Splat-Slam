# generate_gaussians_scene.py
# Outputs JSON with gaussians: mean, scale, rotation(quat), opacity, color
#
# Format per Gaussian:
# {
#   "mean": [x,y,z],
#   "scale": [sx,sy,sz],
#   "rotation": [x,y,z,w],   # quaternion
#   "opacity": a,
#   "color": [r,g,b]
# }
#
# Notes:
# - "mean" is the position.
# - "scale" is the Gaussian standard deviation along local axes (before rotation).
# - "rotation" orients those axes in world space.

import json
import math
import random
from dataclasses import dataclass
from typing import List, Tuple


# ----------------------------
# Small vector/quaternion utils
# ----------------------------

def v_add(a, b): return (a[0]+b[0], a[1]+b[1], a[2]+b[2])
def v_sub(a, b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def v_mul(a, s): return (a[0]*s, a[1]*s, a[2]*s)
def v_dot(a, b): return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
def v_len(a): return math.sqrt(v_dot(a, a))
def v_norm(a):
    L = v_len(a)
    if L < 1e-12: return (0.0, 0.0, 0.0)
    return (a[0]/L, a[1]/L, a[2]/L)

def v_cross(a, b):
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0],
    )

def clamp(x, lo=0.0, hi=1.0):
    return max(lo, min(hi, x))

def lerp(a, b, t):
    return a + (b - a) * t

def lerp3(c0, c1, t):
    return (lerp(c0[0], c1[0], t), lerp(c0[1], c1[1], t), lerp(c0[2], c1[2], t))

def hsv_to_rgb(h, s, v):
    # h in [0,1)
    h = h % 1.0
    i = int(h * 6.0)
    f = h * 6.0 - i
    p = v * (1.0 - s)
    q = v * (1.0 - f * s)
    t = v * (1.0 - (1.0 - f) * s)
    i %= 6
    if i == 0: r,g,b = v,t,p
    elif i == 1: r,g,b = q,v,p
    elif i == 2: r,g,b = p,v,t
    elif i == 3: r,g,b = p,q,v
    elif i == 4: r,g,b = t,p,v
    else: r,g,b = v,p,q
    return (r,g,b)

def quat_from_axis_angle(axis, angle_rad):
    ax = v_norm(axis)
    s = math.sin(angle_rad * 0.5)
    return (ax[0]*s, ax[1]*s, ax[2]*s, math.cos(angle_rad * 0.5))

def quat_mul(q1, q2):
    x1,y1,z1,w1 = q1
    x2,y2,z2,w2 = q2
    return (
        w1*x2 + x1*w2 + y1*z2 - z1*y2,
        w1*y2 - x1*z2 + y1*w2 + z1*x2,
        w1*z2 + x1*y2 - y1*x2 + z1*w2,
        w1*w2 - x1*x2 - y1*y2 - z1*z2
    )

def quat_from_basis(x_axis, y_axis, z_axis):
    # Build quaternion from orthonormal basis (columns = x,y,z)
    # Robust-ish conversion
    m00, m01, m02 = x_axis[0], y_axis[0], z_axis[0]
    m10, m11, m12 = x_axis[1], y_axis[1], z_axis[1]
    m20, m21, m22 = x_axis[2], y_axis[2], z_axis[2]
    tr = m00 + m11 + m22
    if tr > 0.0:
        S = math.sqrt(tr + 1.0) * 2.0
        w = 0.25 * S
        x = (m21 - m12) / S
        y = (m02 - m20) / S
        z = (m10 - m01) / S
    elif (m00 > m11) and (m00 > m22):
        S = math.sqrt(1.0 + m00 - m11 - m22) * 2.0
        w = (m21 - m12) / S
        x = 0.25 * S
        y = (m01 + m10) / S
        z = (m02 + m20) / S
    elif m11 > m22:
        S = math.sqrt(1.0 + m11 - m00 - m22) * 2.0
        w = (m02 - m20) / S
        x = (m01 + m10) / S
        y = 0.25 * S
        z = (m12 + m21) / S
    else:
        S = math.sqrt(1.0 + m22 - m00 - m11) * 2.0
        w = (m10 - m01) / S
        x = (m02 + m20) / S
        y = (m12 + m21) / S
        z = 0.25 * S
    # Normalize
    L = math.sqrt(x*x + y*y + z*z + w*w)
    return (x/L, y/L, z/L, w/L)

def quat_align_z_to_dir(direction, roll_rad=0.0):
    """
    Create quaternion whose local +Z axis points along 'direction'.
    Optional roll around that axis.
    """
    z = v_norm(direction)
    # Pick an "up" that isn't parallel to z
    up = (0.0, 1.0, 0.0) if abs(z[1]) < 0.9 else (1.0, 0.0, 0.0)
    x = v_norm(v_cross(up, z))
    y = v_cross(z, x)
    q = quat_from_basis(x, y, z)
    if abs(roll_rad) > 1e-12:
        q_roll = quat_from_axis_angle(z, roll_rad)
        q = quat_mul(q, q_roll)
    return q


# ----------------------------
# Scene generators
# ----------------------------

@dataclass
class Gaussian:
    mean: Tuple[float, float, float]
    scale: Tuple[float, float, float]
    rotation: Tuple[float, float, float, float]
    opacity: float
    color: Tuple[float, float, float]

def torus_knot(p: int, q: int, t: float, R=2.5, r=1.0) -> Tuple[float,float,float]:
    # Classic (p,q) torus knot
    # https://en.wikipedia.org/wiki/Torus_knot (param form)
    ct = math.cos(t)
    st = math.sin(t)
    cqt = math.cos(q*t)
    sqt = math.sin(q*t)
    x = (R + r*cqt) * math.cos(p*t)
    y = (R + r*cqt) * math.sin(p*t)
    z = r * sqt
    return (x, y, z)

def approx_tangent(f, t, eps=1e-3):
    a = f(t - eps)
    b = f(t + eps)
    return v_norm(v_sub(b, a))

def add_torus_ribbon(gaussians: List[Gaussian], n=7000, seed=1):
    random.seed(seed)
    p, q = 2, 3
    # Offset torus to one side
    offset = (-8.0, 3.0, -5.0)
    def f(tt): 
        knot = torus_knot(p, q, tt, R=2.7, r=1.1)
        return v_add(knot, offset)

    for i in range(n):
        t = (i / n) * (2.0 * math.pi) * 6.0  # multiple loops
        center = f(t)
        tan = approx_tangent(f, t)

        # Tube thickness via "radial" jitter perpendicular to tangent
        # Build a perpendicular frame
        up = (0.0, 1.0, 0.0) if abs(tan[1]) < 0.9 else (1.0, 0.0, 0.0)
        n1 = v_norm(v_cross(tan, up))
        n2 = v_cross(tan, n1)

        ang = random.random() * 2.0 * math.pi
        rad = (random.random() ** 0.5) * 0.35
        offset = v_add(v_mul(n1, math.cos(ang) * rad), v_mul(n2, math.sin(ang) * rad))
        pos = v_add(center, offset)

        # Orient local Z along tangent so anisotropy “streaks” along the curve
        roll = (random.random() - 0.5) * 0.8
        rot = quat_align_z_to_dir(tan, roll_rad=roll)

        # Anisotropic: long along tangent (local Z), thinner across
        base = 0.015 + 0.020 * random.random()
        scale = (base * (0.6 + 0.8 * random.random()),
                 base * (0.6 + 0.8 * random.random()),
                 base * (3.0 + 5.0 * random.random()))

        # Color gradient along t with slight noise
        hue = (t / (2.0 * math.pi)) * 0.08 + 0.55  # bluish/purple band
        col = hsv_to_rgb(hue, 0.65, 1.0)
        col = (clamp(col[0] + (random.random()-0.5)*0.08),
               clamp(col[1] + (random.random()-0.5)*0.08),
               clamp(col[2] + (random.random()-0.5)*0.08))

        # Opacity: stronger near centerline
        a = 0.18 + 0.55 * math.exp(-(rad*rad) / (2*0.12*0.12))
        a *= (0.7 + 0.6*random.random())
        a = clamp(a, 0.05, 0.95)

        gaussians.append(Gaussian(pos, scale, rot, a, col))

def add_spiral_galaxy_disk(gaussians: List[Gaussian], n=9000, seed=2):
    random.seed(seed)
    # A flat-ish disk with spiral arms and a brighter core
    # Offset to another side
    offset = (9.0, -2.0, 7.0)
    arms = 3
    for i in range(n):
        # radius distribution (more points near center)
        u = random.random()
        r = (u ** 1.7) * 5.0  # [0,5]
        theta = random.random() * 2.0 * math.pi

        # Spiral arm offset
        arm_id = random.randrange(arms)
        arm_phase = (2.0 * math.pi * arm_id) / arms
        spiral = 1.35 * math.log1p(r)  # curl
        theta2 = theta + arm_phase + spiral

        # Position in disk
        x = r * math.cos(theta2)
        z = r * math.sin(theta2)
        y = (random.random() - 0.5) * (0.06 + 0.02*r)  # thin disk

        # Add some noise and a mild warp
        x += (random.random() - 0.5) * 0.06
        z += (random.random() - 0.5) * 0.06
        y += 0.08 * math.sin(0.6 * theta2) * (r/5.0)

        pos = (x + offset[0], y + offset[1], z + offset[2])

        # Orient streaks tangential to arm direction
        # Tangent approx in xz-plane
        tan = v_norm((-math.sin(theta2), 0.0, math.cos(theta2)))
        rot = quat_align_z_to_dir(tan, roll_rad=(random.random()-0.5)*0.4)

        # Scale: thin vertically, longer along tangent
        base = 0.010 + 0.018 * random.random()
        scale = (base * (0.7 + 0.5*random.random()),
                 base * 0.25,
                 base * (2.0 + 3.5*random.random()))

        # Color: bluish arms, warm core
        tcol = clamp(r / 5.0)
        arm_col = (0.55, 0.70, 1.00)  # blue-white
        core_col = (1.00, 0.75, 0.45)  # warm
        col = lerp3(core_col, arm_col, tcol)
        # slight random sparkle
        sparkle = (random.random() ** 10) * 0.7
        col = (clamp(col[0] + sparkle), clamp(col[1] + sparkle), clamp(col[2] + sparkle))

        # Opacity: brighter near arms and core
        core_boost = math.exp(-(r*r)/(2*1.2*1.2))
        a = 0.08 + 0.35*core_boost + 0.18*(1.0 - tcol)
        a *= (0.7 + 0.7*random.random())
        a = clamp(a, 0.03, 0.85)

        gaussians.append(Gaussian(pos, scale, rot, a, col))

def add_glow_clusters(gaussians: List[Gaussian], clusters=6, per_cluster=1200, seed=3):
    random.seed(seed)
    for c in range(clusters):
        # Random cluster center around a larger space
        cx = (random.random()-0.5) * 18.0
        cy = (random.random()-0.5) * 12.0
        cz = (random.random()-0.5) * 18.0
        center = (cx, cy, cz)

        # Cluster color theme
        hue = random.random()
        base_col = hsv_to_rgb(hue, 0.55, 1.0)

        for i in range(per_cluster):
            # Gaussian blob around center - increased spread
            # Box-muller-ish: sum of uniforms
            dx = (random.random()+random.random()+random.random()-1.5) * 0.75
            dy = (random.random()+random.random()+random.random()-1.5) * 0.50
            dz = (random.random()+random.random()+random.random()-1.5) * 0.75
            pos = (center[0] + dx, center[1] + dy, center[2] + dz)

            # Random orientation (clusters can be “puffy”)
            # Align to some random direction
            dir = v_norm((random.random()-0.5, random.random()-0.5, random.random()-0.5))
            rot = quat_align_z_to_dir(dir, roll_rad=(random.random()-0.5)*math.pi)

            # Mostly isotropic, slight anisotropy
            base = 0.018 + 0.030 * random.random()
            scale = (base*(0.7+0.6*random.random()),
                     base*(0.7+0.6*random.random()),
                     base*(0.7+0.6*random.random()))

            # Opacity stronger near center
            rr = dx*dx + dy*dy + dz*dz
            a = 0.08 + 0.65 * math.exp(-rr / (2*0.35*0.35))
            a *= (0.6 + 0.8*random.random())
            a = clamp(a, 0.02, 0.95)

            # Color with subtle core whitening
            core = math.exp(-rr / (2*0.18*0.18))
            col = lerp3(base_col, (1.0, 1.0, 1.0), 0.35*core)

            gaussians.append(Gaussian(pos, scale, rot, a, col))


# ----------------------------
# Main
# ----------------------------

def build_scene(count_hint=0, seed=1337):
    # count_hint is not strict; we add multiple components.
    # Use seed to make it deterministic.
    gaussians: List[Gaussian] = []

    add_torus_ribbon(gaussians, n=7000, seed=seed+10)
    add_spiral_galaxy_disk(gaussians, n=9000, seed=seed+20)
    add_glow_clusters(gaussians, clusters=6, per_cluster=1200, seed=seed+30)

    return gaussians

def to_json(gaussians: List[Gaussian]):
    out = {
        "format": "gaussian3d_v1",
        "gaussians": [
            {
                "mean": [g.mean[0], g.mean[1], g.mean[2]],
                "scale": [g.scale[0], g.scale[1], g.scale[2]],
                "rotation": [g.rotation[0], g.rotation[1], g.rotation[2], g.rotation[3]],
                "opacity": float(g.opacity),
                "color": [g.color[0], g.color[1], g.color[2]],
            }
            for g in gaussians
        ]
    }
    return out

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default="scene_gaussians.json")
    parser.add_argument("--seed", type=int, default=1337)
    args = parser.parse_args()

    gaussians = build_scene(seed=args.seed)
    data = to_json(gaussians)

    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)

    print(f"Wrote {len(gaussians)} gaussians to {args.out}")

# python scripts/generate_gaussians_scene.py --out assets/test_scenes/scene_gaussians.json --seed 42