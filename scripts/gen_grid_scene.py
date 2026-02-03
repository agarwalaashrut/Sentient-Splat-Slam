#!/usr/bin/env python3
"""
Generate a synthetic 3D grid of "gaussians" for Week 1 testing.

IMPORTANT:
- Uses key "position" (NOT "pos")
- Output format:
  {
    "gaussians": [
      { "position": [x,y,z], "color": [r,g,b] },
      ...
    ]
  }
"""

import argparse
import json
from pathlib import Path


def clamp01(x: float) -> float:
    return max(0.0, min(1.0, x))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, default=10, help="points per axis (n x n x n)")
    ap.add_argument("--spacing", type=float, default=0.5, help="distance between grid points")
    ap.add_argument("--center", type=float, nargs=3, default=[0.0, 0.0, 0.0], help="grid center (cx cy cz)")
    ap.add_argument("--out", type=str, default="assets/test_scenes/grid_gaussians_large.json", help="output JSON path")
    args = ap.parse_args()

    n = args.n
    if n < 1:
        raise SystemExit("--n must be >= 1")

    cx, cy, cz = args.center
    spacing = args.spacing

    # Indices go 0..n-1, we center them around 0 by subtracting (n-1)/2
    half = (n - 1) / 2.0

    gaussians = []
    for iz in range(n):
        for iy in range(n):
            for ix in range(n):
                x = (ix - half) * spacing + cx
                y = (iy - half) * spacing + cy
                z = (iz - half) * spacing + cz

                # Color encodes normalized index along axes (debug-friendly gradient)
                r = clamp01(ix / (n - 1)) if n > 1 else 0.5
                g = clamp01(iy / (n - 1)) if n > 1 else 0.5
                b = clamp01(iz / (n - 1)) if n > 1 else 0.5

                gaussians.append({
                    "position": [round(x, 6), round(y, 6), round(z, 6)],
                    "color": [round(r, 6), round(g, 6), round(b, 6)],
                })

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    payload = {"gaussians": gaussians}
    out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    print(f"Wrote {len(gaussians)} points to {out_path}")


if __name__ == "__main__":
    main()
