# Kiroku (記録) — Logging Library

**Thread-safe, multi-sink structured logging for C++17**

Kiroku (記録, "record") is a lightweight logging library providing formatted, multi-destination log output with file rotation and configurable verbosity. Used by all Devana components.

---

## Quick Start

```cpp
#include <Logger.h>

using namespace devana::logging;

auto logger = Logger::create(LogConfig::getDevConfig());
Logger::setGlobal(logger);

LOG_INFO("Application started");
LOG_DEBUG("Processing {} sensors", sensorCount);
LOG_ERROR("Connection to {} failed: {}", host, errorMsg);
```

---

## Configuration

```cpp
// Development: DEBUG level, console only, colors enabled
LogConfig config = LogConfig::getDevConfig();

// Production: INFO level, file only, rotation enabled
LogConfig config = LogConfig::getProdConfig();

// Custom
LogConfig config;
config.level = LogLevel::WARNING;
config.output = "both";                     // "console", "file", "both", "none"
config.filepath = "logs/devana.log";
config.rotation.enabled = true;
config.rotation.maxSizeBytes = 50 * 1024 * 1024;
config.rotation.maxFiles = 10;
config.useColors = true;
config.format = "{timestamp} [{level}] [{component}] {message}";
config.component = "MyComponent";
```

---

## Format Placeholders

| Placeholder | Output |
|---|---|
| `{timestamp}` | ISO 8601 timestamp |
| `{level}` | Full level name (`DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`) |
| `{level_short}` | Short level (`DBG`, `INF`, `WRN`, `ERR`, `CRT`) |
| `{component}` | Component name |
| `{message}` | Log message text |
| `{thread}` | Thread ID |

---

## Log Levels

| Level | Color | Use |
|---|---|---|
| `DEBUG` | Cyan | Detailed diagnostic information |
| `INFO` | Green | General operational messages |
| `WARNING` | Yellow | Potential issues |
| `ERROR` | Red | Recoverable failures |
| `CRITICAL` | Bold Red | System-threatening conditions |

---

## Custom Sinks

```cpp
class MySink : public devana::logging::ILogSink {
public:
    void write(LogLevel level, const std::string& timestamp,
               const std::string& component, const std::string& message) override {
        // thread-safe write to your destination
    }
    void flush() override {}
    void setMinLevel(LogLevel level) override { m_min = level; }
    LogLevel getMinLevel() const override { return m_min; }
    std::string getName() const override { return "MySink"; }
private:
    LogLevel m_min = LogLevel::DEBUG;
};

logger->addSink(std::make_shared<MySink>());
```

---

## Architecture

```
lib/kiroku/
├── CMakeLists.txt
├── include/
│   ├── Logger.h        # Main API, global singleton, macros
│   ├── LogLevel.h      # Level enum, string conversion, color codes
│   ├── LogConfig.h     # Configuration struct and presets
│   ├── LogFormatter.h  # Format template engine
│   ├── ILogSink.h      # Sink interface + NullSink
│   ├── ConsoleSink.h   # Terminal output with ANSI colors
│   └── FileSink.h      # File output with size-based rotation
└── src/
    ├── Logger.cpp
    ├── LogConfig.cpp
    ├── LogFormatter.cpp
    ├── ConsoleSink.cpp
    └── FileSink.cpp
```

## Build

```cmake
add_subdirectory(lib/kiroku)
target_link_libraries(MyTarget kiroku::kiroku)
```

## Namespace

All public API lives under `devana::logging`.
