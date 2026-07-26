# arduino_dino_run

Source code for a children's birthday invitation, built as a little Arduino Nano-powered
handheld device. It shows a scrolling Dino Run mini-game (jump the cactus with a button,
level up, track a highscore in EEPROM) plus a set of invitation screens (date, location,
RSVP, etc.) shown on a 16x2 character LCD. There's also a hidden "bomb defusal" easter egg
triggered by a long button press.

## invitation.ino vs. dino_run_waveshare_lcd

Both sketches contain the same game/invitation logic — the difference is the LCD hardware
they're written for:

- **[invitation.ino](invitation.ino)** — the original version, built for a generic
  HD44780-compatible 1602A LCD wired directly to the Nano's digital pins (parallel
  interface via `LiquidCrystal.h`). It needs a contrast potentiometer and uses 6 digital
  pins just for the display.
- **[dino_run_waveshare_lcd/](dino_run_waveshare_lcd/dino_run_waveshare_lcd.ino)** — a newer
  version for the Waveshare LCD1602 I2C Module (AiP31068L controller). This is **not** a
  generic PCF8574 I2C "backpack" board, so it uses the `Waveshare_LCD1602` library instead
  of `LiquidCrystal_I2C`. Because it talks over I2C, wiring drops from 6 pins down to just
  4 (GND, VCC, SDA, SCL), and no contrast potentiometer is needed — contrast is set
  internally by the module.

## Wiring sequence (Waveshare LCD1602 I2C build)

**Before you start:** Unplug the Nano from USB/power while wiring — always work on a dead board.

**1. LCD (I2C module) — 4 connections**

| I2C module pin | Nano pin |
|---|---|
| GND | GND (either of the two GND pins) |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

These are your pre-attached leads — just slide the female ends straight onto the matching Nano pins. This uses up one of the Nano's two GND pins.

**2. Button — 2 connections**

- Identify the two legs to use: one from the left pair, one from the right pair (pick diagonally opposite legs for easier soldering clearance).
- Solder a pigtail lead to each of those two legs.
- One pigtail → **D8**
- Other pigtail → don't plug in yet, this goes to the shared ground junction (step 4)

**3. Buzzer — 2 connections**

- Solder a pigtail to each of the buzzer's two leads.
- Positive lead → **D9**
- Negative lead → don't plug in yet, also goes to the shared ground junction

**4. Shared ground junction**

- Twist together (or solder) the button's spare ground pigtail and the buzzer's negative pigtail — these two now share one joint.
- Solder or crimp on **one more short pigtail** from that joint.
- Plug that single pigtail into the Nano's **second, remaining GND pin**.

Heat-shrink or a small dab of hot glue over this junction is worth it — it's the messiest joint in the build and the one most likely to work loose.

**5. Final check before power-up**

- Confirm nothing is bridging two adjacent Nano pins by accident (easy to do with pigtails crowded around D8/D9/GND).
- Confirm LCD's 4 leads are on GND, 5V, A4, A5 — not swapped.
- Confirm button and buzzer are each using one D-pin and sharing the common GND joint, not plugged into the Nano's GND pins directly (there physically isn't a third one).

**6. Power up and test**

- Connect via USB, upload the sketch (with the `LiquidCrystal_I2C` changes from earlier).
- LCD should light up and show the boot message immediately — if it's blank, check the module's onboard contrast trimpot before troubleshooting wiring.
- Press the button — should register in whatever the game/prompt logic expects.
- Trigger the buzzer condition — should sound.

If anything doesn't respond, the multimeter continuity check we discussed earlier for the button pairing is the fastest way to rule out a bad pin guess before assuming it's a code issue.
