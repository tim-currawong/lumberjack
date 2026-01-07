#include "color_family_configuration.hpp"
#include <QJsonArray>

ColorFamilyConfiguration::ColorFamilyConfiguration()
    : name("Untitled Configuration")
    , defaultPaletteName("Tableau 10")
{
}

ColorFamilyConfiguration::ColorFamilyConfiguration(QString name)
    : name(name)
    , defaultPaletteName("Tableau 10")
{
}

ColorFamilyConfiguration::ColorFamilyConfiguration(const ColorFamilyConfiguration& other)
    : name(other.name)
    , families(other.families)
    , defaultPaletteName(other.defaultPaletteName)
{
}

ColorFamilyConfiguration& ColorFamilyConfiguration::operator=(const ColorFamilyConfiguration& other)
{
    if (this != &other)
    {
        name = other.name;
        families = other.families;
        defaultPaletteName = other.defaultPaletteName;
    }
    return *this;
}

void ColorFamilyConfiguration::addFamily(const ColorFamily& family)
{
    families.append(family);
}

void ColorFamilyConfiguration::removeFamily(int index)
{
    if (index >= 0 && index < families.count())
    {
        families.removeAt(index);
    }
}

void ColorFamilyConfiguration::reorderFamily(int fromIndex, int toIndex)
{
    if (fromIndex >= 0 && fromIndex < families.count() &&
        toIndex >= 0 && toIndex < families.count() &&
        fromIndex != toIndex)
    {
        families.move(fromIndex, toIndex);
    }
}

ColorFamily* ColorFamilyConfiguration::getFamilyAt(int index)
{
    if (index >= 0 && index < families.count())
    {
        return &families[index];
    }
    return nullptr;
}

ColorFamily* ColorFamilyConfiguration::getFamilyById(QString id)
{
    for (int i = 0; i < families.count(); i++)
    {
        if (families[i].getId() == id)
        {
            return &families[i];
        }
    }
    return nullptr;
}

ColorFamily* ColorFamilyConfiguration::matchSeries(QString label)
{
    // Iterate families in order, return first match
    for (int i = 0; i < families.count(); i++)
    {
        if (families[i].matchesPattern(label))
        {
            return &families[i];
        }
    }

    return nullptr;  // No match found
}

QJsonObject ColorFamilyConfiguration::toJson() const
{
    QJsonObject obj;

    obj["name"] = name;
    obj["defaultPalette"] = defaultPaletteName;

    QJsonArray familiesArray;
    foreach (const ColorFamily& family, families)
    {
        familiesArray.append(family.toJson());
    }
    obj["families"] = familiesArray;

    return obj;
}

ColorFamilyConfiguration ColorFamilyConfiguration::fromJson(const QJsonObject& obj)
{
    ColorFamilyConfiguration config;

    config.name = obj["name"].toString();
    config.defaultPaletteName = obj["defaultPalette"].toString("Tableau 10");

    QJsonArray familiesArray = obj["families"].toArray();
    for (int i = 0; i < familiesArray.size(); i++)
    {
        ColorFamily family = ColorFamily::fromJson(familiesArray[i].toObject());
        config.families.append(family);
    }

    return config;
}
