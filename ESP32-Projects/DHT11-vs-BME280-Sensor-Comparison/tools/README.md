# Sensor comparison helper

`compare_sensors.py` reads a Home Assistant History CSV export and compares sensors against a reference.

```
pip install pandas numpy
python3 compare_sensors.py history.csv --list
python3 compare_sensors.py history.csv --ref mi_temperature --pairs "dht11,bme280_temperature" --kind temperature
python3 compare_sensors.py history.csv --ref mi_humidity --pairs "humidity,bme280_humidity" --kind humidity
```

Use `--list` first to see the entity names in your file, then match them by substring. `--trim-hours 1` drops the first hour while the sensors settle.
