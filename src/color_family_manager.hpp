#ifndef COLOR_FAMILY_MANAGER_HPP
#define COLOR_FAMILY_MANAGER_HPP

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QColor>
#include "color_family_configuration.hpp"

/**
 * @brief The ColorFamilyManager class manages multiple color family configurations
 * and handles color assignment for data series
 */
class ColorFamilyManager : public QObject
{
    Q_OBJECT

public:
    static ColorFamilyManager* getInstance();

    // Configuration management
    QStringList getConfigurationNames() const;
    ColorFamilyConfiguration* getActiveConfiguration();
    ColorFamilyConfiguration* getConfiguration(QString name);
    QString getActiveConfigurationName() const { return activeConfigurationName; }

    void setActiveConfiguration(QString name);
    void saveConfiguration(QString name, const ColorFamilyConfiguration& config);
    void deleteConfiguration(QString name);
    void renameConfiguration(QString oldName, QString newName);
    bool hasConfiguration(QString name) const;

    // Color assignment (delegates to active configuration)
    ColorFamily* matchSeries(QString label);
    QColor assignColor(ColorFamily* family, int seriesIndexInFamily);

    // Default palette colors (for series that don't match any family)
    QList<QColor> getDefaultPaletteColors(QString paletteName = "");

    // Background color management
    void setPlotBackgroundColor(QColor color);
    QColor getPlotBackgroundColor() const { return plotBackgroundColor; }

    // Generate shades for all families based on current background
    void regenerateAllShades();

    // Re-apply color families to all existing data sources
    void reapplyAllColorFamilies();

    // Persistence
    void saveToSettings();
    void loadFromSettings();

signals:
    void configurationChanged();
    void activeConfigurationChanged(QString name);
    void configurationsListChanged();

private:
    ColorFamilyManager();
    ~ColorFamilyManager();

    // Singleton instance
    static ColorFamilyManager* instance;

    // Data
    // Each configuration is heap-allocated so that ColorFamilyConfiguration*
    // pointers returned by getActiveConfiguration()/getConfiguration() stay
    // valid even if the QMap itself reallocates/detaches.
    QMap<QString, QSharedPointer<ColorFamilyConfiguration>> configurations;
    QString activeConfigurationName;
    QColor plotBackgroundColor;

    // Helper methods
    QList<QColor> getTableau10Palette() const;
    QList<QColor> getColorBrewerSet1Palette() const;
};

#endif // COLOR_FAMILY_MANAGER_HPP
