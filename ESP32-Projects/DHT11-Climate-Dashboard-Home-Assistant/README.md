# ESP32 Home Climate Monitor with Home Assistant (DHT11 + MQTT Discovery)

This is the [DHT11 Climate Dashboard](../DHT11-Climate-Dashboard) with one addition: the board announces itself to Home Assistant over MQTT Discovery. Home Assistant creates the entities with no YAML, and the local dashboard, history, alerts, WiFi setup and OLED keep working exactly as before. Two automations then switch a fan that is already in Home Assistant, using two thresholds so it never chatters.

## What this adds

- MQTT Discovery: one retained configuration message per reading, so Home Assistant builds a **Climate Monitor** device on its own
- Four entities under that device: temperature, humidity, comfort score and WiFi signal
- The comfort score uses the same math as the dashboard (the thresholds on the Alerts tab decide each tier), so both show the same number
- An availability topic with a last will, so the entities show as unavailable when the board goes offline
- The device page in Home Assistant links to the dashboard at `http://climate.local`
- A pair of example automations in `home-assistant/ha-automations.yaml`

MQTT is optional. Leave `MQTT_HOST` empty in `secrets.h` and the sketch behaves like the base project.

## What you need

- The hardware from the base project: an ESP32 dev board and a DHT11. The push button and the SSD1306 OLED are optional and the sketch runs without them. Wiring is unchanged (DHT11 data on GPIO4), see the [base README](../DHT11-Climate-Dashboard/README.md#wiring).
- Home Assistant with the Mosquitto broker add-on and the MQTT integration
- A fan on a switch entity that is already in Home Assistant (a smart plug, for example)
- The libraries from the base project, plus **PubSubClient** by Nick O'Leary
- Built with ESP32 Arduino core 3.x

## 1. Set up the broker

1. In Home Assistant, open Settings, Add-ons, and install the Mosquitto broker add-on. Start it.
2. Create a Home Assistant user for MQTT (Settings, People, Users) and note the username and password.
3. Open Settings, Devices and services. Home Assistant normally offers to add the MQTT integration as soon as the broker runs. Accept it.

## 2. Flash the board

1. Open `firmware/DHT11-Climate-Dashboard-HA/DHT11-Climate-Dashboard-HA.ino` in the Arduino IDE.
2. Copy `secrets.h.example` to `secrets.h` and set `MQTT_HOST` (your Home Assistant IP address), `MQTT_USER` and `MQTT_PASS`. Use the IP address rather than a `.local` name so the connection does not depend on mDNS.
3. Follow the setup steps in the [base README](../DHT11-Climate-Dashboard/README.md#setup): upload the `data/` folder to LittleFS, upload the sketch, then join the board to your network through the **ProtoCraft-Setup** captive portal.
4. Open the Serial Monitor at 115200 baud. You should see `[mqtt] connected` followed by four `discovery ... ok` lines.

## 3. Check the device

1. Open Settings, Devices and services, MQTT, and look for the **Climate Monitor** device.
2. You should see temperature, humidity, comfort score and WiFi signal under one device.
3. Note the entity ID of the temperature sensor. The automations below assume `sensor.climate_monitor_temperature`.

| Entity | Example ID | Notes |
| --- | --- | --- |
| Temperature | `sensor.climate_monitor_temperature` | degrees C |
| Humidity | `sensor.climate_monitor_humidity` | percent |
| Comfort score | `sensor.climate_monitor_comfort_score` | 0 to 100, same as the dashboard |
| WiFi signal | `sensor.climate_monitor_wifi_signal` | dBm, diagnostic |

The board publishes a new reading every 5 seconds.

## 4. Add the automations

`home-assistant/ha-automations.yaml` contains two automations:

| Automation | Trigger | Action |
| --- | --- | --- |
| Fan on above 27 | Temperature above 27 | Turn the fan switch on |
| Fan off below 25 | Temperature below 25 | Turn the fan switch off |

Replace `switch.corner_fan` with your own fan switch entity. Add them through Settings, Automations, then the three-dot menu, Edit in YAML, or paste the file into your `automations.yaml`.

## Why two thresholds

A single threshold makes the automation fire every time sensor noise crosses the line, so the fan switches on and off repeatedly. The DHT11 reports whole degrees, so its reading hops between neighbouring values, for example 26 and 27. A line at 26.5 sits between them and fires on every hop. Two thresholds leave a gap where the fan keeps its state. This is hysteresis, the same idea as a Schmitt trigger in a circuit. The gap should be wider than the noise in your sensor, which for a DHT11 means wider than one whole degree. Because the readings are whole numbers, "above 27" fires when the reading reaches 28 and "below 25" fires when it reaches 24.

The YAML file also includes the single-threshold version, commented out, so you can reproduce the problem.

## How the MQTT part works

- **Discovery.** On every connect the board publishes four retained messages under `homeassistant/sensor/<device id>/<reading>/config`. Home Assistant reads them and creates the entities. The device id is `climate_` plus the last three bytes of the board's MAC address.
- **State.** Readings go out as one JSON message on `protocraft/<device id>/state`, and each entity picks its value out with a template.
- **Availability.** `protocraft/<device id>/status` is `online` while connected and `offline` through the last will.
- **Buffer size.** Discovery messages are larger than PubSubClient's default 256 byte buffer, so the sketch sets 1024 bytes.
- **Unreachable broker.** A failed connect blocks the main loop for up to about 3 seconds, then retries after 5 seconds, doubling up to 30 seconds. The web server runs in its own task, so the dashboard keeps serving during retries. Sensor reads and the OLED pause only during the 3 second connect attempt.

## Troubleshooting

- **No device appears.** Confirm the broker is running and the MQTT integration is added. Check the Serial Monitor for `[mqtt] connect failed` and the state code (`-2` means the broker could not be reached, `5` means the username or password was refused).
- **Some entities are missing.** Look for `discovery ... FAILED` in the Serial Monitor. The discovery messages need the 1024 byte buffer. Check that your build keeps `setBufferSize(1024)`.
- **Entities show unavailable.** The board is offline or lost its connection to the broker. Entities return when it reconnects.
- **Temperature shows unavailable or never changes.** Check the DHT11 wiring. The Serial Monitor prints `[dht11] read failed` when the sensor does not answer.
- **The automation never fires.** A numeric state trigger fires when the value crosses the threshold, not while it stays above it. If the fan is off and the room is already above 27, wait for a fresh crossing or turn the fan on by hand.
- **Entity IDs differ.** Open the entity in Home Assistant, check its ID under Settings, and update the YAML.
- **The dashboard works but Home Assistant shows nothing.** The problem is the broker address or login in `secrets.h`. Check `mqtt_connected` in `http://climate.local/api/now`.

## License

MIT for code, same as the rest of the repository.
