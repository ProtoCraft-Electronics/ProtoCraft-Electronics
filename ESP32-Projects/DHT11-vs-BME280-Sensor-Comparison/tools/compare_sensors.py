#!/usr/bin/env python3
"""Compare sensors from a Home Assistant History CSV export.

Usage:
  python3 compare_sensors.py history.csv --ref "Mi temperature" \
      --pairs "DHT11 temperature,BME280 temperature" --kind temperature

The History panel export has one row per state change. This script resamples
every column onto a common time grid (default 5 minutes), then reports, for
each sensor against the reference: mean offset (bias), noise after removing the
offset (standard deviation of the difference), RMSE, maximum absolute error and
the correlation. It also prints the resolution step of each sensor.

Column names are matched by a case-insensitive substring of the entity name or
entity id, so "dht11" and "bme280_temperature" both work. Run with --list to
see the names found in your file.
"""
import argparse
import sys

import numpy as np
import pandas as pd


def load(path):
    df = pd.read_csv(path)
    cols = {c.lower(): c for c in df.columns}
    # HA History export layout: entity_id, state, last_changed (names vary by version)
    ent = next((cols[k] for k in cols if k in ("entity_id", "entity")), None)
    st = next((cols[k] for k in cols if k in ("state", "value")), None)
    ts = next((cols[k] for k in cols if k in ("last_changed", "last_updated", "time", "timestamp", "datetime")), None)
    if not (ent and st and ts):
        sys.exit("Expected columns entity_id, state and last_changed. Found: %s" % list(df.columns))
    df = df[[ent, st, ts]].rename(columns={ent: "entity", st: "state", ts: "time"})
    df["time"] = pd.to_datetime(df["time"], utc=True, errors="coerce")
    df["state"] = pd.to_numeric(df["state"], errors="coerce")
    return df.dropna(subset=["time", "state"])


def pick(df, key):
    key = key.lower().strip()
    names = [e for e in df["entity"].unique() if key in str(e).lower()]
    if not names:
        sys.exit("No entity matches %r. Use --list." % key)
    if len(names) > 1:
        sys.exit("%r matches several entities: %s" % (key, names))
    return names[0]


def series(df, entity, rule):
    s = df[df["entity"] == entity].set_index("time")["state"].sort_index()
    s = s[~s.index.duplicated(keep="last")]
    return s.resample(rule).mean().ffill(limit=3)


def resolution(df, entity):
    v = np.sort(df[df["entity"] == entity]["state"].unique())
    d = np.diff(v)
    d = d[d > 1e-9]
    return float(d.min()) if len(d) else float("nan")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--list", action="store_true", help="list entities and exit")
    ap.add_argument("--ref", help="reference sensor (substring)")
    ap.add_argument("--pairs", help="comma-separated sensors to compare with the reference")
    ap.add_argument("--kind", default="value", help="label only: temperature, humidity")
    ap.add_argument("--rule", default="5min", help="resample step (default 5min)")
    ap.add_argument("--trim-hours", type=float, default=0.0, help="drop this many hours from the start (settling)")
    a = ap.parse_args()

    df = load(a.csv)
    if a.list:
        print(df.groupby("entity")["state"].agg(["count", "min", "max"]))
        return
    if not (a.ref and a.pairs):
        sys.exit("Give --ref and --pairs, or use --list")

    ref_e = pick(df, a.ref)
    ref = series(df, ref_e, a.rule)
    print("Reference: %s  (resolution step %.3g)" % (ref_e, resolution(df, ref_e)))
    rows = []
    for key in [k for k in a.pairs.split(",") if k.strip()]:
        e = pick(df, key)
        s = series(df, e, a.rule)
        j = pd.concat([ref.rename("ref"), s.rename("x")], axis=1).dropna()
        if a.trim_hours:
            j = j[j.index >= j.index[0] + pd.Timedelta(hours=a.trim_hours)]
        d = j["x"] - j["ref"]
        rows.append({
            "sensor": e,
            "n": len(j),
            "step": resolution(df, e),
            "bias": d.mean(),
            "noise_sd": d.std(),
            "rmse": float(np.sqrt((d ** 2).mean())),
            "max_abs": d.abs().max(),
            "corr": j["x"].corr(j["ref"]),
            "slope": np.polyfit(j["ref"], j["x"], 1)[0] if len(j) > 2 else float("nan"),
        })
    out = pd.DataFrame(rows).set_index("sensor")
    pd.set_option("display.width", 160)
    print("\nKind: %s, grid %s. bias = mean(sensor - reference), noise_sd = spread of the difference" % (a.kind, a.rule))
    print(out.round(3).to_string())
    print("\nA constant bias can be calibrated out. Judge sensors on noise_sd, slope (1.0 is ideal) and corr.")


if __name__ == "__main__":
    main()
