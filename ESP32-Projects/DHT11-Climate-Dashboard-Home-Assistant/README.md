# ESP32 Home Climate Monitor with Home Assistant (DHT11 + MQTT Discovery)

This is the [DHT11 Climate Dashboard](../DHT11-Climate-Dashboard) with one addition: the board announces itself to Home Assistant over MQTT Discovery. Home Assistant creates the entities with no YAML, and the local dashboard, history, alerts, WiFi setup and OLED keep working exactly as before. Two automations then switch a fan that is already in Home Assistant, using two thresholds so it never chatters.

This guide is self-contained. You do not need the base README to follow it.

## Contents

1. [What this adds](#what-this-adds)
2. [What you need](#what-you-need)
3. [Wiring](#wiring)
4. [Install the Arduino tools and libraries](#install-the-arduino-tools-and-libraries)
5. [Set up the MQTT broker in Home Assistant](#set-up-the-mqtt-broker-in-home-assistant)
6. [Configure and flash the board](#configure-and-flash-the-board)
7. [Join the board to your WiFi](#join-the-board-to-your-wifi)
8. [Check the device in Home Assistant](#check-the-device-in-home-assistant)
9. [Add the fan automations](#add-the-fan-automations)
10. [Adapting it to your setup](#adapting-it-to-your-setup)
11. [Why two thresholds](#why-two-thresholds)
12. [How the MQTT part works](#how-the-mqtt-part-works)
13. [Troubleshooting](#troubleshooting)
14. [Updating a board that already runs the base project](#updating-a-board-that-already-runs-the-base-project)
15. [License](#license)

## What this adds

- MQTT Discovery: one retained configuration message per reading, so Home Assistant builds a **Climate Monitor** device on its own
- Four entities under that device: temperature, humidity, comfort score and WiFi signal
- The comfort score uses the same math as the dashboard (the thresholds on the Alerts tab decide each tier), so both show the same number
- An availability topic with a last will, so the entities show as unavailable when the board goes offline
- The device page in Home Assistant links to the dashboard at `http://climate.local`
- A pair of example automations in `home-assistant/ha-automations.yaml`
- `/api/now` on the board also reports `mqtt_enabled` and `mqtt_connected`, which helps when debugging

MQTT is optional. Leave `MQTT_HOST` empty in `secrets.h` and the sketch behaves like the base project.

## What you need

**Hardware**

- ESP32 dev board
- DHT11 sensor (a 3-pin breakout module, or a bare 4-pin sensor with a 10k resistor)
- Breadboard and jumper wires
- Optional: a momentary push button and a 1.3 inch 128x64 I2C OLED with an **SSD1306** driver. The sketch runs without them.

**Home Assistant**

- Home Assistant running on your network (Home Assistant OS or Supervised, so the add-on store is available)
- The Mosquitto broker add-on and the MQTT integration (set up in step 5)
- A fan on a switch entity that is already in Home Assistant, for example a smart plug. Treat any mains-powered load with care and use a properly rated plug.

**Software**

- Arduino IDE 2.x
- ESP32 Arduino core 3.x
- The libraries listed in step 4

## Wiring

| Component | Pin | ESP32 pin |
| --- | --- | --- |
| DHT11 | VCC | 3.3V |
| DHT11 | GND | GND |
| DHT11 | DATA | GPIO4 |
| Button (optional) | one leg | GPIO27 |
| Button (optional) | other leg | GND |
| Status LED | built in on most dev boards | GPIO2 |
| OLED (optional) | VCC | 3.3V |
| OLED (optional) | GND | GND |
| OLED (optional) | SDA | GPIO21 |
| OLED (optional) | SCL | GPIO22 |

Notes:

- A bare DHT11 needs a 10k pull-up resistor between DATA and 3.3V. Most breakout modules already include one.
- The button uses the internal pull-up, so no resistor is needed.
- If the OLED stays blank, try the I2C address `0x3D` instead of `0x3C` (`OLED_I2C_ADDR` near the top of the sketch) before suspecting the wiring.
- Many OLED modules look identical but use an SH1106 driver. This sketch needs SSD1306.

## Install the Arduino tools and libraries

1. Install the [Arduino IDE 2.x](https://www.arduino.cc/en/software).
2. Add the ESP32 board package. Open File, Preferences, and add this to "Additional boards manager URLs":
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   Then open the Boards Manager, search for **esp32** by Espressif Systems and install version 3.x.
3. Select your board under Tools, Board, esp32. **ESP32 Dev Module** works for most dev kits.
4. Open the Library Manager and install:

   | Library | Author |
   | --- | --- |
   | DHT sensor library | Adafruit |
   | Adafruit Unified Sensor | Adafruit |
   | Adafruit GFX Library | Adafruit |
   | Adafruit SSD1306 | Adafruit |
   | ESPAsyncWebServer | ESP32Async |
   | AsyncTCP | ESP32Async |
   | ArduinoJson (version 7) | Benoit Blanchon |
   | **PubSubClient** (new for this project) | Nick O'Leary |

   `DNSServer`, `Preferences`, `ESPmDNS`, `HTTPClient`, `WiFiClientSecure` and `Wire` ship with the ESP32 core.
5. Install the LittleFS uploader plugin for Arduino IDE 2.x. Download the latest `.vsix` from [earlephilhower/arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload/releases), place it in `~/.arduinoIDE/plugins/` (create the folder if needed) and restart the IDE.

## Set up the MQTT broker in Home Assistant

1. In Home Assistant, open Settings, Add-ons, Add-on store, and install **Mosquitto broker**. Start it, and turn on "Start on boot".
2. Create a user for the board. Open Settings, People, Users, add a user (for example `climate`) and note the password. A dedicated user keeps the broker login separate from your own account.
3. Open Settings, Devices and services. Home Assistant normally offers to add the **MQTT** integration as soon as the broker runs. Accept it. If it does not appear, choose Add integration, search for MQTT and select the Mosquitto broker.
4. Note the IP address of your Home Assistant machine (Settings, System, Network). You will put it in `secrets.h`. A fixed address, set in your router as a DHCP reservation, avoids surprises later.

## Configure and flash the board

1. Open `firmware/DHT11-Climate-Dashboard-HA/DHT11-Climate-Dashboard-HA.ino` in the Arduino IDE. `icon_bitmap.h` sits in the same folder and loads as a second tab.
2. Copy `secrets.h.example` to `secrets.h` in the same folder. The sketch does not compile without it. Set:

   ```cpp
   #define MQTT_HOST "192.168.1.50"     // your Home Assistant IP address
   #define MQTT_PORT 1883
   #define MQTT_USER "climate"          // the user from step 5
   #define MQTT_PASS "your-password"
   ```

   Use the IP address rather than a `.local` name so the connection does not depend on mDNS. `GOOGLE_SCRIPT_URL` is only needed for Google Sheets logging. Leave it as an empty string to skip that feature.
3. Outside India, change `GMT_OFFSET_SEC` near the top of the sketch.
4. Upload the filesystem. With the sketch open, press `Ctrl+Shift+P` (`Cmd+Shift+P` on macOS) and run **Upload LittleFS to Pico/ESP8266/ESP32**. This sends `data/index.html` and `data/setup.html` to the board. Close the Serial Monitor first, because it blocks the upload.
5. Upload the sketch.
6. Open the Serial Monitor at 115200 baud.

## Join the board to your WiFi

On first boot the board has no WiFi credentials, so it starts a setup network.

1. On your phone, connect to the **ProtoCraft-Setup** WiFi network. A setup page should open on its own. If it does not, browse to `192.168.4.1`.
2. Pick your network, enter the password and tap Connect. The board restarts and joins it.
3. Watch the Serial Monitor. You should see the board connect, then `[mqtt] connected` followed by four `discovery ... ok` lines.
4. The dashboard is at **http://climate.local**. Android support for `.local` names is inconsistent. If it does not resolve, use the IP address printed in the Serial Monitor.

To move the board to another network later, use the Settings tab (Save & reconnect, or Forget WiFi network). Holding the button for 5 seconds does the same as Forget WiFi.

## Check the device in Home Assistant

1. Open Settings, Devices and services, MQTT, and select the **Climate Monitor** device.
2. You should see temperature, humidity, comfort score and WiFi signal under one device. The device page has a link to the dashboard.
3. Note the entity ID of the temperature sensor. The automations assume `sensor.climate_monitor_temperature`.

| Entity | Example ID | Notes |
| --- | --- | --- |
| Temperature | `sensor.climate_monitor_temperature` | degrees C, whole degrees on a DHT11 |
| Humidity | `sensor.climate_monitor_humidity` | percent |
| Comfort score | `sensor.climate_monitor_comfort_score` | 0 to 100, same as the dashboard |
| WiFi signal | `sensor.climate_monitor_wifi_signal` | dBm, diagnostic |

The board publishes a new reading every 5 seconds. A DHT11 is slow to follow real changes, so a warm hand on the sensor can take 20 to 30 seconds to show up.

## Add the fan automations

`home-assistant/ha-automations.yaml` contains two automations:

| Automation | Trigger | Action |
| --- | --- | --- |
| Fan on above 27 | Temperature above 27 | Turn the fan switch on |
| Fan off below 25 | Temperature below 25 | Turn the fan switch off |

To add them:

1. Open Settings, Automations and scenes, Create automation, then the three-dot menu and **Edit in YAML**.
2. Paste one automation, replace the placeholders (see the next section) and save. Repeat for the second one.
3. Alternatively, paste the file into your `automations.yaml`. If you already have automations there, merge the entries into the existing list and do not replace the file.
4. Test by warming the sensor, or temporarily lower the thresholds, and watch the fan switch.

## Adapting it to your setup

- **Fan entity.** Replace `switch.corner_fan` with your own switch. Find it under Settings, Devices and services, Entities.
- **Temperature entity.** If your temperature entity ID differs from `sensor.climate_monitor_temperature`, update both automations.
- **Thresholds.** The values 27 and 25 are examples. Keep a gap of more than one whole degree for a DHT11 (see the next section).
- **Device name.** The device and entity names come from `MQTT_DEVICE_NAME` in the sketch. Changing it changes the entity IDs.
- **Publish interval.** `MQTT_PUBLISH_INTERVAL_MS` sets how often readings go out (5000 ms by default).
- **Dashboard link.** `MQTT_CONFIG_URL` is the link shown on the device page. Change it if you reach the board by IP.
- **A second board.** The device ID comes from the MAC address, so a second board appears as a separate device. The entity IDs and names then need renaming in Home Assistant to tell them apart.

## Why two thresholds

A single threshold makes the automation fire every time sensor noise crosses the line, so the fan switches on and off repeatedly. The DHT11 reports whole degrees, so its reading hops between neighbouring values, for example 26 and 27. A line at 26.5 sits between them and fires on every hop. Two thresholds leave a gap where the fan keeps its state. This is hysteresis, the same idea as a Schmitt trigger in a circuit. The gap should be wider than the noise in your sensor, which for a DHT11 means wider than one whole degree. Because the readings are whole numbers, "above 27" fires when the reading reaches 28 and "below 25" fires when it reaches 24.

A numeric state trigger fires when the value crosses the threshold, not while it stays above it. The YAML file also includes the single-threshold version at 26.5, commented out, so you can reproduce the chatter and then compare.

## How the MQTT part works

- **Discovery.** On every connect the board publishes four retained messages under `homeassistant/sensor/<device id>/<reading>/config`. Home Assistant reads them and creates the entities. The device id is `climate_` plus the last three bytes of the board's MAC address.
- **State.** Readings go out as one JSON message on `protocraft/<device id>/state`, and each entity picks its value out with a template.
- **Availability.** `protocraft/<device id>/status` is `online` while connected and `offline` through the last will.
- **Buffer size.** Discovery messages are larger than PubSubClient's default 256 byte buffer, so the sketch sets 1024 bytes.
- **Unreachable broker.** A failed connect blocks the main loop for up to about 3 seconds, then retries after 5 seconds, doubling up to 30 seconds. The web server runs in its own task, so the dashboard keeps serving during retries. Sensor reads and the OLED pause only during the 3 second connect attempt.

## Troubleshooting

| Symptom | Likely cause and fix |
| --- | --- |
| Sketch does not compile, `secrets.h` not found | Copy `secrets.h.example` to `secrets.h` in the sketch folder. |
| Compile error about `PubSubClient.h` | Install PubSubClient by Nick O'Leary in the Library Manager. |
| Other missing header errors | Install the libraries in the table above. Make sure ESPAsyncWebServer and AsyncTCP are the ESP32Async versions. |
| LittleFS upload fails | Close the Serial Monitor, check the plugin is installed and the board and port are selected. |
| Board does not show the ProtoCraft-Setup network | It already has saved WiFi credentials. Use Forget WiFi on the Settings tab, or hold the button for 5 seconds. |
| `[mqtt] connect failed`, state `-2` | The broker could not be reached. Check `MQTT_HOST`, that Mosquitto is running and that the board and Home Assistant share a network. |
| `[mqtt] connect failed`, state `5` | The username or password was refused. Check `MQTT_USER` and `MQTT_PASS`. |
| No device appears in Home Assistant | Confirm the MQTT integration is added. Check `mqtt_connected` in `http://climate.local/api/now`. |
| Some entities are missing | Look for `discovery ... FAILED` in the Serial Monitor. Keep `setBufferSize(1024)` in your build. |
| Entities show unavailable | The board is offline or lost the broker. They return when it reconnects. |
| Temperature unavailable or never changes | Check the DHT11 wiring and pull-up. The Serial Monitor prints `[dht11] read failed` when the sensor does not answer. |
| The automation never fires | The trigger fires on a crossing. If the room is already above 27 with the fan off, wait for a fresh crossing or switch the fan on by hand. |
| Entity IDs differ from the YAML | Check the real IDs under Settings, Devices and services, Entities, and update the YAML. |
| Dashboard works but Home Assistant shows nothing | The broker address or login in `secrets.h` is wrong. |
| Dashboard does not open at `climate.local` | Use the IP from the Serial Monitor. Android often cannot resolve `.local`. |
| Old entities linger after renaming | Delete the old device under MQTT in Home Assistant and reboot the board so it republishes. |

## Updating a board that already runs the base project

If the board already runs the base Climate Dashboard, the dashboard files and saved WiFi are already on it.

1. Install PubSubClient.
2. Add the four `MQTT_` lines to your existing `secrets.h`.
3. Upload this sketch.

The LittleFS upload is only needed on a new or erased board, or if you changed the partition scheme. The `data/` files here are identical to the base project's. Flashing the sketch does not erase LittleFS or the saved WiFi credentials.

## License

MIT for code, same as the rest of the repository.
