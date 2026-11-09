# DHT11 vs BME280: ESP32 sensor comparison with Home Assistant

An ESP32 with a DHT11 and a BME280 on the same board, publishing both sensors to Home Assistant over MQTT Discovery. Use it to log the two sensors next to a reference sensor already in Home Assistant, export the data as a CSV, and see which one you can trust. A small Python script does the maths.

The firmware is the [DHT11 Climate Dashboard with Home Assistant](../DHT11-Climate-Dashboard-Home-Assistant) sketch with the BME280 added. The web dashboard, history, alerts, WiFi setup page and OLED still work as before.

## Contents

1. [What you get](#what-you-get)
2. [What you need](#what-you-need)
3. [Wiring](#wiring)
4. [Install the Arduino tools and libraries](#install-the-arduino-tools-and-libraries)
5. [Set up the MQTT broker in Home Assistant](#set-up-the-mqtt-broker-in-home-assistant)
6. [Configure and flash the board](#configure-and-flash-the-board)
7. [Check the entities in Home Assistant](#check-the-entities-in-home-assistant)
8. [Run a fair comparison](#run-a-fair-comparison)
9. [Export the data from Home Assistant](#export-the-data-from-home-assistant)
10. [Analyse the CSV](#analyse-the-csv)
11. [Measure the effect of an enclosure](#measure-the-effect-of-an-enclosure)
12. [Troubleshooting](#troubleshooting)
13. [License](#license)

## What you get

- Both sensors publish to Home Assistant every 5 seconds under one device, **Climate Monitor**:

| Entity | Example ID | Source | Notes |
| --- | --- | --- | --- |
| Temperature | `sensor.climate_monitor_temperature` | DHT11 | whole degrees C |
| Humidity | `sensor.climate_monitor_humidity` | DHT11 | percent |
| BME280 temperature | `sensor.climate_monitor_bme280_temperature` | BME280 | degrees C, two decimals |
| BME280 humidity | `sensor.climate_monitor_bme280_humidity` | BME280 | percent |
| BME280 pressure | `sensor.climate_monitor_bme280_pressure` | BME280 | hPa, station pressure (not corrected to sea level) |
| Comfort score | `sensor.climate_monitor_comfort_score` | DHT11 | 0 to 100, same as the dashboard |
| WiFi signal | `sensor.climate_monitor_wifi_signal` | ESP32 | dBm, diagnostic |

- The DHT11 entities keep the same IDs as in the Home Assistant project, so any automation that uses them keeps working.
- The BME280 runs in forced mode: one measurement per read, then the chip sleeps. That keeps its self-heating negligible.
- If no BME280 is found, the sketch carries on with the DHT11 only.
- `tools/compare_sensors.py` compares any sensor against a reference from a Home Assistant CSV export.

## What you need

| Part | Notes |
| --- | --- |
| ESP32 dev board | 30-pin ESP32 Dev Kit, 3.3 V logic |
| DHT11 | module or bare sensor (bare sensors need a 10k pull-up on the data line) |
| BME280 breakout | I2C version, address `0x76` or `0x77`. Check the breakout's supply rating and power it from 3.3 V |
| Reference sensor | any room thermometer that already logs temperature and humidity to Home Assistant |
| Home Assistant | with the Mosquitto broker app and the MQTT integration |
| Breadboard, jumper wires | |
| OLED, button (optional) | as in the base project |

## Wiring

| Component | Pin | ESP32 pin |
| --- | --- | --- |
| DHT11 | VCC | 3.3V |
| DHT11 | GND | GND |
| DHT11 | DATA | GPIO4 |
| BME280 | VIN | 3.3V |
| BME280 | GND | GND |
| BME280 | SDA | GPIO21 |
| BME280 | SCL | GPIO22 |
| OLED (optional) | SDA, SCL | GPIO21, GPIO22 (shared with the BME280) |
| Button (optional) | one leg | GPIO27 |
| Button (optional) | other leg | GND |

The BME280 and the OLED share the same I2C bus. Their addresses differ, so they do not clash. Keep the BME280 on short leads, close to the DHT11 and away from the ESP32 module, which runs warm.

## Install the Arduino tools and libraries

1. Install the Arduino IDE 2.x and add the ESP32 board package (Espressif, via Boards Manager).
2. In the Library Manager install:

   | Library | Author |
   | --- | --- |
   | DHT sensor library | Adafruit |
   | Adafruit Unified Sensor | Adafruit |
   | **Adafruit BME280 Library** | Adafruit |
   | Adafruit GFX Library | Adafruit |
   | Adafruit SSD1306 | Adafruit |
   | ArduinoJson (version 7) | Benoit Blanchon |
   | PubSubClient | Nick O'Leary |
   | ESPAsyncWebServer and AsyncTCP | ESP32Async (install from GitHub if they are not in the Library Manager) |

3. Install the LittleFS uploader plugin for Arduino IDE 2.x from [earlephilhower/arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload/releases) and restart the IDE.

## Set up the MQTT broker in Home Assistant

If Home Assistant already runs the Mosquitto broker and the MQTT integration, skip to the next section.

1. In Home Assistant open Settings, Apps (called Add-ons in older versions), open the store and install **Mosquitto broker**. Start it and turn on Start on boot.
2. Create a user for the board under Settings, People, Users, for example `climate`, and note the password.
3. Open Settings, Devices and services and accept the **MQTT** integration when Home Assistant offers it. If it does not appear, choose Add integration and search for MQTT.
4. Note the IP address of the machine running Home Assistant. A DHCP reservation in your router keeps it fixed.

The [Home Assistant project README](../DHT11-Climate-Dashboard-Home-Assistant#set-up-the-mqtt-broker-in-home-assistant) has the same steps in more detail.

## Configure and flash the board

1. Open `firmware/DHT11-vs-BME280-Sensor-Comparison/DHT11-vs-BME280-Sensor-Comparison.ino` in the Arduino IDE. `icon_bitmap.h` loads as a second tab. Keep the whole folder together: the folder name must match the `.ino` name, and `icon_bitmap.h` and `secrets.h` must sit next to it.
2. Copy `secrets.h.example` to `secrets.h` in the same folder and fill in:

   ```cpp
   #define MQTT_HOST "192.168.1.50"     // your Home Assistant IP address
   #define MQTT_PORT 1883
   #define MQTT_USER "climate"
   #define MQTT_PASS "your-password"
   ```

   Leave `GOOGLE_SCRIPT_URL` as an empty string unless you want Google Sheets logging.
3. Outside India, change `GMT_OFFSET_SEC` near the top of the sketch.
4. To build without the BME280, set `BME280_ENABLED` to 0 near the top of the sketch.
5. Close the Serial Monitor, then upload the `data/` folder to the board: press `Ctrl+Shift+P` (`Cmd+Shift+P` on macOS) and run **Upload LittleFS to Pico/ESP8266/ESP32**.
6. Upload the sketch. Open the Serial Monitor at 115200 baud.
7. On first boot the board starts a **ProtoCraft-Setup** WiFi network. Join it with your phone, pick your network and enter the password.

In the Serial Monitor you should see, in order: `[wifi] connected`, `[bme280] found`, `[mqtt] connected`, and then one `discovery ... ok` line for each entity (seven in total). The dashboard is at `http://climate.local`, or at the IP address printed in the Serial Monitor.

## Check the entities in Home Assistant

1. Open Settings, Devices and services, MQTT, and select the **Climate Monitor** device.
2. All seven entities from the table above should show values and update every 5 seconds.
3. The BME280 entities only appear when the BME280 was found at boot.

## Run a fair comparison

The result is only as good as the setup. Use these rules for every run:

1. Put the board and the reference sensor on the same shelf, a few centimetres apart, at the same height.
2. Keep them out of direct sun and away from vents, fans, heaters and anything warm.
3. Keep the DHT11 and the BME280 together, and away from the ESP32 module, which runs warm.
4. Do not move or touch anything during a run.
5. Note the clock time when each run starts.
6. Ignore the first hour of each run while the sensors settle.

| Run | Duration | Conditions | Purpose |
| --- | --- | --- | --- |
| Open air | 48 hours | Board open on the shelf | Which sensor tracks the reference best |
| Enclosure | 24 hours | Board in its enclosure, same position | How much the enclosure shifts the reading |
| Vented enclosure (optional) | 12 to 24 hours | Enclosure with vent slots | Whether vents reduce the drift |

A reference sensor is a second opinion, not the truth. Judge each sensor by how it differs from the reference, and check the reference's own accuracy in its datasheet.

## Export the data from Home Assistant

Home Assistant can export recorded history as a CSV from the browser. You need no extra software.

1. Open **History** in the sidebar.
2. Pick the entities to compare. Use **Choose area, device or entity** and add:
   - the reference temperature entity
   - `sensor.climate_monitor_temperature` (DHT11)
   - `sensor.climate_monitor_bme280_temperature` (BME280)

   For humidity add the matching three humidity entities. You can select all six at once.
3. Set the time range with the date picker at the top, so that it covers the whole run.
4. Open the three-dot menu at the top right and choose **Download data**. The browser saves a CSV file.
5. Check that the file opens and has one row per state change, with columns `entity_id`, `state` and `last_changed`.

Notes:

- Home Assistant keeps recorder history for 10 days by default, so export soon after a run. To keep it longer, raise `purge_keep_days` in the `recorder:` section of `configuration.yaml`.
- The reference sensor and the board report at different rates (every few minutes against every 5 seconds). The analysis script resamples everything onto a common time grid.
- Entity IDs depend on how you named them. Check yours under Settings, Devices and services, Entities.

## Analyse the CSV

```
pip install pandas numpy
python3 tools/compare_sensors.py history.csv --list
python3 tools/compare_sensors.py history.csv --ref <reference temperature> --pairs "climate_monitor_temperature,bme280_temperature" --kind temperature --trim-hours 1
python3 tools/compare_sensors.py history.csv --ref <reference humidity> --pairs "climate_monitor_humidity,bme280_humidity" --kind humidity --trim-hours 1
```

Use `--list` first to see the entity names in your file. `--ref` and `--pairs` match on part of the entity name, so a short unique piece is enough. More options are in [`tools/README.md`](tools/README.md).

The script prints, for each sensor against the reference:

| Column | Meaning |
| --- | --- |
| `step` | smallest change the sensor reports (the DHT11 reports whole degrees) |
| `bias` | average difference from the reference. A constant bias can be calibrated out |
| `noise_sd` | spread of the difference after the average. Lower is better |
| `rmse` | overall error, bias included |
| `max_abs` | worst single difference |
| `corr` | how closely the shape follows the reference |
| `slope` | change per degree of the reference. 1.0 is ideal |

Judge a sensor on `noise_sd`, `slope` and `step`, since a fixed `bias` can be subtracted in code. Pressure has no reference in this setup, so the script does not score it.

## Measure the effect of an enclosure

Heat from the ESP32 can warm the air around a sensor in a closed box, which raises the temperature reading and lowers the relative humidity reading.

1. Finish the open-air run first.
2. Put the board in its enclosure, in the same position, and keep the reference outside beside it. Note the time the lid closes.
3. Run for 24 hours, export the CSV and analyse the BME280 against the reference as above.
4. Drift = the BME280 `bias` in the enclosure run minus its `bias` in the open-air run.
5. Optionally repeat with vent slots at the top and bottom of the enclosure, or with the sensor on a short lead outside the heat zone, and compare the drift.

Correcting the drift in software only works when the heat load is steady. If your WiFi duty cycle or sleep pattern changes, so does the offset.

## Troubleshooting

| Symptom | Likely cause and fix |
| --- | --- |
| `icon_bitmap.h: No such file or directory` | Keep the whole sketch folder together. The folder name must match the `.ino` name |
| `secrets.h` not found | Copy `secrets.h.example` to `secrets.h` in the sketch folder |
| Compile error about `Adafruit_BME280.h` | Install Adafruit BME280 Library, or set `BME280_ENABLED` to 0 |
| Compile error about `PubSubClient.h` | Install PubSubClient by Nick O'Leary |
| `[bme280] not found at 0x76 or 0x77` | Check SDA on GPIO21, SCL on GPIO22, power and ground. Some breakouts are SPI-only or need the I2C solder jumper set |
| BME280 entities missing in Home Assistant | The sensor was not found at boot. Fix the wiring and reset the board. If old entities linger, delete the device under MQTT and reboot the board |
| `[mqtt] connect failed, state -2` | The broker is unreachable. Check `MQTT_HOST`, that Mosquitto is running and that both are on the same network |
| `[mqtt] connect failed, state 5` | The username or password was refused. Check `MQTT_USER` and `MQTT_PASS` |
| DHT11 reads fail or never change | Check the data wiring and pull-up. The Serial Monitor prints `[dht11] read failed` |
| BME280 reads a few degrees high even in open air | It is too close to the ESP32 module or a regulator. Move it away on a longer lead |
| `tools/compare_sensors.py` reports no matching entity | Run it with `--list` and use a part of the exact entity name |
| History export is empty | The range is outside recorder retention, or the entities were not selected. Pick the entities and the date range again |
| The dashboard does not open at `climate.local` | Use the IP address printed in the Serial Monitor. Android often cannot resolve `.local` names |

## License

MIT, see the repository's `LICENSE`.
