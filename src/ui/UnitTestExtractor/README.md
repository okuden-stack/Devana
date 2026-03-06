# UnitTestExtractor — Schema & Scenario Reference

The `UnitTestExtractor` widget subscribes to a ZeroMQ PUB socket on a background thread and displays live field values from incoming messages. Like the injector, it is entirely schema-driven — call `setSchema()` and the display rebuilds itself.

Schemas are loaded from **`tools/config/test_scenarios.json`** at startup. See the injector README for the full field type reference. This document covers extractor-specific behaviour.

---

## What the Extractor Shows

### Live field panel ("Last Received")

One labelled row per field in the schema. Updates on every message received. Fields not present in the incoming JSON show `—`. Fields of type `readonly` are display-only and will update regardless.

**Automatic colour coding:**

| Condition | Colour |
|---|---|
| `bool` = `true` | Green bold |
| `bool` = `false` | Red |
| `real` ≥ 0.9 | Red bold (high confidence / severity) |
| `real` ≥ 0.7 | Orange |
| Everything else | Default |

This gives you an at-a-glance read on severity-type fields (confidence, threat score, etc.) without any extra configuration.

### Message log

A scrollable monospaced log of every received message, formatted as:

```
[HH:mm:ss.zzz] devana.threat
{"track_id":"trk_0001","label":"hostile_uav","severity":0.923,...}

[HH:mm:ss.zzz] devana.threat
{"track_id":"trk_0002","label":"vehicle","severity":0.412,...}
```

The log is capped at a configurable number of entries (default 500). Oldest entries are dropped when the cap is reached. Click **💾 Save Log** to write all captured payloads to a `.txt` or `.jsonl` file for offline analysis.

---

## JSON Schema Reference

The extractor block inside a scenario follows the same `fields` array format as the injector. On the extractor side, `type` controls only how the live value is displayed and colour-coded — there are no input controls.

```json
"extractor": {
  "topic":    "devana.threat",
  "endpoint": "tcp://localhost:5556",
  "fields": [
    { "name": "track_id",        "type": "string",   "required": true },
    { "name": "label",           "type": "string",   "required": true },
    { "name": "severity",        "type": "real",     "required": true, "tooltip": "0.0 – 1.0, colour-coded at 0.7 and 0.9" },
    { "name": "threat_class",    "type": "choice",   "choices": ["NONE","LOW","MEDIUM","HIGH","CRITICAL"] },
    { "name": "detected_at_ms",  "type": "uint64" },
    { "name": "requires_action", "type": "bool" },
    { "name": "model_version",   "type": "readonly", "tooltip": "Set by classifier" }
  ]
}
```

### Field types on the extractor

| Type | Display | Colour coding |
|---|---|---|
| `string` | Plain text label | None |
| `int` | Plain text label | None |
| `uint64` | Plain text label | None |
| `real` | Numeric string | Yes — red ≥ 0.9, orange ≥ 0.7 |
| `bool` | `"true"` / `"false"` | Yes — green / red |
| `choice` | Plain text label | None |
| `readonly` | Plain text label, italic | None |

> Fields in the incoming message that are **not in the schema** are silently ignored in the live panel but still appear in the raw message log. This means you can run a minimal schema for the fields you care about without needing to declare every key the publisher sends.

---

## Extractor vs Injector Field Schemas

The injector and extractor schemas for the same scenario do **not** need to be identical. Common patterns:

**Monitor a subset of fields** — declare only the fields you're interested in on the extractor:

```json
"extractor": {
  "topic": "sentinel.detection",
  "fields": [
    { "name": "confidence",     "type": "real" },
    { "name": "classification", "type": "string" }
  ]
}
```

**Show computed/downstream fields** — the extractor can declare fields that the injector never sends, such as fields added by the processing pipeline:

```json
"extractor": {
  "topic": "devana.threat",
  "fields": [
    { "name": "track_id",      "type": "string" },
    { "name": "severity",      "type": "real" },
    { "name": "model_version", "type": "readonly", "tooltip": "Added by classifier, not in injector schema" }
  ]
}
```

**Different topics** — the injector publishes on one topic, the extractor listens on a different one. This is the normal pattern when testing a full pipeline: inject on Sentinel's input topic, extract on Devana's output topic:

```json
{
  "name": "End-to-End Pipeline",
  "injector":  { "topic": "sentinel.detection", "endpoint": "tcp://*:5555",     "fields": [...] },
  "extractor": { "topic": "devana.threat",      "endpoint": "tcp://localhost:5556", "fields": [...] }
}
```

---

## Controls

| Control | Behaviour |
|---|---|
| **▶ Listen** | Connects to endpoint, starts background recv thread |
| **⏹ Stop** | Stops the recv thread cleanly (in-flight messages are dropped) |
| **🗑 Clear** | Clears the log and resets all live field labels to `—` |
| **💾 Save Log** | Opens a save dialog, writes all captured raw JSON payloads, one per line |
| **Keep last N msgs** | Caps the in-memory log. Does not affect the live field panel. |

---

## Captured Messages API

If you drive the harness programmatically (e.g. from an integration test), `UnitTestExtractor` exposes:

```cpp
// All captured raw JSON strings since last clear
QStringList msgs = extractor->capturedMessages();

// Last N only
QStringList recent = extractor->capturedMessages(10);

// Total received
int total = extractor->messageCount();

// Clear
extractor->clearLog();
```

The `messageReceived(topic, fields)` signal fires on every message with the parsed `QVariantMap`, letting you connect assertions directly:

```cpp
connect(extractor, &UnitTestExtractor::messageReceived,
        this, [](const QString& topic, const QVariantMap& fields) {
    assert(fields["severity"].toDouble() > 0.5);
});
```

---

## Adding a New Scenario

See the injector README. The extractor is configured from the same `"scenarios"` array in `tools/config/test_scenarios.json`. No recompilation required.
