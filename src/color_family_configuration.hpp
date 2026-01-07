#ifndef COLOR_FAMILY_CONFIGURATION_HPP
#define COLOR_FAMILY_CONFIGURATION_HPP

#include <QString>
#include <QList>
#include <QJsonObject>
#include "color_family.hpp"

/**
 * @brief The ColorFamilyConfiguration class represents a named collection
 * of color families with their patterns and settings
 */
class ColorFamilyConfiguration
{
public:
    ColorFamilyConfiguration();
    ColorFamilyConfiguration(QString name);
    ColorFamilyConfiguration(const ColorFamilyConfiguration& other);
    ColorFamilyConfiguration& operator=(const ColorFamilyConfiguration& other);

    // Getters
    QString getName() const { return name; }
    QList<ColorFamily> getFamilies() const { return families; }
    QString getDefaultPaletteName() const { return defaultPaletteName; }

    // Setters
    void setName(QString n) { name = n; }
    void setDefaultPaletteName(QString paletteName) { defaultPaletteName = paletteName; }

    // Family management
    void addFamily(const ColorFamily& family);
    void removeFamily(int index);
    void replaceFamilies(const QList<ColorFamily>& newFamilies) { families = newFamilies; }
    void reorderFamily(int fromIndex, int toIndex);
    ColorFamily* getFamilyAt(int index);
    ColorFamily* getFamilyById(QString id);
    int getFamilyCount() const { return families.count(); }

    // Pattern matching
    ColorFamily* matchSeries(QString label);

    // Serialization
    QJsonObject toJson() const;
    static ColorFamilyConfiguration fromJson(const QJsonObject& obj);

private:
    QString name;                    // Configuration name (e.g., "Electrical Analysis")
    QList<ColorFamily> families;     // List of color families
    QString defaultPaletteName;      // Name of fallback palette for unmatched series
};

#endif // COLOR_FAMILY_CONFIGURATION_HPP
