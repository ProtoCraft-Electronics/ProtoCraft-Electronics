# The AI Prompt Behind This Build

**ProtoCraft Electronics** — ESP32 LED Toggle + OLED Status Display

Every firmware build on this channel starts as a written prompt to an AI coding
assistant, then gets reviewed and corrected with 13 years of hardware judgment
before it ever touches a board. This is the exact prompt used for this project,
shared as-is so you can run it yourself, adapt it, or see how a well-scoped
prompt turns into working embedded C.

---

## The Prompt

> Create an Arduino sketch for an ESP32 Dev Kit to turn on the onboard LED
> when a switch connected to GPIO 4 is pressed. Turn it off when it's
> pressed the next time (toggle behavior, one press = one state change).
>
> There is also a 1.3" OLED display connected (SSD1306 driver, I2C). While
> the board is booting, show the channel logo. After that, switch the
> display to show the current LED status (ON/OFF), and update it live every
> time the button is pressed.

That's the whole brief. No pin-debounce algorithm was specified, no library
names, no display layout — those are judgment calls the assistant has to make,
and where they get made well or badly is the actual review work.

## Why This Prompt Works

A prompt like this earns a good result because it specifies three things
clearly and leaves the rest open on purpose:

1. **The exact hardware** — ESP32 Dev Kit, GPIO 4 switch, onboard LED, 1.3"
   OLED. No ambiguity about what's connected to what.
2. **The exact behavior** — toggle on press, not "on while held." Two
   distinct display states (boot vs. status), not one static screen.
3. **The outcome, not the implementation** — it says *what* the board should
   do, not *how* to debounce a button or *which* bitmap format to use. That's
   left to the assistant to propose, and to the engineer to correct.

## What Still Needed a Professional Eye

This is the part a beginner-facing "AI wrote my code" video usually skips —
and the part that actually matters:

- **Debounce approach** — the first-pass answer used `delay()`-based
  debounce, which blocks the whole loop and would make the OLED status
  screen feel unresponsive. Rewritten to a non-blocking `millis()` based
  debounce so the board stays responsive to other tasks later (sensors,
  Wi-Fi, etc.) — the same pattern used across the weather station series.
- **`LED_BUILTIN` isn't universal** — plenty of ESP32 Dev Kit clones have no
  onboard user LED, or wire it to a different GPIO (GPIO2 is common). The
  sketch calls this out explicitly instead of silently assuming.
- **Display driver mismatch** — 1.3" OLEDs are sold under both SSD1306 and
  SH1106 drivers that look identical but aren't code-compatible. Worth
  checking before you assume the first answer is right.
- **Boot logo as a real bitmap, not placeholder text** — turning a channel
  logo PNG into a clean monochrome bitmap that's still legible at 128x64 is
  a design/engineering step no prompt does for you; the icon was extracted,
  thresholded, and hand-checked for legibility rather than dumped in at full
  detail.

## Try It Yourself

Copy the prompt above into your AI assistant of choice and see what you get
— then compare it against the firmware in this repo. If your assistant's
first answer uses `delay()` for debounce or assumes `LED_BUILTIN` always
exists, you've just found two of the same review points caught here.

---

*Part of the ProtoCraft Electronics "AI-assisted coding" series. Full
firmware, wiring diagram, and README: see this repository.*
