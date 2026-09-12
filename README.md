# BMW K-CAN2 Trackskip

`trackskip` turns the two Free-to-Use (FTU) buttons on a JQ Werks Madtrace wheel into one-shot previous/next-track commands for the BMW media system. It is a deliberately small, dedicated M5Stack module: one AtomS3R receives the two dry contacts and an Atomic CAN Base sends the confirmed media frames to K-CAN2.

> **Vehicle-safety boundary.** This project transmits CAN frames. It is not a passive diagnostic tool. Build and validate it on a bench first, then connect only to the intended K-CAN2 pair. Do not attach it to PT-CAN or any airbag/SRS wiring. Disconnect the battery negative before working around the airbag and retain the steering-clock-spring centre position.

## What is confirmed, and what still needs a bench check

| Item | Status | Basis |
| --- | --- | --- |
| Two independent FTU button signals share one common conductor. | Confirmed by the supplied JQ manual. | FTU #1 is red/white; FTU #2 is yellow/green. |
| Previous command | Supplied and accepted for this project. | Standard CAN `0x0A3`, DLC 2: `[FD FF]`. |
| Next command | Supplied and accepted for this project. | Standard CAN `0x0A3`, DLC 2: `[FE FF]`. |
| Neutral command | Supplied and accepted for this project. | Standard CAN `0x0A3`, DLC 2: `[FC FF]`. |
| K-CAN2 bitrate | Configuration assumption. | `100 kbit/s`; confirm by passive capture on this exact car before normal-mode connection. |
| Frame DLC and neutral timing | Bench-validation item. | Firmware emits DLC 2 and neutral 80 ms after each press. |

## Hardware

| Qty. | Module | Purpose |
| ---: | --- | --- |
| 1 | M5Stack AtomS3R | Controller and two button inputs. |
| 1 | M5Stack Atomic CAN Base (CA-IS3050G) | Isolated physical CAN interface on the Atom lower connector. |
| 1 | Atom Grove cable / strain-relieved pigtail | Connects the three FTU conductors to the Atom Grove port. |
| 1 | Suitable automotive enclosure, fused 5 V supply and K-CAN2 harness | Installation-specific. |

The Atomic CAN Base occupies Atom pins G5/G6. The two FTU inputs therefore use the Grove port: G1/GPIO1 and G2/GPIO2. They are configured as pull-up inputs; a closed button pulls the input to the FTU common/GND.

```text
JQ FTU common ───────────────────────────── Atom Grove GND
JQ FTU #1 red/white (previous) ──────────── Atom Grove G1 / GPIO1
JQ FTU #2 yellow/green (next) ───────────── Atom Grove G2 / GPIO2

AtomS3R + Atomic CAN Base ───────────────── K-CAN2 CAN-H / CAN-L
```

Do **not** join the FTU common to vehicle chassis, 12 V, a CAN wire or an airbag/SRS circuit. Verify the three wheel-side conductors with a multimeter while the wheel is unplugged: each button must short only its own signal to the shared common. For an installation with a long/noisy cable, add a small automotive-qualified input conditioner or at least validate the internal pull-ups and debounce on the actual harness before permanent assembly.

## Behaviour

On a debounced press, `trackskip` sends the relevant command once, waits 80 ms, then sends neutral. Holding a button does not repeat tracks. The Atom display shows `PREVIOUS`, `NEXT`, `READY`, or a transmit/bus fault. The serial log prints every queued frame, but a queued transmit is not proof of an accepted command; check the K-CAN2 trace and iDrive behaviour during bench validation.

## Bench plan

1. With the controller unpowered and disconnected from the car, verify the FTU common and two button contacts using continuity only.
2. Power the Atom from USB. Check that each press produces exactly one command and `[FC FF]` after 80 ms in a CAN analyser trace.
3. On the target vehicle, capture K-CAN2 passively first. Confirm 100 kbit/s, standard ID `0x0A3`, DLC 2, and that no existing ECU produces a conflicting frame.
4. Only then reconnect the Atomic CAN Base in normal mode and test one command at a time with a stable power supply. If the command does not work, unplug the module and return to passive capture; do not try other buses or IDs.

## Build and upload

```bash
git clone https://github.com/untoco/trackskip.git
cd trackskip
python3 -m venv .tooling/platformio
.tooling/platformio/bin/python -m pip install --upgrade pip "platformio==6.1.19"
.tooling/platformio/bin/pio run
.tooling/platformio/bin/pio device list
.tooling/platformio/bin/pio run --target upload --upload-port /dev/cu.usbmodem101
```

Choose the actual port reported by `pio device list`; the example port above is not universal. The project intentionally does not upload automatically.

## Sources

- JQ Werks Madtrace user manual supplied with this task: FTU #1/FTU #2 wiring designation and the manufacturer’s airbag/clock-spring warnings.
- [M5Stack Atomic CAN Base documentation](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) for the CA-IS3050G interface and Atom connector assignment.
- [Espressif TWAI API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/twai.html) for normal-mode transmission and bus-off handling.
