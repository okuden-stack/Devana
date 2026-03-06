# Devana

### AI-Powered Multi-Domain Threat Defense System

> *Devana — Slavic goddess of the hunt. She does not wait to be found.*

---

## Overview

Devana is an autonomous threat intelligence and response platform designed to sit above deployed sensor products — including Sentinel Analytics, DADS, and ACCD — and act as a unified defensive brain. It correlates inputs across physical and cyber domains, classifies threats, and either acts autonomously within safe bounds or escalates to a human operator for high-stakes decisions.

Where individual sensor products detect, **Devana decides**.

---

## Test Harness

Devana has no operator UI — it is a headless processing pipeline. The Qt6 window that opens is a **development test harness** for exercising the pipeline without building a full integration.

### Scenario picker

Scenarios are defined in `tools/config/test_scenarios.json`. Each scenario loads a schema into both the injector and extractor panels simultaneously. Add a new object to the `"scenarios"` array and restart — no recompile required.

### CLI arguments

```bash
# Open with the default first scenario
./Devana

# Preselect a scenario by name
./Devana --scenario "Detection Event (Sentinel → Devana)"

# Load a custom scenarios file and start on a specific scenario
./Devana --scenarios-file /path/to/my_scenarios.json --scenario "CAN Bus Frame"

# List all scenarios in a file and exit (no window opens)
./Devana --scenarios-file /path/to/my_scenarios.json --list-scenarios
```

| Flag | Short | Description |
|---|---|---|
| `--scenario <name>` | `-s` | Preselect scenario by exact name on startup |
| `--scenarios-file <path>` | `-f` | Load a different JSON file instead of the default |
| `--list-scenarios` | `-l` | Print scenario names and exit |

### File layout

```
tools/config/test_scenarios.json   ← scenario definitions (JSON, no recompile)
src/ui/TestField.h                 ← field descriptor struct (TestField, TestSchema)
src/ui/UnitTestInjector/           ← ZMQ PUB widget + README
src/ui/UnitTestExtractor/          ← ZMQ SUB widget + README
src/ui/TestHarness/                ← TestHarnessWindow, ScenarioLoader
```

See `src/ui/UnitTestInjector/README.md` and `src/ui/UnitTestExtractor/README.md` for the full JSON schema reference.

---

## Core Capabilities

| Capability | Description |
|---|---|
| **Sensor Fusion** | Correlates detections across Sentinel, DADS, and ACCD into unified threat events |
| **Threat Classification** | Scores and categorizes threats by type, severity, and confidence |
| **Intent Estimation** | Models behavioral sequences to assess intent over time, not just point-in-time detections |
| **Automated Response** | Triggers safe, pre-approved responses autonomously (alerts, logging, countermeasures) |
| **Human-in-the-Loop** | Escalates ambiguous or high-severity events to an operator dashboard for review |
| **Adaptive Learning** | Improves continuously from operator decisions — confirmed and dismissed threats feed back into the model |

---

## Threat Domains

Devana operates across both:

- **Physical** — intruders, drones, vehicles, perimeter breaches, behavioral anomalies
- **Cyber** — covert channels, anomalous network activity, signals intelligence (via ACCD)

Events from both domains are fused into a single unified threat picture.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        SENSOR LAYER                         │
│     Sentinel (vision)  │  DADS (drone)  │  ACCD (covert)    │
└───────────────────────────────┬─────────────────────────────┘
                                │ raw detections + metadata
┌───────────────────────────────▼─────────────────────────────┐
│                       FUSION ENGINE                         │
│   Correlates events across sensors, time, and space         │
│   Produces structured, unified threat events                │
└───────────────────────────────┬─────────────────────────────┘
                                │ fused threat events
┌───────────────────────────────▼─────────────────────────────┐
│                    THREAT CLASSIFIER                        │
│   ML model: severity score, threat type, confidence         │
│   Outputs structured threat objects with rationale          │
└──────────────┬──────────────────────────────┬───────────────┘
               │ high confidence              │ ambiguous / high severity
┌──────────────▼──────────┐       ┌───────────▼───────────────┐
│    RESPONSE ENGINE      │       │    OPERATOR DASHBOARD     │
│  Autonomous safe actions│       │  Human reviews & decides  │
│  Alerts, logging, locks │       │  Decisions fed to model   │
└─────────────────────────┘       └───────────────────────────┘
                                │ all outcomes + labels
┌───────────────────────────────▼─────────────────────────────┐
│                      LEARNING LOOP                          │
│   Labeled outcomes stored, model retrained periodically     │
│   Devana improves the longer it is deployed                 │
└─────────────────────────────────────────────────────────────┘
```

---

## ML Problem Decomposition

Devana is composed of several distinct machine learning problems, each buildable and testable independently:

### 1. Threat Classifier *(Stage 1 — start here)*

- **Type:** Supervised classification
- **Input:** Fused event features (detection type, confidence, zone, time, frequency, source sensor)
- **Output:** Threat score (0–1), threat category label
- **Model:** Gradient Boosted Tree (XGBoost) → upgrade to neural net as data grows
- **Why start here:** Fast to train, highly interpretable, no deep learning required

### 2. Sensor Fusion Engine *(Stage 2)*

- **Type:** Rule-based first, then learned correlation model
- **Input:** Asynchronous event streams from all sensors
- **Output:** Correlated, unified threat events with spatial and temporal context
- **Model:** Start with hand-crafted rules, replace with learned time-window correlation

### 3. Intent Estimator *(Stage 3)*

- **Type:** Sequence classification
- **Input:** Ordered sequence of correlated events over a time window
- **Output:** Estimated intent (surveilling, probing, transiting, attacking)
- **Model:** LSTM or Transformer
- **Why this matters:** A drone flying over once is different from a drone that orbits and returns

### 4. Adaptive Learning Pipeline *(Stage 4)*

- **Type:** Human-in-the-loop / active learning
- **Input:** Operator confirm/dismiss/escalate decisions
- **Output:** New labeled training data, periodic model retraining
- **Goal:** Devana becomes more accurate the longer it is deployed at a site

---

## Build & Dependencies

### System requirements

Run `./devana doctor` to verify all requirements are met.

| Dependency | Required | Purpose |
|---|---|---|
| CMake ≥ 3.16 | Yes | Build system |
| Qt6 | Yes | Test harness UI |
| libzmq + cppzmq | Yes | Test harness PUB/SUB transport |
| pkg-config | Yes | CMake ZMQ discovery |
| Python 3 | Yes | Build system CLI (`./devana`) |
| ONNX Runtime | No | ML inference (Stage 1+) |

### Installing ZeroMQ

```bash
# Debian / Ubuntu
sudo apt install libzmq3-dev

# Fedora / RHEL
sudo dnf install zeromq-devel

# Arch Linux
sudo pacman -S zeromq

# macOS
brew install zeromq cppzmq
```

### Build commands

```bash
./devana build              # incremental build
./devana build --debug      # debug symbols
./devana rebuild            # clean + rebuild from scratch
./devana build --regen      # force regenerate CMakeLists.txt from components.json
./devana run                # build and launch the test harness
./devana doctor             # verify all dependencies are installed
./devana info               # show component and config summary
```

---

## Tech Stack

| Layer | Technology |
|---|---|
| **Training** | Python — PyTorch, scikit-learn |
| **Data Handling** | pandas, numpy |
| **Experiment Tracking** | MLflow (self-hosted) |
| **Model Export** | ONNX |
| **C++ Inference** | ONNX Runtime |
| **Event Storage** | SQLite → PostgreSQL at scale |
| **Operator UI** | Qt6 |
| **Sensor Integration** | Custom socket layer (TCP/UDP/UDS) |

---

## Deployment Model

Devana is designed to deploy alongside existing sensor products with no changes to their internals.

- Sensor products emit detection events over **Unix Domain Sockets (UDS)** or TCP
- Devana's fusion engine subscribes to these streams
- Trained models are exported to **ONNX format** and run via **ONNX Runtime in C++**
- No Python runtime required at deployment — pure C++ binary
- Operator dashboard built in **Qt6** for consistency with existing product UIs

---

## Development Roadmap

```
Stage 1 — Threat Classifier
  ✦ Define threat event schema
  ✦ Generate synthetic labeled training data
  ✦ Train XGBoost classifier
  ✦ Evaluate: precision, recall, F1, confusion matrix
  ✦ Export to ONNX
  ✦ Load and run inference in C++

Stage 2 — Fusion Engine
  ✦ Define sensor event schema per product
  ✦ Build rule-based correlator
  ✦ Add spatial and temporal windowing
  ✦ Replace rules with learned correlation model

Stage 3 — Intent Estimator
  ✦ Build event sequence dataset
  ✦ Train LSTM on behavioral sequences
  ✦ Integrate output into threat classifier pipeline

Stage 4 — Adaptive Learning Loop
  ✦ Operator dashboard with confirm/dismiss/escalate
  ✦ Label storage pipeline
  ✦ Scheduled retraining workflow
  ✦ Model versioning with MLflow
```

---

## Key Design Principles

**Sensor agnostic** — Devana consumes structured detection events, not raw sensor data. New sensor products can be integrated by implementing the event schema.

**Interpretable first** — threat scores must be explainable to operators. Black-box decisions are not acceptable in this domain. Start with interpretable models (GBT) before reaching for complexity.

**Human authority is preserved** — automation handles only pre-approved, low-risk response actions. Any action with significant consequence requires explicit operator confirmation.

**Fail safe** — if Devana is offline or uncertain, sensors continue operating independently. Devana is additive, never a single point of failure.

**Deployable anywhere** — C++ inference binary, SQLite storage, Qt6 UI. No cloud dependency, no external services required. Runs air-gapped.

---

## Integration Points

| Product | Data Provided |
|---|---|
| **Sentinel Analytics** | Object detections, classifications, bounding boxes, camera zone, confidence scores |
| **DADS** | Drone detections, track history, altitude, bearing, RF signature |
| **ACCD** | Covert channel events, protocol anomalies, confidence score, channel type |

---

## Success Metrics

- **Threat classifier precision ≥ 0.90** — low false positive rate is critical; operator trust depends on it
- **Threat classifier recall ≥ 0.85** — missing a real threat is worse than a false alarm
- **Fusion latency < 500ms** — events must be correlated and scored before the threat has time to act
- **Operator decision time reduced by ≥ 40%** vs. reviewing raw sensor feeds manually
- **Model improves measurably** after each retraining cycle as labeled data accumulates

---

## Project Status

> 🟡 Pre-development — architecture and roadmap defined, Stage 1 implementation beginning.

---

*Part of the General Dynamics Mission Systems sensor intelligence product family.*
