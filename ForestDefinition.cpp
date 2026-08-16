#include "ForestDefinition.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QRegularExpression>

#include <cmath>

namespace {

bool finiteNumber(const QJsonValue &value, double &result) {
    if(!value.isDouble())
        return false;
    result = value.toDouble();
    return std::isfinite(result);
}

bool requiredNumber(
    const QJsonObject &object,
    const QString &key,
    const QString &context,
    double &result,
    QStringList &errors) {
    if(finiteNumber(object.value(key), result))
        return true;
    errors.append(context + ": '" + key + "' must be a finite number.");
    return false;
}

bool requiredRange(
    const QJsonObject &object,
    const QString &key,
    const QString &context,
    ForestNumberRange &result,
    QStringList &errors,
    bool positive) {
    const QJsonArray values = object.value(key).toArray();
    if(values.size() != 2
            || !finiteNumber(values.at(0), result.minimum)
            || !finiteNumber(values.at(1), result.maximum)) {
        errors.append(context + ": '" + key
            + "' must contain exactly two finite numbers.");
        return false;
    }
    if(result.minimum > result.maximum) {
        errors.append(context + ": '" + key + "' minimum exceeds maximum.");
        return false;
    }
    if(positive && result.minimum <= 0.0) {
        errors.append(context + ": '" + key + "' values must be positive.");
        return false;
    }
    return true;
}

QString requiredString(
    const QJsonObject &object,
    const QString &key,
    const QString &context,
    QStringList &errors) {
    const QString result = object.value(key).toString().trimmed();
    if(result.isEmpty())
        errors.append(context + ": '" + key + "' must be a non-empty string.");
    return result;
}

void warnUnknown(
    const QJsonObject &object,
    const QSet<QString> &known,
    const QString &context,
    QStringList &warnings) {
    for(auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if(!known.contains(it.key()))
            warnings.append(context + ": unknown field '" + it.key() + "'.");
    }
}

bool validId(const QString &id) {
    static const QRegularExpression expression(
        QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]*$"));
    return expression.match(id).hasMatch();
}

void validateDefaultInsideRange(
    double value,
    const ForestNumberRange &range,
    const QString &label,
    const QString &context,
    QStringList &errors) {
    if(value < range.minimum || value > range.maximum)
        errors.append(context + ": default '" + label + "' is outside its limits.");
}

bool parseOsm(
    const QJsonObject &object,
    ForestRecipeDefinition &recipe,
    const QString &context,
    QStringList &errors,
    QStringList &warnings) {
    if(object.isEmpty())
        return true;
    warnUnknown(object, {"priority", "categories", "matchAny"}, context, warnings);

    double priority = 0.0;
    if(!requiredNumber(object, "priority", context, priority, errors)
            || priority < 0.0 || std::floor(priority) != priority) {
        errors.append(context + ": 'priority' must be a non-negative integer.");
        return false;
    }
    recipe.osmPriority = static_cast<int>(priority);

    const QJsonArray categories = object.value("categories").toArray();
    const QSet<QString> supportedCategories {
        "woodland", "scrub", "heath", "grassland", "wetland", "agriculture",
        "orchard", "parkland", "golf_course", "cemetery", "sports", "zoo"
    };
    for(const QJsonValue &value : categories) {
        const QString category = value.toString().trimmed().toLower();
        if(category.isEmpty()) {
            errors.append(context + ": 'categories' must contain non-empty strings.");
            continue;
        }
        if(!supportedCategories.contains(category)) {
            errors.append(context + ": unsupported PolyVeg category '"
                + category + "'.");
            continue;
        }
        if(!recipe.osmCategories.contains(category))
            recipe.osmCategories.append(category);
    }

    const QJsonArray rules = object.value("matchAny").toArray();
    if(rules.isEmpty() && recipe.osmCategories.isEmpty()) {
        errors.append(context
            + ": provide at least one 'categories' entry or 'matchAny' rule.");
        return false;
    }

    for(int index = 0; index < rules.size(); ++index) {
        const QString ruleContext = context + QString(".matchAny[%1]").arg(index);
        if(!rules.at(index).isObject() || rules.at(index).toObject().isEmpty()) {
            errors.append(ruleContext + ": rule must be a non-empty object.");
            continue;
        }
        ForestOsmMatchRule rule;
        const QJsonObject ruleObject = rules.at(index).toObject();
        for(auto it = ruleObject.constBegin(); it != ruleObject.constEnd(); ++it) {
            const QString tag = it.key().trimmed().toLower();
            const QJsonArray values = it.value().toArray();
            QStringList permitted;
            for(const QJsonValue &value : values) {
                const QString text = value.toString().trimmed().toLower();
                if(!text.isEmpty() && !permitted.contains(text))
                    permitted.append(text);
            }
            if(tag.isEmpty() || permitted.isEmpty())
                errors.append(ruleContext + ": every tag requires string values.");
            else
                rule.tags.insert(tag, permitted);
        }
        if(!rule.tags.isEmpty())
            recipe.osmMatchAny.append(rule);
    }
    return !recipe.osmCategories.isEmpty() || !recipe.osmMatchAny.isEmpty();
}

bool parseVegetation(
    const QJsonObject &object,
    ForestVegetationDefinition &entry,
    const QString &shapesPath,
    const QString &context,
    QStringList &errors,
    QStringList &warnings) {
    warnUnknown(object,
        {"id", "name", "shape", "stratum", "proportion", "yawDegrees",
         "uniformScale", "plantingDepthMetres", "footprintRadiusMetres",
         "maximumSlopeDegrees"},
        context, warnings);

    entry.id = requiredString(object, "id", context, errors);
    entry.name = requiredString(object, "name", context, errors);
    entry.shape = requiredString(object, "shape", context, errors);
    entry.stratum = requiredString(object, "stratum", context, errors).toLower();
    if(!validId(entry.id))
        errors.append(context + ": 'id' contains unsupported characters.");
    if(!QSet<QString>({"canopy", "understory", "ground", "edge"})
            .contains(entry.stratum))
        errors.append(context + ": unsupported stratum '" + entry.stratum + "'.");

    requiredNumber(object, "proportion", context, entry.proportion, errors);
    if(entry.proportion <= 0.0)
        errors.append(context + ": 'proportion' must be positive.");
    requiredRange(object, "yawDegrees", context, entry.yawDegrees, errors, false);
    requiredRange(object, "uniformScale", context, entry.uniformScale, errors, true);
    requiredNumber(object, "footprintRadiusMetres", context,
        entry.footprintRadiusMetres, errors);
    if(entry.footprintRadiusMetres <= 0.0)
        errors.append(context + ": 'footprintRadiusMetres' must be positive.");

    if(object.contains("plantingDepthMetres")) {
        entry.hasPlantingDepth = requiredNumber(object, "plantingDepthMetres",
            context, entry.plantingDepthMetres, errors);
        if(entry.hasPlantingDepth && entry.plantingDepthMetres < 0.0)
            errors.append(context + ": 'plantingDepthMetres' cannot be negative.");
    }
    if(object.contains("maximumSlopeDegrees")) {
        entry.hasMaximumSlope = requiredNumber(object, "maximumSlopeDegrees",
            context, entry.maximumSlopeDegrees, errors);
        if(entry.hasMaximumSlope
                && (entry.maximumSlopeDegrees < 0.0
                    || entry.maximumSlopeDegrees > 90.0))
            errors.append(context + ": 'maximumSlopeDegrees' must be in 0..90.");
    }

    const QFileInfo shapeInfo(entry.shape);
    if(shapeInfo.fileName() != entry.shape
            || entry.shape.contains('/') || entry.shape.contains('\\')
            || shapeInfo.suffix().compare("s", Qt::CaseInsensitive) != 0) {
        errors.append(context + ": 'shape' must be a safe .s basename.");
    } else if(!QFileInfo::exists(shapesPath + "/" + entry.shape)) {
        errors.append(context + ": shape not found in route Shapes: " + entry.shape);
    }
    return true;
}

} // namespace

ForestCatalogLoadResult ForestDefinitionLoader::loadRoute(const QString &routePath) {
    return loadFile(routePath + "/OpenRails/polyveg.json",
                    routePath + "/shapes");
}

ForestCatalogLoadResult ForestDefinitionLoader::loadFile(
    const QString &filePath,
    const QString &routeShapesPath) {
    ForestCatalogLoadResult result;
    result.filePath = QFileInfo(filePath).absoluteFilePath();

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)) {
        result.errors.append("Unable to open forest catalog: " + result.filePath);
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError) {
        result.errors.append(QString("JSON parse error at offset %1: %2")
            .arg(parseError.offset).arg(parseError.errorString()));
        return result;
    }
    if(!document.isObject()) {
        result.errors.append("polyveg.json root must be an object.");
        return result;
    }

    const QJsonObject root = document.object();
    warnUnknown(root, {"schemaVersion", "polyVeg"}, "root", result.warnings);
    double schemaVersion = 0.0;
    if(!requiredNumber(root, "schemaVersion", "root", schemaVersion, result.errors)
            || schemaVersion != 1.0) {
        result.errors.append("root: supported 'schemaVersion' is 1.");
        return result;
    }
    result.catalog.schemaVersion = 1;

    QJsonArray recipes = root.value("polyVeg").toArray();
    if(recipes.isEmpty()) {
        result.errors.append("root: 'polyVeg' must contain at least one recipe.");
        return result;
    }

    QSet<QString> recipeIds;
    for(int index = 0; index < recipes.size(); ++index) {
        const QString context = QString("polyVeg[%1]").arg(index);
        if(!recipes.at(index).isObject()) {
            result.errors.append(context + ": recipe must be an object.");
            continue;
        }
        const QJsonObject object = recipes.at(index).toObject();
        warnUnknown(object,
            {"id", "name", "description", "defaults", "limits",
             "distribution", "osm", "vegetation"},
            context, result.warnings);

        ForestRecipeDefinition recipe;
        recipe.id = requiredString(object, "id", context, result.errors);
        recipe.name = requiredString(object, "name", context, result.errors);
        recipe.description = object.value("description").toString();
        if(!validId(recipe.id))
            result.errors.append(context + ": 'id' contains unsupported characters.");
        const QString foldedId = recipe.id.toCaseFolded();
        if(recipeIds.contains(foldedId))
            result.errors.append(context + ": duplicate recipe id '" + recipe.id + "'.");
        recipeIds.insert(foldedId);

        const QJsonObject defaults = object.value("defaults").toObject();
        const QString defaultsContext = context + ".defaults";
        warnUnknown(defaults,
            {"widthMetres", "densityPerSquareKilometre", "terrainDepthMetres",
             "variationScale", "trackClearanceMetres", "roadClearanceMetres",
             "maximumTrees"},
            defaultsContext, result.warnings);
        requiredNumber(defaults, "widthMetres", defaultsContext,
            recipe.defaultWidthMetres, result.errors);
        requiredNumber(defaults, "densityPerSquareKilometre", defaultsContext,
            recipe.defaultDensityPerSquareMetre, result.errors);
        recipe.defaultDensityPerSquareMetre /= 1000000.0;
        requiredNumber(defaults, "terrainDepthMetres", defaultsContext,
            recipe.defaultTerrainDepthMetres, result.errors);
        requiredNumber(defaults, "variationScale", defaultsContext,
            recipe.defaultVariationScale, result.errors);
        requiredNumber(defaults, "trackClearanceMetres", defaultsContext,
            recipe.defaultTrackClearanceMetres, result.errors);
        requiredNumber(defaults, "roadClearanceMetres", defaultsContext,
            recipe.defaultRoadClearanceMetres, result.errors);
        double defaultMaximumTrees = 0.0;
        if(defaults.contains("maximumTrees"))
            requiredNumber(defaults, "maximumTrees", defaultsContext,
                defaultMaximumTrees, result.errors);
        if(recipe.defaultWidthMetres <= 0.0
                || recipe.defaultDensityPerSquareMetre <= 0.0
                || recipe.defaultTerrainDepthMetres < 0.0
                || recipe.defaultVariationScale < 0.0
                || recipe.defaultVariationScale > 1.0
                || recipe.defaultTrackClearanceMetres < 0.0
                || recipe.defaultRoadClearanceMetres < 0.0)
            result.errors.append(defaultsContext + ": defaults are outside safe ranges.");

        const QJsonObject limits = object.value("limits").toObject();
        const QString limitsContext = context + ".limits";
        warnUnknown(limits,
            {"widthMetres", "densityPerSquareKilometre", "maximumTrees"},
            limitsContext, result.warnings);
        requiredRange(limits, "widthMetres", limitsContext,
            recipe.widthLimitsMetres, result.errors, true);
        requiredRange(limits, "densityPerSquareKilometre", limitsContext,
            recipe.densityLimitsPerSquareMetre, result.errors, true);
        recipe.densityLimitsPerSquareMetre.minimum /= 1000000.0;
        recipe.densityLimitsPerSquareMetre.maximum /= 1000000.0;
        ForestNumberRange maximumTreesRange;
        requiredRange(limits, "maximumTrees", limitsContext,
            maximumTreesRange, result.errors, true);
        recipe.defaultMaximumTrees = static_cast<int>(defaultMaximumTrees);
        if(maximumTreesRange.maximum > 10000000.0
                || std::floor(maximumTreesRange.minimum) != maximumTreesRange.minimum
                || std::floor(maximumTreesRange.maximum) != maximumTreesRange.maximum)
            result.errors.append(limitsContext
                + ": 'maximumTrees' must contain integers in 1..10000000.");
        recipe.minimumMaximumTrees = static_cast<int>(maximumTreesRange.minimum);
        recipe.maximumMaximumTrees = static_cast<int>(maximumTreesRange.maximum);
        if(recipe.defaultMaximumTrees < recipe.minimumMaximumTrees
                || recipe.defaultMaximumTrees > recipe.maximumMaximumTrees)
            result.errors.append(context
                + ": default 'maximumTrees' is outside its limits.");
        validateDefaultInsideRange(recipe.defaultWidthMetres,
            recipe.widthLimitsMetres, "widthMetres", context, result.errors);
        validateDefaultInsideRange(recipe.defaultDensityPerSquareMetre,
            recipe.densityLimitsPerSquareMetre, "densityPerSquareKilometre",
            context, result.errors);

        const QJsonObject distribution = object.value("distribution").toObject();
        const QString distributionContext = context + ".distribution";
        warnUnknown(distribution,
            {"maximumSlopeDegrees", "edgeFeatherMetres",
             "minimumSeparationMetres", "preventFootprintOverlap"},
            distributionContext, result.warnings);
        requiredNumber(distribution, "maximumSlopeDegrees", distributionContext,
            recipe.maximumSlopeDegrees, result.errors);
        requiredNumber(distribution, "edgeFeatherMetres", distributionContext,
            recipe.edgeFeatherMetres, result.errors);
        requiredNumber(distribution, "minimumSeparationMetres", distributionContext,
            recipe.minimumSeparationMetres, result.errors);
        if(!distribution.value("preventFootprintOverlap").isBool())
            result.errors.append(distributionContext
                + ": 'preventFootprintOverlap' must be boolean.");
        recipe.preventFootprintOverlap =
            distribution.value("preventFootprintOverlap").toBool();
        if(recipe.maximumSlopeDegrees < 0.0 || recipe.maximumSlopeDegrees > 90.0
                || recipe.edgeFeatherMetres < 0.0
                || recipe.minimumSeparationMetres <= 0.0)
            result.errors.append(distributionContext
                + ": distribution values are outside safe ranges.");
        if(!recipe.preventFootprintOverlap)
            result.errors.append(distributionContext
                + ": version 1 requires global footprint-overlap prevention.");

        if(object.contains("osm"))
            parseOsm(object.value("osm").toObject(), recipe,
                context + ".osm", result.errors, result.warnings);

        const QJsonArray vegetation = object.value("vegetation").toArray();
        if(vegetation.isEmpty())
            result.errors.append(context + ": 'vegetation' must not be empty.");
        QSet<QString> entryIds;
        double proportionTotal = 0.0;
        for(int entryIndex = 0; entryIndex < vegetation.size(); ++entryIndex) {
            const QString entryContext = context
                + QString(".vegetation[%1]").arg(entryIndex);
            if(!vegetation.at(entryIndex).isObject()) {
                result.errors.append(entryContext + ": entry must be an object.");
                continue;
            }
            ForestVegetationDefinition entry;
            parseVegetation(vegetation.at(entryIndex).toObject(), entry,
                routeShapesPath, entryContext, result.errors, result.warnings);
            const QString foldedEntryId = entry.id.toCaseFolded();
            if(entryIds.contains(foldedEntryId))
                result.errors.append(entryContext
                    + ": duplicate vegetation id '" + entry.id + "'.");
            entryIds.insert(foldedEntryId);
            proportionTotal += entry.proportion;
            recipe.vegetation.append(entry);
        }
        if(proportionTotal > 0.0) {
            for(ForestVegetationDefinition &entry : recipe.vegetation)
                entry.normalizedProportion = entry.proportion / proportionTotal;
        }
        result.catalog.polyVeg.append(recipe);
    }
    return result;
}
