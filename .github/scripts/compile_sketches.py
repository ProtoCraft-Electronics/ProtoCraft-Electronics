#!/usr/bin/env python3
"""Compile every Arduino sketch in the repo with arduino-cli.

Each sketch is copied to a temp folder first, so:
  - *.example files (secrets.h.example, google_root_ca.h.example) are copied
    to their real names with placeholder values, and the sketch builds
    without anyone's credentials
  - the folder always matches the .ino name, as Arduino requires

Usage:
    python3 .github/scripts/compile_sketches.py               # all sketches
    python3 .github/scripts/compile_sketches.py PATH [PATH]   # only sketches under these paths
"""

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

DEFAULT_FQBN = "esp32:esp32:esp32"

# Board per path prefix. The first match wins; anything else builds for ESP32 Dev Module.
BOARDS = [
    ("STM32-Projects/", "STMicroelectronics:stm32:Nucleo_64:pnum=NUCLEO_G070RB"),
    # Uses LED_BUILTIN, which the generic ESP32 Dev Module doesn't define; README targets the DOIT board.
    ("ESP32-Projects/LED-Toggle-OLED-Status/", "esp32:esp32:esp32doit-devkit-v1"),
]


def fqbn_for(sketch: Path) -> str:
    rel = sketch.relative_to(ROOT).as_posix()
    for prefix, fqbn in BOARDS:
        if rel.startswith(prefix):
            return fqbn
    return DEFAULT_FQBN


def find_sketches(filters):
    out = subprocess.run(
        ["git", "ls-files", "-z", "*.ino"], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout
    sketches = sorted(ROOT / p for p in out.split("\0") if p)
    if filters:
        wanted = [Path(f).resolve() for f in filters]
        sketches = [s for s in sketches if any(s.is_relative_to(w) for w in wanted)]
    return sketches


def stage(ino: Path, workdir: Path) -> Path:
    """Copy the sketch's folder into workdir/<name>/ and fill in *.example files."""
    dest = workdir / ino.stem
    shutil.copytree(ino.parent, dest, ignore=shutil.ignore_patterns(".git"))
    # A legacy sketch may share its folder with other files; keep only this .ino.
    for other in dest.glob("*.ino"):
        if other.name != ino.name:
            other.unlink()
    for example in dest.rglob("*.example"):
        real = example.with_suffix("")
        if not real.exists():
            shutil.copy(example, real)
    return dest


def main():
    sketches = find_sketches(sys.argv[1:])
    if not sketches:
        print("No sketches to compile.")
        return 0

    results = []
    with tempfile.TemporaryDirectory() as tmp:
        for i, ino in enumerate(sketches):
            rel = ino.relative_to(ROOT).as_posix()
            fqbn = fqbn_for(ino)
            staged = stage(ino, Path(tmp) / str(i))
            print(f"::group::{rel}  ({fqbn})", flush=True)
            proc = subprocess.run(["arduino-cli", "compile", "--fqbn", fqbn, str(staged)])
            print("::endgroup::", flush=True)
            ok = proc.returncode == 0
            if not ok:
                print(f"::error file={rel}::Compile failed for {fqbn}")
            results.append((rel, fqbn, ok))

    failed = [r for r in results if not r[2]]
    lines = ["| Sketch | Board | Result |", "|---|---|---|"]
    lines += [f"| `{rel}` | `{fqbn}` | {'✅' if ok else '❌'} |" for rel, fqbn, ok in results]
    table = "\n".join(lines)
    print("\n" + table)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write("## Sketch compile results\n\n" + table + "\n")

    print(f"\n{len(results) - len(failed)}/{len(results)} sketches compiled.")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
