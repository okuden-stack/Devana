# UnitTestInjector — Schema & Scenario Reference

The `UnitTestInjector` widget is schema-driven. You hand it a `TestSchema` (a list of `TestField` descriptors) and it renders the appropriate input controls automatically — no subclassing, no hardcoding.

Schemas are defined in **`tools/config/test_scenarios.json`** and loaded at startup. You can also define them in code via `TestHarnessWindow::configure*()` methods, but the JSON file is the preferred way to add new scenarios without recompiling.

---

## JSON Location

```
Devana/
└── tools/
    └── config/
        ├── components.json       ← build system (don't touch for test purposes)
        ├── product.json          ← build system (don't touch for test purposes)
        └── test_scenarios.json   ← test harness scenarios ← your file
```

---

## Full Schema Reference

### Top-level structure

```json
{
  "scenarios": [
    {
      "name": "Human-readable name shown in the scenario picker dropdown",
      "injector": { ... },
      "extractor": { ... }
    }
  ]
}
```

Each object in `"scenarios"` maps to one entry in the dropdown. Both `"injector"` and `"extractor"` are optional — omit one if you only need the other side.

---

### Injector block

```json
"injector": {
  "topic":    "sentinel.detection",
  "endpoint": "tcp://*:5555",
  "fields":   [ ... ]
}
```

| Key | Required | Description |
|---|---|---|
| `topic` | yes | ZeroMQ topic prefix published on every inject |
| `endpoint` | no | ZMQ bind endpoint. Default: `"tcp://*:5555"` |
| `fields` | yes | Ordered list of field descriptors (see below) |

### Extractor block

```json
"extractor": {
  "topic":    "sentinel.detection",
  "endpoint": "tcp://localhost:5555",
  "fields":   [ ... ]
}
```

| Key | Required | Description |
|---|---|---|
| `topic` | yes | ZeroMQ subscription filter |
| `endpoint` | no | ZMQ connect endpoint. Default: `"tcp://localhost:5555"` |
| `fields` | yes | Ordered list of field descriptors |

> The injector **binds** (`tcp://*:PORT`), the extractor **connects** (`tcp://localhost:PORT`). When both panels are in the same process the injector and extractor topics are typically identical.

---

### Field descriptors

Every field object requires at minimum `"name"` and `"type"`. All other keys are type-specific.

#### `"string"` — free-text line edit

```json
{
  "name":     "sensor_id",
  "type":     "string",
  "default":  "sensor_acoustic_01",
  "required": true,
  "tooltip":  "Unique identifier of the originating sensor"
}
```

| Key | Default | Notes |
|---|---|---|
| `default` | `""` | Pre-filled value |
| `required` | `false` | Adds a `*` suffix to the label |
| `tooltip` | `""` | Shown on hover |

#### `"int"` — integer spin box

```json
{
  "name":     "retry_count",
  "type":     "int",
  "default":  3,
  "min":      0,
  "max":      100,
  "required": false
}
```

#### `"uint64"` — unsigned 64-bit integer (line edit with numeric hint)

```json
{
  "name":     "timestamp_ms",
  "type":     "uint64",
  "default":  0
}
```

> `QSpinBox` cannot represent the full uint64 range, so this renders as a `QLineEdit`. Set `default` to `0` and the widget will stamp the current epoch milliseconds if your C++ code auto-fills it before injecting.

#### `"real"` — double spin box

```json
{
  "name":     "confidence",
  "type":     "real",
  "default":  0.85,
  "min":      0.0,
  "max":      1.0,
  "decimals": 3,
  "required": true
}
```

| Key | Default | Notes |
|---|---|---|
| `min` | `0.0` | Minimum value (inclusive) |
| `max` | `1000000000.0` | Maximum value (inclusive) |
| `decimals` | `3` | Decimal places shown in spin box |

#### `"bool"` — checkbox

```json
{
  "name":     "encrypted",
  "type":     "bool",
  "default":  false
}
```

#### `"choice"` — combo box (fixed set of strings)

```json
{
  "name":     "sensor_type",
  "type":     "choice",
  "choices":  ["acoustic", "camera", "radar", "lidar"],
  "default":  "acoustic",
  "required": true
}
```

| Key | Required | Notes |
|---|---|---|
| `choices` | yes | Non-empty array of strings |
| `default` | no | Must be a value in `choices`. Falls back to first item. |

#### `"readonly"` — display-only label (extractor only)

```json
{
  "name":    "model_version",
  "type":    "readonly",
  "tooltip": "Set by classifier, not user-configurable"
}
```

Renders as a greyed-out label on the injector, and a live-updating value label on the extractor. Has no `default` or `required`.

---

## Complete Example

```json
{
  "scenarios": [
    {
      "name": "Detection Event (Sentinel → Devana)",
      "injector": {
        "topic":    "sentinel.detection",
        "endpoint": "tcp://*:5555",
        "fields": [
          { "name": "sensor_id",      "type": "string", "default": "sensor_acoustic_01", "required": true,  "tooltip": "Unique sensor identifier" },
          { "name": "sensor_type",    "type": "choice", "choices": ["acoustic","camera","radar","lidar"], "default": "acoustic", "required": true },
          { "name": "timestamp_ms",   "type": "uint64", "default": 0 },
          { "name": "confidence",     "type": "real",   "default": 0.85, "min": 0.0, "max": 1.0, "decimals": 3, "required": true },
          { "name": "classification", "type": "choice", "choices": ["uav","vehicle","personnel","unknown"], "default": "uav", "required": true },
          { "name": "bearing_deg",    "type": "real",   "default": 45.0,  "min": 0.0,   "max": 360.0 },
          { "name": "range_m",        "type": "real",   "default": 120.0, "min": 0.0,   "max": 50000.0 },
          { "name": "track_id",       "type": "string", "default": "trk_0001" },
          { "name": "priority",       "type": "choice", "choices": ["LOW","MEDIUM","HIGH","CRITICAL"], "default": "HIGH" },
          { "name": "encrypted",      "type": "bool",   "default": false }
        ]
      },
      "extractor": {
        "topic":    "sentinel.detection",
        "endpoint": "tcp://localhost:5555",
        "fields": [
          { "name": "sensor_id",      "type": "string",   "required": true },
          { "name": "sensor_type",    "type": "readonly" },
          { "name": "timestamp_ms",   "type": "uint64" },
          { "name": "confidence",     "type": "real",     "required": true },
          { "name": "classification", "type": "readonly" },
          { "name": "bearing_deg",    "type": "real" },
          { "name": "range_m",        "type": "real" },
          { "name": "track_id",       "type": "string" },
          { "name": "priority",       "type": "readonly" },
          { "name": "encrypted",      "type": "bool" }
        ]
      }
    }
  ]
}
```

---

## Burst Controls

The injector has a built-in burst mode — it does not need to be specified in the JSON. At runtime:

- **Count** — how many messages to send (1–10,000)
- **Interval** — milliseconds between each send (1–10,000 ms)

Click **⚡ Burst** to start, click **⏹ Stop** to abort early.

---

## Serialisation

On each inject, the widget serialises all fields to a compact JSON object. Fields of type `readonly` are excluded. An internal `_injected_at_ms` epoch timestamp is appended automatically:

```json
{
  "sensor_id": "sensor_acoustic_01",
  "sensor_type": "acoustic",
  "timestamp_ms": "1741234567890",
  "confidence": 0.850,
  "classification": "uav",
  "bearing_deg": 45.0,
  "range_m": 120.0,
  "track_id": "trk_0001",
  "priority": "HIGH",
  "encrypted": false,
  "_injected_at_ms": "1741234567912"
}
```

This is sent as two ZMQ frames: topic frame then payload frame, matching the standard PUB/SUB pattern Sentinel and Devana use throughout.

---

## Adding a New Scenario

1. Open `tools/config/test_scenarios.json`
2. Add a new object to the `"scenarios"` array
3. Restart Devana — the scenario picker dropdown will include it automatically

No recompilation required.
