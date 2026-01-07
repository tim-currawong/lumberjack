#include "color_family_manager.hpp"
#include "data_source.hpp"
#include "lumberjack_settings.hpp"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>

ColorFamilyManager* ColorFamilyManager::instance = nullptr;

ColorFamilyManager::ColorFamilyManager()
    : activeConfigurationName("")
    , plotBackgroundColor(0xF0, 0xF0, 0xF0)  // Default light gray
{
}

ColorFamilyManager::~ColorFamilyManager()
{
}

ColorFamilyManager* ColorFamilyManager::getInstance()
{
    if (instance == nullptr)
    {
        instance = new ColorFamilyManager();
        instance->loadFromSettings();
    }
    return instance;
}

QStringList ColorFamilyManager::getConfigurationNames() const
{
    return configurations.keys();
}

ColorFamilyConfiguration* ColorFamilyManager::getActiveConfiguration()
{
    if (activeConfigurationName.isEmpty())
    {
        return nullptr;
    }

    if (configurations.contains(activeConfigurationName))
    {
        return &configurations[activeConfigurationName];
    }

    return nullptr;
}

ColorFamilyConfiguration* ColorFamilyManager::getConfiguration(QString name)
{
    if (configurations.contains(name))
    {
        return &configurations[name];
    }
    return nullptr;
}

void ColorFamilyManager::setActiveConfiguration(QString name)
{
    if (name.isEmpty() || name == "(None)")
    {
        activeConfigurationName = "";
        emit activeConfigurationChanged("");
        emit configurationChanged();
        return;
    }

    if (configurations.contains(name))
    {
        activeConfigurationName = name;
        regenerateAllShades();
        emit activeConfigurationChanged(name);
        emit configurationChanged();
    }
}

void ColorFamilyManager::saveConfiguration(QString name, const ColorFamilyConfiguration& config)
{
    bool isNew = !configurations.contains(name);

    configurations[name] = config;

    if (isNew)
    {
        emit configurationsListChanged();
    }

    emit configurationChanged();
    saveToSettings();
}

void ColorFamilyManager::deleteConfiguration(QString name)
{
    if (configurations.contains(name))
    {
        configurations.remove(name);

        if (activeConfigurationName == name)
        {
            activeConfigurationName = "";
            emit activeConfigurationChanged("");
        }

        emit configurationsListChanged();
        emit configurationChanged();
        saveToSettings();
    }
}

void ColorFamilyManager::renameConfiguration(QString oldName, QString newName)
{
    if (configurations.contains(oldName) && !configurations.contains(newName))
    {
        ColorFamilyConfiguration config = configurations[oldName];
        config.setName(newName);

        configurations.remove(oldName);
        configurations[newName] = config;

        if (activeConfigurationName == oldName)
        {
            activeConfigurationName = newName;
            emit activeConfigurationChanged(newName);
        }

        emit configurationsListChanged();
        emit configurationChanged();
        saveToSettings();
    }
}

bool ColorFamilyManager::hasConfiguration(QString name) const
{
    return configurations.contains(name);
}

ColorFamily* ColorFamilyManager::matchSeries(QString label)
{
    ColorFamilyConfiguration* config = getActiveConfiguration();
    if (config)
    {
        return config->matchSeries(label);
    }
    return nullptr;
}

QColor ColorFamilyManager::assignColor(QString label, DataSource* source, int seriesIndexInFamily)
{
    ColorFamily* family = matchSeries(label);

    if (family)
    {
        // Ensure shades are generated
        if (family->getShades().isEmpty())
        {
            family->generateShades(plotBackgroundColor);
        }

        return family->getShadeForSeries(source, seriesIndexInFamily);
    }

    // No match - return color from default palette
    ColorFamilyConfiguration* config = getActiveConfiguration();
    QString paletteName = config ? config->getDefaultPaletteName() : "Tableau 10";

    QList<QColor> palette = getDefaultPaletteColors(paletteName);

    if (palette.isEmpty())
    {
        return QColor(0, 0, 0);  // Fallback to black
    }

    // Use series index to select from palette
    int paletteIndex = seriesIndexInFamily % palette.count();
    return palette.at(paletteIndex);
}

QList<QColor> ColorFamilyManager::getDefaultPaletteColors(QString paletteName)
{
    if (paletteName.isEmpty())
    {
        ColorFamilyConfiguration* config = getActiveConfiguration();
        paletteName = config ? config->getDefaultPaletteName() : "Tableau 10";
    }

    if (paletteName == "Tableau 10")
    {
        return getTableau10Palette();
    }
    else if (paletteName == "ColorBrewer Set1")
    {
        return getColorBrewerSet1Palette();
    }

    // Default to Tableau 10
    return getTableau10Palette();
}

void ColorFamilyManager::setPlotBackgroundColor(QColor color)
{
    plotBackgroundColor = color;
    regenerateAllShades();
}

void ColorFamilyManager::regenerateAllShades()
{
    ColorFamilyConfiguration* config = getActiveConfiguration();
    if (config)
    {
        QList<ColorFamily> families = config->getFamilies();
        for (int i = 0; i < families.count(); i++)
        {
            ColorFamily* family = config->getFamilyAt(i);
            if (family)
            {
                family->generateShades(plotBackgroundColor);
            }
        }
    }
}

QList<QColor> ColorFamilyManager::getTableau10Palette() const
{
    QList<QColor> colors;

    // Tableau 10 palette - scientifically designed for visibility
    colors.append(QColor(31, 119, 180));   // Blue
    colors.append(QColor(255, 127, 14));   // Orange
    colors.append(QColor(44, 160, 44));    // Green
    colors.append(QColor(214, 39, 40));    // Red
    colors.append(QColor(148, 103, 189));  // Purple
    colors.append(QColor(140, 86, 75));    // Brown
    colors.append(QColor(227, 119, 194));  // Pink
    colors.append(QColor(127, 127, 127));  // Gray
    colors.append(QColor(188, 189, 34));   // Olive
    colors.append(QColor(23, 190, 207));   // Cyan

    return colors;
}

QList<QColor> ColorFamilyManager::getColorBrewerSet1Palette() const
{
    QList<QColor> colors;

    // ColorBrewer Set1 - qualitative palette
    colors.append(QColor(228, 26, 28));    // Red
    colors.append(QColor(55, 126, 184));   // Blue
    colors.append(QColor(77, 175, 74));    // Green
    colors.append(QColor(152, 78, 163));   // Purple
    colors.append(QColor(255, 127, 0));    // Orange
    colors.append(QColor(255, 255, 51));   // Yellow
    colors.append(QColor(166, 86, 40));    // Brown
    colors.append(QColor(247, 129, 191));  // Pink
    colors.append(QColor(153, 153, 153));  // Gray

    return colors;
}

void ColorFamilyManager::saveToSettings()
{
    LumberjackSettings* settings = LumberjackSettings::getInstance();

    // Save active configuration name
    settings->saveSetting("colorFamilies", "activeConfiguration", activeConfigurationName);

    // Save all configurations as JSON
    QJsonArray configurationsArray;
    foreach (QString name, configurations.keys())
    {
        ColorFamilyConfiguration config = configurations[name];
        configurationsArray.append(config.toJson());
    }

    QJsonDocument doc(configurationsArray);
    settings->saveSetting("colorFamilies", "configurations", QString(doc.toJson(QJsonDocument::Compact)));
}

void ColorFamilyManager::loadFromSettings()
{
    LumberjackSettings* settings = LumberjackSettings::getInstance();

    // Load active configuration name
    activeConfigurationName = settings->loadSetting("colorFamilies", "activeConfiguration", "").toString();

    // Load all configurations
    QString configurationsJson = settings->loadSetting("colorFamilies", "configurations", "[]").toString();

    QJsonDocument doc = QJsonDocument::fromJson(configurationsJson.toUtf8());
    if (doc.isArray())
    {
        QJsonArray configurationsArray = doc.array();
        for (int i = 0; i < configurationsArray.size(); i++)
        {
            ColorFamilyConfiguration config = ColorFamilyConfiguration::fromJson(configurationsArray[i].toObject());
            configurations[config.getName()] = config;
        }
    }

    // Regenerate shades for active configuration
    if (!activeConfigurationName.isEmpty())
    {
        regenerateAllShades();
    }
}
