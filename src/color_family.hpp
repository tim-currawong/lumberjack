#ifndef COLOR_FAMILY_HPP
#define COLOR_FAMILY_HPP

#include <QObject>
#include <QColor>
#include <QString>
#include <QStringList>
#include <QList>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

/**
 * @brief The ColorFamily class represents a group of related colors
 * with pattern-based automatic assignment to data series
 */
class ColorFamily
{
public:
    ColorFamily();
    ColorFamily(QString name, QColor baseHue, QStringList patterns = QStringList());
    ColorFamily(const ColorFamily& other);
    ColorFamily& operator=(const ColorFamily& other);

    // Getters
    QString getId() const { return id; }
    QString getName() const { return name; }
    QColor getBaseHue() const { return baseHue; }
    QStringList getPatterns() const { return patterns; }
    QList<QColor> getShades() const { return shades; }

    // Setters
    void setName(QString n) { name = n; }
    void setBaseHue(QColor color);
    void setPatterns(QStringList p) { patterns = p; }

    // Pattern matching
    bool matchesPattern(QString label) const;

    // Shade management
    void generateShades(QColor backgroundColor);
    QColor getShadeForSeries(int seriesIndexInFamily);

    // Serialization
    QJsonObject toJson() const;
    static ColorFamily fromJson(const QJsonObject& obj);

private:
    QString id;                              // Unique identifier (UUID)
    QString name;                            // Display name (e.g., "Current Readings")
    QColor baseHue;                          // Base color for this family
    QStringList patterns;                    // Wildcard patterns for matching
    QList<QColor> shades;                    // Generated color shades

    // Helper methods
    double contrastRatio(QColor c1, QColor c2) const;
    void ensureMinimumContrast(QColor& color, QColor background, double minRatio = 4.5);
};

#endif // COLOR_FAMILY_HPP
