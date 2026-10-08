#!/usr/bin/env python3
"""Repo convention checks for ProtoCraft Electronics.

Runs on every pull request and push to main. Fails the build when a rule
below is broken, so problems are caught before they reach the public repo.

Usage:
    python3 .github/scripts/check_conventions.py                 # check the tree
    python3 .github/scripts/check_conventions.py --commits A..B  # also check commit identities
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# Files that must never be committed: real credentials and certificates.
# Only their *.example templates belong in the repo.
FORBIDDEN_FILES = {"secrets.h", "google_root_ca.h"}

# Projects that predate the "sketch lives in a folder named after it" rule.
# Their GitHub links are already in published video descriptions, so they are
# left where they are. Do not add new entries here: fix the layout instead.
LEGACY_SKETCH_LAYOUT = {
    "ESP32-Projects/GPIO_Switch_00/firmware/GPIO_Switch_00.ino",
}

# Deprecated ESP32 Arduino Core v2.x APIs (CLAUDE.md: Core v3.x only).
DEPRECATED_CALLS = {
    "ledcSetup": "use ledcAttach(pin, freq, resolution) instead (Core v3.x)",
    "ledcAttachPin": "use ledcAttach(pin, freq, resolution) instead (Core v3.x)",
    "ledcDetachPin": "use ledcDetach(pin) instead (Core v3.x)",
}

# Pre-v3 ESP-NOW receive callback: (const uint8_t *mac, const uint8_t *data, int len)
OLD_ESPNOW_RECV = re.compile(
    r"void\s+\w+\s*\(\s*const\s+uint8_t\s*\*\s*\w+\s*,\s*const\s+uint8_t\s*\*\s*\w+\s*,\s*int\s+\w+\s*\)"
)
ESPNOW_RECV_REGISTER = re.compile(r"esp_now_register_recv_cb\s*\(\s*(?:esp_now_recv_cb_t\s*\()?\s*(\w+)")

# The channel link must be the sub_confirmation URL, not the @handle URL.
HANDLE_LINK = re.compile(r"\]\(\s*https?://(?:www\.)?youtube\.com/@[^)]*\)")

CODE_SUFFIXES = {".ino", ".c", ".cpp", ".h", ".hpp"}


def tracked_files():
    out = subprocess.run(
        ["git", "ls-files", "-z"], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout
    return [Path(p) for p in out.split("\0") if p]


def strip_comments(code):
    """Remove // and /* */ comments so notes like 'replaces ledcSetup()' don't trip checks."""
    code = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def line_of(text, index):
    return text.count("\n", 0, index) + 1


def check_tree(errors):
    files = tracked_files()

    for path in files:
        posix = path.as_posix()
        if posix.startswith(".github/"):
            continue

        if path.name in FORBIDDEN_FILES:
            errors.append(f"{posix}: credential file is committed. Remove it and commit only {path.name}.example")

        if path.suffix == ".ino":
            if path.parent.name != path.stem and posix not in LEGACY_SKETCH_LAYOUT:
                errors.append(
                    f"{posix}: Arduino needs the sketch in a folder with the same name, "
                    f"e.g. {path.parent.as_posix()}/{path.stem}/{path.name}"
                )

        if path.suffix in CODE_SUFFIXES:
            raw = (ROOT / path).read_text(encoding="utf-8", errors="replace")
            code = strip_comments(raw)

            for name, fix in DEPRECATED_CALLS.items():
                for m in re.finditer(rf"\b{name}\s*\(", code):
                    errors.append(f"{posix}:{line_of(code, m.start())}: deprecated {name}(), {fix}")

            registered = set(ESPNOW_RECV_REGISTER.findall(code))
            for m in OLD_ESPNOW_RECV.finditer(code):
                func = re.match(r"void\s+(\w+)", m.group(0)).group(1)
                if func in registered:
                    errors.append(
                        f"{posix}:{line_of(code, m.start())}: ESP-NOW receive callback {func}() uses the "
                        "pre-v3 signature, use (const esp_now_recv_info_t *info, const uint8_t *data, int len)"
                    )

        if path.suffix == ".md":
            text = (ROOT / path).read_text(encoding="utf-8", errors="replace")
            for m in HANDLE_LINK.finditer(text):
                errors.append(
                    f"{posix}:{line_of(text, m.start())}: YouTube @handle link, use "
                    "https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1"
                )

    # Every project folder that contains firmware needs a README.
    projects = {Path(*p.parts[: p.parts.index("firmware")]) for p in files if "firmware" in p.parts}
    for project in sorted(projects):
        if not (ROOT / project / "README.md").is_file():
            errors.append(f"{project.as_posix()}/README.md: project has firmware/ but no README.md")


def check_commits(rev_range, errors):
    """Commits must use the repo identity's noreply address, never a personal email."""
    out = subprocess.run(
        ["git", "log", "--no-merges", "--format=%h%x09%ae%x09%ce", rev_range],
        cwd=ROOT, check=True, capture_output=True, text=True,
    ).stdout
    for line in out.splitlines():
        sha, author, committer = line.split("\t")
        for role, email in (("author", author), ("committer", committer)):
            if email == "noreply@github.com":  # commits made in the GitHub web UI
                continue
            if not email.endswith("@users.noreply.github.com"):
                errors.append(
                    f"commit {sha}: {role} email is not a GitHub noreply address. "
                    'Run: git config user.email "206307122+lithinha@users.noreply.github.com"'
                )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commits", help="git revision range whose commit identities to check, e.g. origin/main..HEAD")
    args = parser.parse_args()

    errors = []
    check_tree(errors)
    if args.commits:
        check_commits(args.commits, errors)

    if errors:
        print(f"Found {len(errors)} problem(s):\n")
        for e in errors:
            print(f"  - {e}")
        # Inline annotations on the PR's "Files changed" tab.
        for e in errors:
            m = re.match(r"([^:\s]+\.\w+)(?::(\d+))?: (.*)", e)
            if m:
                print(f"::error file={m.group(1)},line={m.group(2) or 1}::{m.group(3)}")
        return 1
    print("All convention checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
