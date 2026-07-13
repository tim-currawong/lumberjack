#include "color_family.hpp"
#include <QtMath>
#include <QJsonDocument>

ColorFamily::ColorFamily()
    : id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , name("Unnamed Family")
    , baseHue(Qt::blue)
{
}

ColorFamily::ColorFamily(QString name, QColor baseHue, QStringList patterns)
    : id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , name(name)
    , baseHue(baseHue)
    , patterns(patterns)
{
}

ColorFamily::ColorFamily(const ColorFamily& other)
    : id(other.id)
    , name(other.name)
    , baseHue(other.baseHue)
    , patterns(other.patterns)
    , shades(other.shades)
{
}

ColorFamily& ColorFamily::operator=(const ColorFamily& other)
{
    if (this != &other)
    {
        id = other.id;
        name = other.name;
        baseHue = other.baseHue;
        patterns = other.patterns;
        shades = other.shades;
    }
    return *this;
}

void ColorFamily::setBaseHue(QColor color)
{
    baseHue = color;
    // Clear shades so they'll be regenerated with new base color
    shades.clear();
}

bool ColorFamily::matchesPattern(QString label) const
{
    // Try to match against each pattern
    foreach (QString pattern, patterns)
    {
        if (pattern.isEmpty())
            continue;

        // Convert wildcard pattern to regex
        QString regexPattern = QRegularExpression::wildcardToRegularExpression(pattern);
        QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);

        if (regex.match(label).hasMatch())
        {
            return true;
        }
    }

    return false;
}

void ColorFamily::generateShades(QColor backgroundColor)
{
    shades.clear();

    if (!baseHue.isValid())
    {
        baseHue = Qt::blue;
    }

    // Convert base color to HSL for manipulation
    int h = baseHue.hue();
    int s = baseHue.saturation();
    int l = baseHue.lightness();

    // Determine if background is light or dark
    bool lightBg = backgroundColor.lightness() > 128;

    if (lightBg)
    {
        // For light backgrounds: generate medium to darker shades, anchored on
        // the base color's own lightness (offsets chosen to reproduce the prior
        // fixed ladder of 140/110/80/60 at l = 128, the lightness of a typical
        // fully-saturated hue) so that very dark or very light base picks (e.g.
        // black) are actually respected instead of being pulled into a fixed
        // mid-range band.
        //
        // The ladder is shifted as a whole to stay in [0, 255], rather than
        // clamping each stop independently -- clamping each stop collapses
        // several of them onto the same boundary value (e.g. a black base
        // would clamp 3 of the 4 stops to 0), while shifting preserves the
        // spacing between stops so all four stay distinguishable.
        int mediumLightL = l + 12;
        int darkL = l - 68;

        if (darkL < 0)
        {
            mediumLightL += -darkL;
            darkL = 0;
        }
        else if (mediumLightL > 255)
        {
            darkL -= (mediumLightL - 255);
            mediumLightL = 255;
        }

        int mediumL = mediumLightL - 30;
        int mediumDarkL = mediumLightL - 60;

        QColor mediumLight = QColor::fromHsl(h, 200, mediumLightL);  // Medium-light, saturated
        QColor medium = QColor::fromHsl(h, 210, mediumL);            // Medium, more saturated
        QColor mediumDark = QColor::fromHsl(h, 220, mediumDarkL);    // Medium-dark, highly saturated
        QColor dark = QColor::fromHsl(h, 230, darkL);                // Dark, very saturated

        // Ensure minimum contrast
        ensureMinimumContrast(mediumLight, backgroundColor);
        ensureMinimumContrast(medium, backgroundColor);
        ensureMinimumContrast(mediumDark, backgroundColor);
        ensureMinimumContrast(dark, backgroundColor);

        shades.append(mediumLight);
        shades.append(medium);
        shades.append(mediumDark);
        shades.append(dark);
    }
    else
    {
        // For dark backgrounds: generate light to bright shades
        QColor veryLight = QColor::fromHsl(h, qRound(s * 0.6), qRound(l * 0.85));
        QColor light = QColor::fromHsl(h, qRound(s * 0.7), qRound(l * 0.70));
        QColor medium = QColor::fromHsl(h, qRound(s * 0.8), qRound(l * 0.55));
        QColor mediumDark = QColor::fromHsl(h, qRound(s * 0.9), qRound(l * 0.45));

        // Ensure minimum contrast
        ensureMinimumContrast(veryLight, backgroundColor);
        ensureMinimumContrast(light, backgroundColor);
        ensureMinimumContrast(medium, backgroundColor);
        ensureMinimumContrast(mediumDark, backgroundColor);

        shades.append(veryLight);
        shades.append(light);
        shades.append(medium);
        shades.append(mediumDark);
    }

    // Contrast enforcement above can push different starting shades toward the
    // same lightness floor/ceiling, making them collapse into near-duplicates
    // for highly saturated base hues. Nudge hue apart to keep shades distinct.
    separateSimilarShades(backgroundColor);
}

void ColorFamily::separateSimilarShades(QColor backgroundColor)
{
    const int minLightnessSeparation = 12;
    const int hueSeparationThreshold = 18;
    const int hueStep = 6;
    const int maxNudgesPerPair = 8;

    auto hueDistance = [](int h1, int h2) -> int {
        if (h1 < 0 || h2 < 0)
            return 0;  // achromatic color; hue is meaningless
        int diff = qAbs(h1 - h2) % 360;
        return qMin(diff, 360 - diff);
    };

    for (int i = 1; i < shades.count(); i++)
    {
        for (int attempt = 0; attempt < maxNudgesPerPair; attempt++)
        {
            int lightnessDiff = qAbs(shades[i].lightness() - shades[i - 1].lightness());
            int hueDiff = hueDistance(shades[i].hue(), shades[i - 1].hue());

            if (lightnessDiff >= minLightnessSeparation || hueDiff >= hueSeparationThreshold)
                break;

            int newHue = (shades[i].hue() + hueStep + 360) % 360;
            QColor nudged = QColor::fromHsl(newHue, shades[i].saturation(), shades[i].lightness());
            ensureMinimumContrast(nudged, backgroundColor);
            shades[i] = nudged;
        }
    }
}

QColor ColorFamily::getShadeForSeries(int seriesIndexInFamily)
{
    // If no shades generated, return base color
    if (shades.isEmpty())
    {
        return baseHue;
    }

    // Each series in the family gets a different shade
    // seriesIndexInFamily = 0, 1, 2, ... for first, second, third series in the family
    int shadeIndex = seriesIndexInFamily % shades.count();
    return shades.at(shadeIndex);
}

double ColorFamily::contrastRatio(QColor c1, QColor c2) const
{
    // Calculate relative luminance (WCAG formula)
    auto luminance = [](QColor c) -> double {
        double r = c.redF();
        double g = c.greenF();
        double b = c.blueF();

        // Apply gamma correction
        r = (r <= 0.03928) ? r / 12.92 : qPow((r + 0.055) / 1.055, 2.4);
        g = (g <= 0.03928) ? g / 12.92 : qPow((g + 0.055) / 1.055, 2.4);
        b = (b <= 0.03928) ? b / 12.92 : qPow((b + 0.055) / 1.055, 2.4);

        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    };

    double l1 = luminance(c1);
    double l2 = luminance(c2);

    // Ensure l1 is the lighter color
    if (l1 < l2)
    {
        double temp = l1;
        l1 = l2;
        l2 = temp;
    }

    return (l1 + 0.05) / (l2 + 0.05);
}

void ColorFamily::ensureMinimumContrast(QColor& color, QColor background, double minRatio)
{
    int maxIterations = 50;
    int iteration = 0;

    bool lightBg = background.lightness() > 128;

    while (contrastRatio(color, background) < minRatio && iteration < maxIterations)
    {
        int l = color.lightness();

        if (lightBg)
        {
            // Darken the color
            l = qMax(0, l - 5);
        }
        else
        {
            // Lighten the color
            l = qMin(255, l + 5);
        }

        color.setHsl(color.hue(), color.saturation(), l);
        iteration++;
    }
}

QJsonObject ColorFamily::toJson() const
{
    QJsonObject obj;

    obj["id"] = id;
    obj["name"] = name;
    obj["baseColor"] = baseHue.name(QColor::HexArgb);

    QJsonArray patternsArray;
    foreach (QString pattern, patterns)
    {
        patternsArray.append(pattern);
    }
    obj["patterns"] = patternsArray;

    return obj;
}

ColorFamily ColorFamily::fromJson(const QJsonObject& obj)
{
    ColorFamily family;

    family.id = obj["id"].toString();
    family.name = obj["name"].toString();
    family.baseHue = QColor(obj["baseColor"].toString());

    QJsonArray patternsArray = obj["patterns"].toArray();
    QStringList patternsList;
    for (int i = 0; i < patternsArray.size(); i++)
    {
        patternsList.append(patternsArray[i].toString());
    }
    family.patterns = patternsList;

    return family;
}
