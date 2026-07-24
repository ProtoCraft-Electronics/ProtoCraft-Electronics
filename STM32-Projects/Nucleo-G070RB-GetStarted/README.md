# STM32CubeIDE Get Started — Download, Install & First Blink Code

First video in the STM32 series: install STM32CubeIDE from scratch, create a
new project for a NUCLEO-64 board, and get the on-board LED blinking. No
external parts — just the Nucleo board and a USB cable.

Board used on camera: **NUCLEO-G070RB**. The workflow (Board Selector →
generate code → edit `main.c` inside the `USER CODE` tags → build → flash) is
identical for every STM32 Nucleo/eval board, including F105-based boards —
only the board search term and the on-board LED's pin name change. See
**Porting to another board** below.

## 🔌 Wiring / Parts List

| Part | Notes |
|---|---|
| NUCLEO-G070RB (or any STM32 Nucleo-64 board) | On-board ST-LINK/V2-1, no external programmer needed |
| USB Type-A to Mini/Micro-B cable | Powers the board and carries the ST-LINK debug/flash link |

**Note:** Dev-board-only project, no external wiring or custom PCB — no
`hardware/` folder for this project.

## 🚀 Setup

1. Download **STM32CubeIDE** from [st.com/stm32cubeide](https://www.st.com/en/development-tools/stm32cubeide.html)
   (free, no purchase required — a "Get Software" button skips the myST
   account signup). Run the installer for your OS and accept the defaults.
2. Plug in the Nucleo board via USB. Windows installs the ST-LINK driver
   automatically; Linux/macOS need no extra driver.
3. Launch STM32CubeIDE, pick a workspace folder, then
   **File → New → STM32 Project**.
4. In the **Board Selector** tab, search for your exact board (e.g.
   `NUCLEO-G070RB`), select it, click **Next**, name the project, leave the
   language as C, and click **Finish**. Accept the prompt to initialize all
   peripherals with their default mode — this opens the Device Configuration
   Tool (CubeMX view) with the board's default pinout already applied.
5. In the **Pinout & Configuration** view, find the pin labeled with the
   on-board user LED's name (`LD4` on the G070RB) and confirm it's set to
   `GPIO_Output`. This label is what you'll use in code — see the porting
   note below if your board's LED has a different name/pin.
6. **Project → Generate Code** (or the gear icon). Accept the prompt to
   switch to the C/C++ perspective — `main.c` opens.
7. Inside the `while (1)` loop, between the `/* USER CODE BEGIN 3 */` and
   `/* USER CODE END 3 */` tags, add:
   ```c
   HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
   HAL_Delay(500);
   ```
   Only edit inside `USER CODE` tags — CubeMX preserves those blocks and
   overwrites everything else if you regenerate code later.
8. Build (hammer icon), then flash via the green **Run** button — CubeIDE
   flashes over the on-board ST-LINK and the console prints
   `Download verified successfully`.
9. The on-board LED should now blink once every second.

See [firmware/blink_first_code/main.c](firmware/blink_first_code/main.c) for
the reference file — it's the relevant slice of the CubeMX-generated project
with the two added lines marked, not a drop-in project on its own (the full
project — `Drivers/`, `Core/`, `.project`, `.cproject`, linker script — only
exists once you generate it per the steps above).

### Porting to another board (including F105)

The steps above are the same for any STM32 Nucleo or evaluation board:

1. In step 4's Board Selector, search for your board instead (e.g. an
   F105-based Connectivity-line eval board).
2. In step 5, the on-board LED may be on a different pin and have a
   different user label (e.g. `LD2` instead of `LD4`) — CubeMX shows this
   label directly in the Pinout view once the board is loaded, so use
   whatever name appears there instead of `LD4`/`LD4_Pin` in step 7.
3. Everything else — Generate Code, edit inside `USER CODE` tags, build,
   flash — is unchanged.

## 🛠️ Requirements

- STM32CubeIDE (free, includes CubeMX, HAL, and the ST-LINK GDB server)
- No external libraries — this project uses HAL only

## 📺 Video

Part of the STM32 Get Started series on the
[ProtoCraft Electronics](https://www.youtube.com/channel/UCBnjPIkKBEFrhlGcf1cxJmw?sub_confirmation=1) YouTube channel.
Video link: coming soon.
