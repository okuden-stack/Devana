/**
 * @file    TestField.h
 * @brief   Field descriptor for dynamic test harness UI population
 * @author  AK
 *
 * Define a schema as a QList<TestField> and pass it to UnitTestInjector
 * or UnitTestExtractor — both widgets render themselves from it.
 *
 * EXAMPLE — DetectionEvent schema:
 *
 *   QList<TestField> schema = {
 *       TestField::string("sensor_id",      "sensor_acoustic_01",   true),
 *       TestField::string("sensor_type",    "acoustic",             true),
 *       TestField::uint64("timestamp_ms",   0),
 *       TestField::real  ("confidence",     0.85, 0.0, 1.0),
 *       TestField::string("classification", "uav",                  true),
 *       TestField::real  ("bearing_deg",    45.0, 0.0, 360.0,      false),
 *       TestField::real  ("range_m",        120.0, 0.0, 50000.0,   false),
 *       TestField::string("track_id",       ""),
 *       TestField::choice("priority",       {"LOW","MEDIUM","HIGH"}, "HIGH"),
 *       TestField::flag  ("encrypted",      false),
 *   };
 *
 *   injector->setSchema("sentinel.detection", schema);
 *   extractor->setSchema("devana.threat",     schema);
 */

#pragma once

#include <climits>   // INT_MAX used in TestField::integer() default arg

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>

/**
 * @struct TestField
 * @brief Describes a single field in a test message schema
 */
struct TestField
{
    // ── Field types ──────────────────────────────────────────────────────────
    enum class Type
    {
        String,   ///< Free-text line edit
        Int,      ///< Integer spin box
        UInt64,   ///< uint64 — displayed as line edit (spinbox can't hold uint64)
        Real,     ///< Double spin box
        Bool,     ///< Checkbox
        Choice,   ///< Combo box (enum / fixed set of strings)
        ReadOnly, ///< Greyed-out label — for extractor-only display fields
    };

    QString     name;           ///< Field name (used as JSON key)
    Type        type;
    QVariant    defaultValue;   ///< Shown on injector, used for reset
    bool        required;       ///< Shows * suffix on label
    QStringList choices;        ///< Only for Type::Choice
    double      minVal = 0.0;   ///< Only for Type::Real / Type::Int
    double      maxVal = 1e9;   ///< Only for Type::Real / Type::Int
    int         decimals = 3;   ///< Decimal places for Type::Real
    QString     tooltip;        ///< Optional tooltip on the label

    // ── Convenience factories ─────────────────────────────────────────────
    static TestField string(const QString& name,
                            const QString& defaultVal = "",
                            bool required = false,
                            const QString& tooltip = "")
    {
        TestField f;
        f.name         = name;
        f.type         = Type::String;
        f.defaultValue = defaultVal;
        f.required     = required;
        f.tooltip      = tooltip;
        return f;
    }

    static TestField integer(const QString& name,
                             int defaultVal = 0,
                             int min = 0,
                             int max = INT_MAX,
                             bool required = false)
    {
        TestField f;
        f.name         = name;
        f.type         = Type::Int;
        f.defaultValue = defaultVal;
        f.minVal       = min;
        f.maxVal       = max;
        f.required     = required;
        return f;
    }

    static TestField uint64(const QString& name,
                            quint64 defaultVal = 0,
                            bool required = false)
    {
        TestField f;
        f.name         = name;
        f.type         = Type::UInt64;
        f.defaultValue = QVariant::fromValue(defaultVal);
        f.required     = required;
        return f;
    }

    static TestField real(const QString& name,
                          double defaultVal = 0.0,
                          double min = 0.0,
                          double max = 1e9,
                          bool required = false,
                          int decimals = 3)
    {
        TestField f;
        f.name         = name;
        f.type         = Type::Real;
        f.defaultValue = defaultVal;
        f.minVal       = min;
        f.maxVal       = max;
        f.decimals     = decimals;
        f.required     = required;
        return f;
    }

    static TestField flag(const QString& name,
                          bool defaultVal = false,
                          bool required = false)
    {
        TestField f;
        f.name         = name;
        f.type         = Type::Bool;
        f.defaultValue = defaultVal;
        f.required     = required;
        return f;
    }

    static TestField choice(const QString& name,
                            const QStringList& options,
                            const QString& defaultVal = "",
                            bool required = false)
    {
        TestField f;
        f.name         = name;
        f.type         = Type::Choice;
        f.choices      = options;
        f.defaultValue = defaultVal.isEmpty() ? options.value(0) : defaultVal;
        f.required     = required;
        return f;
    }

    static TestField readOnly(const QString& name, const QString& tooltip = "")
    {
        TestField f;
        f.name         = name;
        f.type         = Type::ReadOnly;
        f.required     = false;
        f.tooltip      = tooltip;
        return f;
    }
};

using TestSchema = QList<TestField>;
