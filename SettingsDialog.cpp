#include "SettingsDialog.h"
#include "Game.h"
#include "GuiFunct.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QRadioButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QColorDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QLabel>
#include <QIntValidator>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    GuiFunct::applyEditorPanelStyle(this);
    setWindowTitle("TSRE5 Settings Editor");
    resize(1100, 850);
    setupUi();
}

QWidget* SettingsDialog::createScrollTab(QFormLayout*& layout, QTabWidget* tabs, const QString& title) {
    QScrollArea* sa = new QScrollArea(tabs);
    sa->setWidgetResizable(true);
    
    QWidget* container = new QWidget();
    QVBoxLayout* vBox = new QVBoxLayout(container);
    
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(11, 5, 11, 5); 
    headerLayout->setSpacing(10);
    
    QLabel* lblToken = new QLabel("<b>Token</b>");
    lblToken->setFixedWidth(200); 
    lblToken->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* lblValue = new QLabel("<b>Value</b>");
    lblValue->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); 
    
    headerLayout->addWidget(lblToken);
    headerLayout->addWidget(lblValue);
    headerLayout->addStretch(1);
    
    vBox->addLayout(headerLayout);
    
    QWidget* formContent = new QWidget();
    layout = new QFormLayout(formContent);
    layout->setLabelAlignment(Qt::AlignRight);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formContent->setStyleSheet("QLabel { min-width: 200px; max-width: 200px; qproperty-alignment: 'AlignTop | AlignRight'; }");
    layout->setVerticalSpacing(1);

    vBox->addWidget(formContent);
    vBox->addStretch(1);

    sa->setWidget(container);
    tabs->addTab(sa, title);
    return container;
}

void SettingsDialog::addRow(QFormLayout* l, const QString& key, const QString& type, const QString& label, const QString& helpText) {
    QString lowKey = key.toLower();
    QString tooltip = helpForSetting(key, helpText);
    QWidget* rowField = new QWidget();
    QHBoxLayout* rowLayout = new QHBoxLayout(rowField);
    rowLayout->setSpacing(15);
    rowLayout->setContentsMargins(6, 2, 6, 2);
    QString rowBg = (l->rowCount() % 2 == 0) ? "#202020" : "#2B2B2B";
    rowField->setStyleSheet(QString("QWidget { background-color: %1; color: white; }").arg(rowBg));
    
    QWidget* valueContainer = new QWidget();
    valueContainer->setFixedWidth(550); 
    QHBoxLayout* valueLayout = new QHBoxLayout(valueContainer);
    valueLayout->setContentsMargins(0, 0, 0, 0);
    valueLayout->setSpacing(5);
    valueLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QWidget* inputPart = nullptr;

    if (type == "bool") {
        QWidget* boolCont = new QWidget();
        QHBoxLayout* h = new QHBoxLayout(boolCont);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(10);

        QRadioButton* rbTrue = new QRadioButton("True");
        QRadioButton* rbFalse = new QRadioButton("False");

        // Keep radio-button exclusivity, but paint the F12 boolean controls with
        // the same clean square indicator used by checkboxes.
        const QString squareRadioStyle =
            "QRadioButton { color: white; spacing: 5px; }"
            "QRadioButton::indicator {"
            " width: 13px; height: 13px; background-color: #202020;"
            " border: 1px solid #9a9a9a; border-radius: 0px;"
            "}"
            "QRadioButton::indicator:hover { border-color: #f08200; }"
            "QRadioButton::indicator:checked {"
            " background-color: #f08200; border-color: #ffad3b; border-radius: 0px;"
            "}";
        rbTrue->setStyleSheet(squareRadioStyle);
        rbFalse->setStyleSheet(squareRadioStyle);

        rbTrue->setProperty("buddy", QVariant::fromValue(rbFalse));
        rbFalse->setProperty("buddy", QVariant::fromValue(rbTrue));

        h->addWidget(rbTrue);
        h->addWidget(rbFalse);
        
        valueWidgetMap[lowKey] = rbTrue; 
        inputPart = boolCont;
    } else if (type == "dir" || type == "color") {
        QWidget* dirCont = new QWidget();
        QHBoxLayout* h = new QHBoxLayout(dirCont);
        h->setContentsMargins(0,0,0,0);
        QLineEdit* le = new QLineEdit();
        le->setFixedWidth(450); 
        QPushButton* btn = new QPushButton(type == "dir" ? "..." : "Color");
        btn->setFixedWidth(type == "dir" ? 30 : 60);
        
        if (type == "dir") {
            connect(btn, &QPushButton::clicked, [this, le]() {
                QString d = QFileDialog::getExistingDirectory(this, "Select Directory", le->text());
                if (!d.isEmpty()) le->setText(d);
            });
        } else {
            connect(btn, &QPushButton::clicked, [this, le]() {
                QColor c = QColorDialog::getColor(QColor(le->text()), this);
                if (c.isValid()) le->setText(c.name().toUpper());
            });
        }
        h->addWidget(le); h->addWidget(btn);
        valueWidgetMap[lowKey] = le;
        inputPart = dirCont;
    } else if (type == "twonumber") {
        QWidget* numCont = new QWidget();
        QHBoxLayout* h = new QHBoxLayout(numCont);
        h->setContentsMargins(0,0,0,0);
        QLineEdit* n1 = new QLineEdit(); 
        QLineEdit* n2 = new QLineEdit();
        n1->setFixedWidth(60);
        n2->setFixedWidth(60);
        h->addWidget(n1); h->addWidget(n2);
        valueWidgetMap[lowKey] = n1;
        subValueWidgetMap[lowKey] = n2;
        inputPart = numCont;
    } else if (type == "textbox") {
        QTextEdit* te = new QTextEdit();
        te->setMaximumHeight(50);
        te->setFixedWidth(500);
        valueWidgetMap[lowKey] = te;
        inputPart = te;
    } else { 
        QLineEdit* le = new QLineEdit();
        le->setFixedWidth(500);
        if (type == "number" || type == "int") {
            le->setFixedWidth(60);
            if (type == "int")
                le->setValidator(new QIntValidator(le));
        }
        valueWidgetMap[lowKey] = le;
        inputPart = le;
    }

    valueLayout->addWidget(inputPart);
    if (!tooltip.isEmpty()) {
        valueContainer->setToolTip(tooltip);
        if (inputPart) {
            inputPart->setToolTip(tooltip);
            QList<QWidget*> childWidgets = inputPart->findChildren<QWidget*>();
            for (int i = 0; i < childWidgets.size(); i++)
                childWidgets[i]->setToolTip(tooltip);
        }
    }

    rowLayout->addWidget(valueContainer, 0, Qt::AlignTop);
    rowLayout->addStretch(1);

    QLabel* labelWidget = new QLabel(label);
    labelWidget->setAlignment(Qt::AlignTop | Qt::AlignRight);
    labelWidget->setStyleSheet(QString("QLabel { min-width: 200px; max-width: 200px; background-color: %1; color: white; padding: 4px 6px; }").arg(rowBg));
    if (!tooltip.isEmpty())
        labelWidget->setToolTip(tooltip);
    l->addRow(labelWidget, rowField);
    if (!tooltip.isEmpty()) {
        QLabel* rowLabel = qobject_cast<QLabel*>(l->labelForField(rowField));
        if (rowLabel)
            rowLabel->setToolTip(tooltip);
    }
}

QString SettingsDialog::helpForSetting(const QString& key, const QString& fallback) const {
    QString k = key.toLower();
    if (k == "consoleoutput") return "Shows log output in the command window while TSRE is running. Leave this off for normal use unless you are troubleshooting.";
    if (k == "debugoutput") return "Enables extra diagnostic logging. Use this only while chasing a problem because it can create noisy logs.";
    if (k == "fullscreen") return "Starts the editor maximized. Leave this off if you prefer TSRE to restore normal window placement.";
    if (k == "soundenabled") return "Controls legacy TSRE sound support. This is separate from GenX interface sounds.";
    if (k == "scosoundenabled") return "Enables GenX interface sounds such as placement clicks, error buzzes, and mode-change chirps.";
    if (k == "startapp") return "Selects the startup tool. Use r for Route Editor, c for Consist Editor, or s for Shape Viewer.";
    if (k == "systemtheme") return "Uses the Windows system palette instead of TSRE's built-in dark interface colors.";
    if (k == "unsafemode") return "Enables advanced maintenance operations that can alter route data. Keep this off unless you know a tool requires it.";
    if (k == "useimperial") return "Displays supported measurements in imperial units where TSRE provides that conversion.";
    if (k == "usennumpad") return "Allows numpad keys to act as movement/editing shortcuts.";
    if (k == "useworkingdir") return "When true, TSRE writes working files relative to the current working directory. When false, logs stay beside TSRE.";
    if (k == "warningbox") return "Shows a warning before closing when route changes have not been saved.";
    if (k == "logfiledays") return "Deletes log files older than this many days during log cleanup.";
    if (k == "logfilemax") return "Keeps at most this many log files. Larger values preserve history but can clutter the TSRE folder.";
    if (k == "gameroot") return "Optional path to the MSTS or ORTS Train Simulator folder. If set, it can bypass folder browsing at startup.";
    if (k == "routename") return "Optional route folder name to open directly. Leave blank to use the route selection screen.";
    if (k == "createnewifnotexist") return "Creates the named route if routeName is set and the route does not already exist.";
    if (k == "starttilex") return "Optional startup tile X coordinate. Leave blank unless you want TSRE to jump to a specific tile.";
    if (k == "starttiley") return "Optional startup tile Y coordinate. Leave blank unless you want TSRE to jump to a specific tile.";
    if (k == "geopath") return "Folder containing SRTM HGT elevation files used by the F3 terrain tools.";
    if (k == "loadactivities") return "Loads route activities for validation and editing. Turn this off only to speed up startup on routes where activities do not matter.";
    if (k == "loadallwfiles") return "Loads every world file for whole-route checking. This is useful for diagnostics but slower on large routes.";
    if (k == "routemergestring") return "Advanced route merge offset string. Example format: IRM:0:0:0.";
    if (k == "routemergetdb") return "Allows route merge operations to merge track database data. Use only with a planned merge workflow.";
    if (k == "routemergeterrain") return "Allows route merge operations to overwrite overlapping terrain heights.";
    if (k == "routemergeterrtex") return "Allows route merge operations to overwrite overlapping terrain texture assignments.";
    if (k == "mainwindowlayout") return "Controls the main editor panel order. P is Properties, T is Tools, W is World, and C is Control Panel.";
    if (k == "toolshidden") return "Starts with tool panels hidden so the viewport gets more room.";
    if (k == "uiscale") return "Scales editor fonts and panel widths. Recommended public range is 1.00 to 1.25; 1.15 works well on Scott's 32-inch 2K display.";
    if (k == "camerafov") return "Sets the camera field of view in degrees. Lower values feel more zoomed in; higher values show more peripheral view.";
    if (k == "cameraspeedmax") return "Camera movement speed while holding Shift.";
    if (k == "cameraspeedmin") return "Camera movement speed while holding Ctrl.";
    if (k == "cameraspeedstd") return "Normal camera movement speed.";
    if (k == "camerasticktoterrain") return "Keeps the camera from dropping underground. This can also be toggled with the slash key.";
    if (k == "lockcamera") return "Locks the camera's Y axis while moving. This is the same behavior toggled with the period key.";
    if (k == "mousespeed") return "Controls mouse look and pan sensitivity. Lower values slow the mouse down; 0.1 is a comfortable GenX default.";
    if (k == "aasamples") return "Sets anti-aliasing sample count. Higher values can smooth edges but may reduce performance.";
    if (k == "allowobjlag") return "Allows object loading to spread over multiple frames to reduce stalls while moving through populated routes.";
    if (k == "imagesubstitution") return "Allows TSRE to substitute ACE or DDS textures when one version is missing.";
    if (k == "imageupgrade") return "Prefers DDS textures when available. Turn this off if you need TSRE to follow shape-defined texture names more strictly.";
    if (k == "maxobjlag") return "Caps deferred object loading work. Higher values may load faster but can increase stutter.";
    if (k == "mstsshadows") return "Enables simplified MSTS-style shadow rendering.";
    if (k == "newsymbols") return "Uses the newer TSRE editor symbols. Turn this off to use the older pyramid-style symbols.";
    if (k == "objectlod") return "Object draw distance. Around 2000 is enough for most routes; higher values show more objects but cost performance.";
    if (k == "railprofile") return "Sets the two rail-edge offsets used when drawing dynamic track.";
    if (k == "shadowlowmapsize") return "Resolution for lower quality shadow maps.";
    if (k == "shadowmapsize") return "Resolution for normal shadow maps. Larger values can look better but cost memory and performance.";
    if (k == "shadowsenabled") return "Enables shadow rendering. This can affect performance on busy routes.";
    if (k == "texturequality") return "Controls texture quality level used by the renderer.";
    if (k == "tilelod") return "Number of terrain tiles loaded in each direction around the camera. Higher values show farther terrain but cost memory and startup time.";
    if (k == "usequadtree") return "Uses quadtree object organization for route rendering and lookup performance.";
    if (k == "hudenabled") return "Shows the simple editor HUD overlay.";
    if (k == "hudscale") return "Scales HUD overlay text.";
    if (k == "markerheight") return "Height of route marker sticks.";
    if (k == "markerlines") return "Shows marker guide lines when a route loads.";
    if (k == "markertext") return "Text size used for marker labels.";
    if (k == "ogldefaultlinewidth") return "Default OpenGL line width for editor guide lines.";
    if (k == "rendertritems") return "Shows black TrItem markers for track database items.";
    if (k == "sectionlineheight") return "Height of grey section guide lines.";
    if (k == "selectedcolor") return "Color used for selected object outlines.";
    if (k == "selectedwidth") return "Line width used for selected object outlines.";
    if (k == "selectedterrcolor") return "Color used for selected terrain patch outlines.";
    if (k == "selectedterrwidth") return "Line width used for selected terrain patch outlines.";
    if (k == "skycolor") return "Viewport sky color used by the route editor.";
    if (k == "usesuperelevation") return "Applies superelevation when rendering curves that contain it.";
    if (k == "viewcompass") return "Shows the compass heading display at the top center of the viewport.";
    if (k == "viewmarkers") return "Shows markers selected through the Control Panel.";
    if (k == "viewtrlabels") return "Shows labels for track database items.";
    if (k == "wirelineheight") return "Height of yellow TDB and RDB guide lines above the ground.";
    if (k == "defaultelevationbox") return "Default elevation value used by placement/editing controls that expose an elevation box.";
    if (k == "defaultmovestep") return "Default object movement step in meters for keyboard/object nudging.";
    if (k == "ignoremissingglobalshapes") return "When true, TSRE can list track and road shapes even if they are missing from Global\\Shapes.";
    if (k == "leavetrackshapeafterdelete") return "Leaves the visible track or road shape after deleting database lines. Use only for careful manual repair.";
    if (k == "maxautoplacement") return "Maximum distance in meters used by auto-placement tools.";
    if (k == "numrecentitems") return "Number of recently used objects kept in the object placement panel.";
    if (k == "sigoffset") return "Offset used when placing signal objects relative to the track line.";
    if (k == "snapableradius") return "Maximum distance for snapping to nearby objects or targets.";
    if (k == "snapableonlyrot") return "When true, snapping only affects rotation. When false, snapping can also affect placement.";
    if (k == "trackelevationmaxpm") return "Maximum grade in permille allowed by track elevation tools.";
    if (k == "useonlypositivequaternions") return "Forces saved object rotations to use positive quaternion representations.";
    if (k == "usetdbemptyitems") return "Preserves database numbering by keeping empty TDB items when needed.";
    if (k == "writeenabled") return "Allows route files to be written. Set false for a read-only inspection session.";
    if (k == "writetdb") return "Writes objects directly to the track database. Set false if you prefer manually adding items with the Z key.";
    if (k == "terrainbrushcolor") return "Default F2 terrain brush color.";
    if (k == "terrainbrushintensity") return "Default F2 brush intensity.";
    if (k == "terrainbrushsize") return "Default F2 brush size.";
    if (k == "terraincut") return "Default cutting value for terrain shaping around track and road lines.";
    if (k == "terrainembankment") return "Default embankment value for terrain shaping around track and road lines.";
    if (k == "terrainradius") return "Default maximum radius for terrain shaping tools.";
    if (k == "terrainsize") return "Default terrain shaping width. In GenX, width handling was improved for track cuts and embankments.";
    if (k == "preloadtextures") return "Comma-separated TERRTEX textures to preload. Supports ACE, BMP, DDS, and PNG entries where TSRE supports them.";
    if (k == "mapimageresolution") return "Downloaded map image resolution for F3 map tiles. GenX defaults to 4096 for clearer tile imagery.";
    if (k == "mapengine") return "Selects the map imagery provider setup used by the F3 map tools.";
    if (k == "mapboximagemapsurl") return "Mapbox static imagery URL template. Use only if configuring Mapbox imagery.";
    if (k == "mapboximagemapszoomoffset") return "Zoom offset for Mapbox imagery. A value of -1 is commonly required.";
    if (k == "mapboxmapapikey") return "Mapbox API key used by the Mapbox imagery URL.";
    if (k == "googleimagemapsurl") return "Google Maps static imagery URL template. Use only if configuring Google imagery.";
    if (k == "googlemapapikey") return "Google Maps API key used by the Google imagery URL.";
    if (k == "imagemapsurl") return "Custom imagery URL template. Tokens can include latitude, longitude, zoom, and resolution placeholders.";
    if (k == "mapapikey") return "General API key used by custom map imagery URLs.";
    if (k == "autofix") return "Attempts to repair known TDB anomalies. Use carefully and keep route backups.";
    if (k == "deepunderground") return "Depth threshold used to flag route pieces that are far below terrain.";
    if (k == "deletetrwatermarks") return "Deletes TrWatermark entries that are not used by Open Rails.";
    if (k == "deleteviewdbspheres") return "Deletes ViewDBSphere entries that are not used by Open Rails.";
    if (k == "legacysupport") return "Retains legacy ViewDBSphere and VDBID data when saving.";
    if (k == "listfiles") return "Creates used and unused file lists when exiting. Useful for route cleanup.";
    if (k == "objectstoremove") return "Comma-separated shapes to remove during route cleanup. This requires listfiles and loadAllWFiles.";
    if (k == "routerebuildtdb") return "Allows route TDB rebuild operations. This requires unsafe mode and should be used only with backups.";
    if (k == "sorttileobjects") return "Sorts tile objects by detail level when saving world files.";
    if (k == "fpslimit") return "Optional frame rate limit for advanced or network modes.";
    if (k == "playermode") return "Advanced mode intended for player-style operation rather than route editing.";
    if (k == "proceduraltracks") return "Enables procedural track support where TSRE uses that path.";
    if (k == "serverauth") return "Enables authentication for network/server editing modes.";
    if (k == "serverlogin") return "Login string used by network/server editing modes.";
    if (k == "usenetworkeng") return "Enables the network editor engine. Leave this off for normal local route editing.";
    if (k == "cewindowlayout") return "Controls Consist Editor panel layout. C is Consists, 1 is Main List, and 2 is Second List.";
    if (k == "colorconview") return "Background color used by the Consist Editor consist view.";
    if (k == "colorshapeview") return "Background color used by the Shape Viewer.";
    if (k == "includefolder") return "Optional include-folder override for OpenRailsCZSK project structures.";
    if (k == "loadconsists") return "Loads consists at startup. Turn this off to skip consist loading.";
    if (k == "ortsengenable") return "Gives precedence to ENG settings found in OpenRails folders.";
    if (!fallback.isEmpty()) {
        QString text = fallback;
        text[0] = text[0].toUpper();
        if (!text.endsWith("."))
            text += ".";
        return text;
    }
    return "";
}

void SettingsDialog::createKeyAssignmentsTab(QTabWidget* tabs) {
    QTableWidget* table = new QTableWidget(tabs);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Key" << "Action");
    table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->setAlternatingRowColors(false);
    table->setStyleSheet(
        "QTableWidget { background-color: #202020; color: white; border: 1px solid #555; }"
        "QHeaderView::section { background-color: #303030; color: white; border: 1px solid #555; padding: 3px; }"
    );

    int visualRow = 0;
    bool firstSection = true;
    QStringList lines = keyAssignmentsText().split('\n');
    for (int i = 0; i < lines.size(); i++) {
        QString line = lines[i].trimmed();
        if (line.isEmpty())
            continue;

        int split = line.indexOf(QRegExp("\\s{2,}"));
        if (split < 0) {
            if (!firstSection) {
                int spacerRow = table->rowCount();
                table->insertRow(spacerRow);
                QTableWidgetItem* spacer = new QTableWidgetItem("");
                spacer->setBackground(QColor("#202020"));
                table->setItem(spacerRow, 0, spacer);
                table->setSpan(spacerRow, 0, 1, 2);
                table->setRowHeight(spacerRow, 8);
            }
            int row = table->rowCount();
            table->insertRow(row);
            QTableWidgetItem* section = new QTableWidgetItem(line);
            QFont font = section->font();
            font.setBold(true);
            font.setUnderline(true);
            font.setPointSizeF(font.pointSizeF() + 1.5);
            section->setFont(font);
            section->setForeground(QColor(Qt::white));
            section->setBackground(QColor("#303030"));
            table->setItem(row, 0, section);
            table->setSpan(row, 0, 1, 2);
            table->setRowHeight(row, 26);
            visualRow = 0;
            firstSection = false;
            continue;
        }

        int row = table->rowCount();
        table->insertRow(row);
        QString key = line.left(split).trimmed();
        QString action = line.mid(split).trimmed();
        QColor bg = (visualRow % 2 == 0) ? QColor("#202020") : QColor("#2B2B2B");

        QTableWidgetItem* keyItem = new QTableWidgetItem(key);
        QTableWidgetItem* actionItem = new QTableWidgetItem(action);
        keyItem->setForeground(QColor(Qt::white));
        actionItem->setForeground(QColor(Qt::white));
        keyItem->setBackground(bg);
        actionItem->setBackground(bg);
        table->setItem(row, 0, keyItem);
        table->setItem(row, 1, actionItem);
        table->setRowHeight(row, 22);
        visualRow++;
    }

    tabs->addTab(table, "Key Assignments");
}

QString SettingsDialog::keyAssignmentsText() const {
    return QString(
        "ROUTE EDITOR - MENU SHORTCUTS\n"
        "Shift+Ctrl+S  Save route\n"
        "Alt+F4        Exit/close route editor\n"
        "Ctrl+Z        Undo\n"
        "Ctrl+C        Copy\n"
        "Ctrl+V        Paste\n"
        "F1            Show Objects tools\n"
        "F2            Show Terrain tools\n"
        "F3            Show Geo tools\n"
        "F4            Show Activity tools\n"
        "F5            Toggle Properties window\n"
        "F7            Toggle Control Panel\n"
        "F8            Toggle Errors/Messages window\n"
        "F12           Toggle Settings window\n"
        "/             Toggle Stick Camera To Terrain\n"
        "\n"
        "ROUTE EDITOR - VIEWPORT / OBJECT KEYS\n"
        "Ctrl          Slow object movement step; enables Ctrl-modified actions\n"
        "Shift         Enables Shift-modified actions\n"
        "Alt           Fast object movement step; enables Alt-modified actions\n"
        "B             Create new tile at current camera tile\n"
        "Esc           Cancel resize/translate/rotate modes\n"
        "E             Toggle Select tool\n"
        "Q             Toggle Place tool\n"
        "R             Toggle Rotate mode\n"
        "T             Toggle Translate mode\n"
        "Y             Toggle Resize mode\n"
        "Ctrl+Q        Toggle auto-add-to-TDB\n"
        "Shift+Q       Toggle stick pointer to terrain\n"
        "Home          Jump camera/pointer upward by 40m\n"
        "\n"
        "TERRAIN / HEIGHT TOOL CONTEXT\n"
        "Z             Toggle terrain brush direction +/- when not Ctrl\n"
        "Alt+A         Select all terrain texture patches on the currently selected tile\n"
        "\n"
        "SELECT / PLACE TOOL CONTEXT\n"
        "Up / 8        Move selected object forward; or resize/rotate X+\n"
        "Down / 2      Move selected object backward; or resize/rotate X-\n"
        "Left / 4      Move selected object left; or resize/rotate Y-\n"
        "Right / 6     Move selected object right; or resize/rotate Y+\n"
        "PageUp / 9    Move selected object up; or resize/rotate Z+\n"
        "PageDown / 3  Move selected object down; or rotate/resize Z-\n"
        "7             Move selected object down; or rotate/resize Z-\n"
        "F             Set/conform terrain to selected track/object/ruler\n"
        "Ctrl+F        Conform terrain along the selected track/road vector within the current tile\n"
        "Shift+F       Smooth terrain around selected track/object/ruler\n"
        "H             Adjust selected object position to terrain\n"
        "N             Adjust selected object rotation to terrain\n"
        "Delete        Delete selected object / track item / activity object\n"
        "C             Clone selected world object\n"
        "P             Pick object rotation for placement\n"
        "Ctrl+P        Pick object for placement\n"
        "Shift+P       Pick object rotation + elevation for placement\n"
        "Z             Toggle selected world object in/out of TDB\n"
        "X             Flip selected world object\n"
        "\n"
        "FREE CAMERA\n"
        "W / Up        Move camera forward\n"
        "S / Down      Move camera backward\n"
        "A / Left      Move camera left\n"
        "D / Right     Move camera right\n"
        "Space         Vertical/control movement mode\n"
        "Shift         Fast camera speed\n"
        "Ctrl          Slow camera speed\n"
        ".             Toggle Y-axis/camera lock\n"
        "PageUp        Move/act like upward/forward camera control when no object selected\n"
        "PageDown      Move/act like downward/back camera control when no object selected\n"
        "\n"
        "ROTATING CAMERA\n"
        "W / Up        Move camera forward\n"
        "S / Down      Move camera backward\n"
        "A / Left      Move camera left\n"
        "D / Right     Move camera right\n"
        "Space         Vertical/control movement mode\n"
        "Shift         Fast camera speed\n"
        "Q / Space     Key release clears control mode\n"
        "E / Shift     Key release restores normal speed\n"
        "\n"
        "ROUTE EDITOR WINDOW\n"
        "Esc           Close route editor window when handled by RouteEditorWindow\n"
        "\n"
        "SHAPE VIEWER / CONSIST VIEW CONTEXT\n"
        "Delete        Delete selected consist item\n"
        "F             Flip selected consist item\n"
        "Right         Move selected consist item right\n"
        "Left          Move selected consist item left\n");
}

void SettingsDialog::setupUi() {
    QVBoxLayout* main = new QVBoxLayout(this);
    QTabWidget* tabs = new QTabWidget(this);
    main->addWidget(tabs);
    setStyleSheet(QString(
        "QDialog, QTabWidget::pane, QScrollArea, QWidget { background-color: #303030; color: white; }"
        "QLabel { color: white; }"
        "QRadioButton { color: white; }"
        "QPushButton { background-color: #202020; color: white; border: 1px solid #777; padding: 3px; }"
        "QPushButton:pressed { background-color: #3a3a3a; }"
        "QTabBar::tab { background-color: #202020; color: white; border: 1px solid #555; padding: 4px 10px; }"
        "QTabBar::tab:selected { background-color: #404040; }"
    ) + GuiFunct::scoEditorPanelStyle() + QString(
        "QRadioButton::indicator {"
        " width: 13px; height: 13px; background-color: #202020; border: 1px solid #9a9a9a; border-radius: 0px;"
        "}"
        "QRadioButton::indicator:hover { border-color: #f08200; }"
        "QRadioButton::indicator:checked {"
        " background-color: #f08200; border-color: #ffad3b; border-radius: 0px;"
        "}"
    ));
    QFormLayout* l = nullptr;

    createScrollTab(l, tabs, "General");
    addRow(l, "consoleOutput", "bool", "Console Output", "Displays log output in realtime in command window");
    addRow(l, "debugOutput", "bool", "Debug Output", "enables extended logging detail");
    addRow(l, "fullscreen", "bool", "Fullscreen", "Prevents screen from being maximized");
    addRow(l, "soundEnabled", "bool", "Sound Enabled", "");
    addRow(l, "startapp", "string", "Start App", "r=Route Edit, c=Consist Edit, s=Shapeviewer");
    addRow(l, "systemTheme", "bool", "System Theme", "true uses Windows palette");
    addRow(l, "unsafemode", "bool", "Unsafe Mode", "Only for risky features");
    addRow(l, "useImperial", "bool", "Use Imperial", "");
    addRow(l, "usenNumPad", "bool", "Use NumPad", "");
    addRow(l, "useWorkingDir", "bool", "Use Working Dir", "false saves logs to TSRE folder");
    addRow(l, "warningBox", "bool", "Warning Box", "warn before exiting without save");

    createScrollTab(l, tabs, "Logging");
    addRow(l, "logfiledays", "number", "Log File Days", "delete files older than X days");
    addRow(l, "logfilemax", "number", "Log File Max", "keep only X logs");

    createScrollTab(l, tabs, "Startup");
    addRow(l, "gameRoot", "dir", "Game Root", "your ORTS Content drive/folder");
    addRow(l, "routeName", "string", "Route Name", "add route name to skip route selection menu");
    addRow(l, "createNewIfNotExist", "bool", "Create New Route", "Create routeName if not present");
    addRow(l, "startTileX", "number", "Start Tile X", "");
    addRow(l, "startTileY", "number", "Start Tile Y", "");
    addRow(l, "geoPath", "dir", "Geo Path", "Folder housing HGT files");
    addRow(l, "loadActivities", "bool", "Load Activities", "");
    addRow(l, "loadAllWFiles", "bool", "Load All W Files", "");
    addRow(l, "routeMergeString", "string", "Route Merge String", "e.g. IRM:0:0:0");
    addRow(l, "routeMergeTDB", "bool", "Route Merge TDB", "");
    addRow(l, "routeMergeTerrain", "bool", "Route Merge Terrain", "");
    addRow(l, "routeMergeTerrtex", "bool", "Route Merge Terrtex", "");

    createScrollTab(l, tabs, "UI");
    addRow(l, "mainWindowLayout", "string", "Window Layout", "P = Properties, T = Tools, W = World, C = Control Panel");
    addRow(l, "toolsHidden", "bool", "Tools Hidden", "");
    addRow(l, "uiScale", "number", "UI Scale", "1.00 to 1.25 recommended");
    addRow(l, "scoSoundEnabled", "bool", "UI Sounds", "Enables interface clicks, error buzzes, and success chirps");

    createScrollTab(l, tabs, "Camera");
    addRow(l, "cameraFov", "number", "Camera FOV", "");
    addRow(l, "cameraSpeedMax", "number", "Camera Speed Max (Shift)", "");
    addRow(l, "cameraSpeedMin", "number", "Camera Speed Min (Ctrl)", "");
    addRow(l, "cameraSpeedStd", "number", "Camera Speed Normal", "");
    addRow(l, "cameraStickToTerrain", "bool", "Stick to Terrain", "Toggle with / key");
    addRow(l, "lockCamera", "bool", "Lock Camera", "Toggle with . key");
    addRow(l, "mouseSpeed", "number", "Mouse Speed", "");

    createScrollTab(l, tabs, "Rendering");
    addRow(l, "AASamples", "number", "AA Samples", "");
    addRow(l, "allowObjLag", "number", "Allow Obj Lag", "");
    addRow(l, "imageSubstitution", "bool", "Image Substitution", "allow for ACE or DDS to be shown if missing DDS or ACE");
    addRow(l, "imageUpgrade", "bool", "Image Upgrade", "show DDS if available");
    addRow(l, "maxObjLag", "int", "Max Obj Lag", "");
    addRow(l, "MSTSshadows", "bool", "MSTS Shadows", "");
    addRow(l, "newSymbols", "bool", "New Symbols", "False uses old pyramids");
    addRow(l, "objectLod", "number", "Object LOD", "2000 is plenty");
    addRow(l, "railProfile", "twonumber", "Rail Profile", "rail edges for dynamic track");
    addRow(l, "shadowLowMapSize", "number", "Shadow Low Map", "");
    addRow(l, "shadowMapSize", "number", "Shadow Map", "");
    addRow(l, "shadowsEnabled", "bool", "Shadows Enabled", "affects performance if true");
    addRow(l, "textureQuality", "number", "Texture Quality", "");
    addRow(l, "tileLod", "number", "Tile LOD", "");
    addRow(l, "useQuadTree", "bool", "Use QuadTree", "");

    createScrollTab(l, tabs, "Overlays");
    addRow(l, "hudEnabled", "bool", "HUD Enabled", "");
    addRow(l, "hudScale", "number", "HUD Scale", "");
    addRow(l, "markerHeight", "number", "Marker Height", "");
    addRow(l, "markerLines", "bool", "Marker Lines", "");
    addRow(l, "markerText", "number", "Marker Text Size", "");
    addRow(l, "oglDefaultLineWidth", "number", "OGL Line Width", "");
    addRow(l, "renderTrItems", "bool", "Render Tr Items", "");
    addRow(l, "sectionLineHeight", "number", "Section Line Height", "");
    addRow(l, "fogColor", "color", "Fog Color", "");
    addRow(l, "fogDensity", "string", "Fog Density", "");
    addRow(l, "selectedColor", "color", "Selected Color", "");
    addRow(l, "selectedWidth", "number", "Selected Width", "");
    addRow(l, "selectedTerrColor", "color", "Selected Terrain Color", "");
    addRow(l, "selectedTerrWidth", "number", "Selected Terrain Width", "");
    addRow(l, "skyColor", "color", "Sky Color", "");
    addRow(l, "useSuperelevation", "bool", "Use Superelevation", "");
    addRow(l, "viewCompass", "bool", "View Compass", "");
    addRow(l, "viewMarkers", "bool", "View Markers", "");
    addRow(l, "viewTRLabels", "bool", "View TR Labels", "");
    addRow(l, "wireLineHeight", "number", "Wire Line Height", "");

    createScrollTab(l, tabs, "Objects");
    addRow(l, "defaultElevationBox", "number", "Default Elevation", "");
    addRow(l, "defaultMoveStep", "number", "Default Move Step", "");
    addRow(l, "ignoreMissingGlobalShapes", "bool", "Ignore Global Shapes", "");
    addRow(l, "leaveTrackShapeAfterDelete", "bool", "Leave Track Shape", "");
    addRow(l, "maxAutoPlacement", "number", "Max Auto Placement", "");
    addRow(l, "numRecentItems", "number", "Recent Items List", "");
    addRow(l, "sigOffset", "number", "Signal Offset", "");
    addRow(l, "snapableRadius", "number", "Snapable Radius", "");
    addRow(l, "snapableOnlyRot", "number", "Snapable Only Rot", "");
    addRow(l, "trackElevationMaxPm", "number", "Max Grade Permille", "");
    addRow(l, "useOnlyPositiveQuaternions", "bool", "Positive Quaternions", "");
    addRow(l, "useTdbEmptyItems", "bool", "Use TDB Empty Items", "");
    addRow(l, "writeEnabled", "bool", "Write Enabled", "");
    addRow(l, "writeTDB", "bool", "Write TDB", "");

    createScrollTab(l, tabs, "Terrain");
    addRow(l, "terrainBrushColor", "color", "Terrain Brush Color", "");
    addRow(l, "terrainBrushIntensity", "number", "Brush Intensity", "");
    addRow(l, "terrainBrushSize", "number", "Brush Size", "");
    addRow(l, "terrainCut", "number", "Terrain Cut", "");
    addRow(l, "terrainEmbankment", "number", "Terrain Embankment", "");
    addRow(l, "terrainRadius", "number", "Terrain Radius", "");
    addRow(l, "terrainSize", "number", "Terrain Size", "");
    addRow(l, "preloadTextures", "textbox", "Preload Textures", "TERRTEX: ace, bmp, dds, png");

    createScrollTab(l, tabs, "Map");
    addRow(l, "mapImageResolution", "number", "Map Resolution", "");
    addRow(l, "mapengine", "string", "Map Engine", "");
    addRow(l, "imageMapsUrl", "string", "Image Maps URL", "");
    addRow(l, "imageMapsZoomOffset", "number", "Image Maps Zoom Offset", "");
    addRow(l, "MapAPIKey", "string", "General Map API Key", "");

    createScrollTab(l, tabs, "Cleanup");
    addRow(l, "autoFix", "bool", "Auto Fix", "repair TDB anomalies");
    addRow(l, "deepunderground", "number", "Deep Underground", "");
    addRow(l, "deleteTrWatermarks", "bool", "Delete Tr Watermarks", "");
    addRow(l, "deleteViewDbSpheres", "bool", "Delete VDB Spheres", "");
    addRow(l, "legacySupport", "bool", "Legacy Support", "retention of ViewDBSphere and VDBID");
    addRow(l, "listfiles", "bool", "List Files", "create lists of files used/unused on exit");
    addRow(l, "objectsToRemove", "textbox", "Objects To Remove", "comma separated list of shapes");
    addRow(l, "routeRebuildTDB", "bool", "Route Rebuild TDB", "");
    addRow(l, "sortTileObjects", "bool", "Sort Tile Objects", "Orders items by detail level on save");

    createScrollTab(l, tabs, "Advanced");
    addRow(l, "fpsLimit", "number", "FPS Limit", "");
    addRow(l, "playerMode", "bool", "Player Mode", "");
    addRow(l, "proceduralTracks", "bool", "Procedural Tracks", "");
    addRow(l, "serverAuth", "bool", "Server Auth", "");
    addRow(l, "serverLogin", "string", "Server Login", "");
    addRow(l, "useNetworkEng", "bool", "Use Network Engine", "");

    createScrollTab(l, tabs, "Consist");
    addRow(l, "ceWindowLayout", "string", "CE Layout", "C-Consists, 1-Main List, 2-Second List");
    addRow(l, "colorConView", "color", "Consist View Color", "");
    addRow(l, "colorShapeView", "color", "Shape View Color", "");
    addRow(l, "includeFolder", "string", "Include Folder", "");
    addRow(l, "loadConsists", "bool", "Load Consists", "");
    addRow(l, "ortsEngEnable", "bool", "ORTS Eng Enable", "");

    createKeyAssignmentsTab(tabs);

    QString setFile = "settings.txt";
    QPushButton* saveBtn = new QPushButton(QString("Save %1").arg(setFile));
    QPushButton* reloadBtn = new QPushButton("Save, Restart and Restore");
    reloadBtn->setToolTip("Saves these settings, restarts TSRE, and restores the current route, camera, and window layout. The restart is blocked while route changes are pending.");
    QHBoxLayout* settingsButtons = new QHBoxLayout();
    settingsButtons->addWidget(saveBtn);
    settingsButtons->addWidget(reloadBtn);
    main->addLayout(settingsButtons);

    connect(saveBtn, &QPushButton::clicked, this, [this, setFile]() {
        save(setFile);
    });
    connect(reloadBtn, &QPushButton::clicked, this, [this]() {
        emit restartAndRestoreRequested();
    });
    
    loadSettings();
}

bool SettingsDialog::save(const QString& filename) {
    if (!backupSettingsFile(filename))
        return false;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);

    out << "// TSRE GenX v0.6 settings\n";
    out << "// Both # and // can be used for comments.\n";
    out << "// Recommended public uiScale range: 1.00 to 1.25. Scott's 32\" 2K setup uses 1.15.\n\n";

    out << "\n//// General / System\n\n";
    writeSetting(out, "consoleOutput", "false", "display log output in realtime in the command window");
    writeSetting(out, "debugOutput", "false", "enable extended logging detail");
    writeSetting(out, "fullscreen", "false", "start maximized/fullscreen");
    writeSetting(out, "soundEnabled", "false", "legacy TSRE sound support");
    writeSetting(out, "startapp", "r", "r = Route Editor, c = Consist Editor, s = Shape Viewer");
    writeSetting(out, "systemTheme", "false", "true uses your Windows palette");
    writeSetting(out, "unsafemode", "false", "only enable for risky/advanced features");
    writeSetting(out, "useImperial", "on", "convert some display values from metric");
    writeSetting(out, "usenNumPad", "true");
    writeSetting(out, "useWorkingDir", "false", "false saves logs to the TSRE folder");
    writeSetting(out, "warningBox", "true", "warn before exiting without saving");

    out << "\n\n//// Logging\n\n";
    writeSetting(out, "logfiledays", "20", "delete logs older than X days");
    writeSetting(out, "logfilemax", "50000", "keep only X logs");

    out << "\n\n//// Startup / Route Selection\n\n";
    writeOptionalSetting(out, "gameRoot", "", "your MSTS/ORTS \"Train Simulator\" folder", true);
    writeOptionalSetting(out, "routeName", "", "route name to skip route selection menu", true);
    writeSetting(out, "createNewIfNotExist", "true", "create routeName if it does not already exist");
    writeOptionalSetting(out, "startTileX", "", "optional start location");
    writeOptionalSetting(out, "startTileY", "", "optional start location");
    out << "\n";
    writeSetting(out, "geoPath", "e:/SRTM", "drive/folder containing HGT files");
    writeSetting(out, "loadActivities", "true", "check route activities for errors");
    writeSetting(out, "loadAllWFiles", "false", "true loads the entire route for error checking; slower on large routes");
    out << "\n";
    writeOptionalSetting(out, "routeMergeString", "", "merge route offsets, e.g. \"IRM:0:0:0\"", true);
    writeOptionalSetting(out, "routeMergeTDB", "false", "set true to merge TDBs");
    writeOptionalSetting(out, "routeMergeTerrain", "false", "set true to overwrite overlapping terrain heights");
    writeOptionalSetting(out, "routeMergeTerrtex", "false", "set true to overwrite overlapping terrain textures");

    out << "\n\n//// UI / Windows / Scaling\n\n";
    writeSetting(out, "mainWindowLayout", "PWTC", "window order: P = Properties, T = Tools, W = World, C = Control Panel", true);
    writeSetting(out, "toolsHidden", "false", "only show the viewport");
    writeSetting(out, "uiScale", "1.15", "global editor UI font/panel scale; 1.00 to 1.25 recommended");
    writeSetting(out, "scoSoundEnabled", "true", "interface clicks, error buzzes, and success chirps");

    out << "\n\n//// Camera / Viewport\n\n";
    writeSetting(out, "cameraFov", "35");
    writeSetting(out, "cameraSpeedMax", "40", "camera movement with SHIFT");
    writeSetting(out, "cameraSpeedMin", "1", "camera movement with CTRL");
    writeSetting(out, "cameraSpeedStd", "3", "camera movement normal");
    writeSetting(out, "cameraStickToTerrain", "true", "stop camera from going underground, toggled with \"/\" key");
    writeSetting(out, "lockCamera", "false", "same as pressing \".\" while moving the camera");
    writeSetting(out, "mouseSpeed", "0.1", "mouse look/pan speed");

    out << "\n\n//// Rendering / Performance\n\n";
    writeSetting(out, "AASamples", "0");
    writeSetting(out, "allowObjLag", "10");
    writeSetting(out, "imageSubstitution", "true", "allow ACE or DDS substitution if one is missing");
    writeSetting(out, "imageUpgrade", "true", "prefer DDS if available; false uses shape-defined texture only");
    writeSetting(out, "maxObjLag", "10");
    writeSetting(out, "MSTSshadows", "false", "simplified shadows when true");
    writeSetting(out, "newSymbols", "true", "true uses newer TSRE symbols; false uses older pyramids");
    writeSetting(out, "objectLod", "3000", "2000 is plenty for most routes");
    writeSetting(out, "railProfile", "0.7175, 0.7895", "rail edges for dynamic track display");
    writeSetting(out, "shadowLowMapSize", "1024");
    writeSetting(out, "shadowMapSize", "2048");
    writeSetting(out, "shadowsEnabled", "false", "affects performance if true");
    writeSetting(out, "textureQuality", "1");
    writeSetting(out, "tileLod", "1", "tiles in each direction to load");
    writeSetting(out, "useQuadTree", "true");

    out << "\n\n//// View Overlays / Markers\n\n";
    writeSetting(out, "fogColor", "");
    writeSetting(out, "fogDensity", "");
    writeSetting(out, "hudEnabled", "false");
    writeSetting(out, "hudScale", "1", "HUD text scale");
    writeSetting(out, "markerHeight", "10", "height of marker sticks");
    writeSetting(out, "markerLines", "true", "show marker lines when route loads");
    writeSetting(out, "markerText", "2.5", "marker text size");
    writeSetting(out, "oglDefaultLineWidth", "1", "width of standard lines");
    writeSetting(out, "renderTrItems", "false", "show black TrItem markers");
    writeSetting(out, "sectionLineHeight", "5.0", "grey section line height");
    writeSetting(out, "selectedColor", "#B612FF", "object selection line color");
    writeSetting(out, "selectedWidth", "2", "object selection line width");
    writeSetting(out, "selectedTerrColor", "#FFB612", "terrain selection line color");
    writeSetting(out, "selectedTerrWidth", "4", "terrain selection line width");
    writeSetting(out, "skyColor", "#E0FFFF");
    writeSetting(out, "useSuperelevation", "false", "apply superelevation when rendering curves");
    writeSetting(out, "viewCompass", "false", "show compass at top center");
    writeSetting(out, "viewMarkers", "true", "view markers selected in Control Panel");
    writeSetting(out, "viewTRLabels", "false", "show track item labels");
    writeSetting(out, "wireLineHeight", "6.8", "yellow TDB/RDB line height");

    out << "\n\n//// Object Editing / Placement\n\n";
    writeSetting(out, "defaultElevationBox", "0");
    writeSetting(out, "defaultMoveStep", "0.25");
    writeSetting(out, "ignoreMissingGlobalShapes", "true", "false shows only track/road shapes present in Global\\Shapes");
    writeSetting(out, "leaveTrackShapeAfterDelete", "false", "use only when deleting track/road but keeping TDB lines");
    writeSetting(out, "maxAutoPlacement", "999", "max distance in meters for auto-placement");
    writeSetting(out, "numRecentItems", "30", "length of recently used items list");
    writeSetting(out, "sigOffset", "2.5", "offset for signal object placement");
    writeSetting(out, "snapableRadius", "20", "max distance to snap to nearest object");
    writeSetting(out, "snapableOnlyRot", "false", "false allows free rotation");
    writeSetting(out, "trackElevationMaxPm", "100", "maximum grade in permille");
    writeSetting(out, "useOnlyPositiveQuaternions", "false");
    writeSetting(out, "useTdbEmptyItems", "true", "preserve node numbering when deleting TDB items");
    writeSetting(out, "writeEnabled", "true", "set false for read-only");
    writeSetting(out, "writeTDB", "true", "set false to manually add to TDB via Z key");

    out << "\n\n//// Terrain Editing / F2 Brush Defaults\n\n";
    writeSetting(out, "terrainBrushColor", "#1B2E29", "", true);
    writeSetting(out, "terrainBrushIntensity", "70");
    writeSetting(out, "terrainBrushSize", "25");
    writeSetting(out, "terrainCut", "2");
    writeSetting(out, "terrainEmbankment", "2");
    writeSetting(out, "terrainRadius", "9");
    writeSetting(out, "terrainSize", "1");

    out << "\n\n//// Terrain Texture / Seasonal Paint\n\n";
    writeSetting(out, "preloadTextures", "rock.ace", "supports ace, bmp, dds, png files in TERRTEX folder", true);

    out << "\n\n//// Map / F3 Imagery\n\n";
    writeSetting(out, "mapImageResolution", "4096", "downloaded map imagery resolution");
    writeOptionalSetting(out, "mapengine", "", "optional map imagery provider selector");
    out << "\n";
    writeOptionalSetting(out, "imageMapsUrl", "", "custom imagery URL, use {lat}, {lon}, {zoom}, {res}", false);
    writeOptionalSetting(out, "imageMapsZoomOffset", "-1", "required for MapBox");
    writeOptionalSetting(out, "MapAPIKey", "", "your API key");
    out << "\n";
    out << "// MapBox example:\n";
    out << "// imageMapsUrl = https://api.mapbox.com/styles/v1/mapbox/satellite-v9/static/{lon},{lat},{zoom}/{res}x{res}?access_token=\n";
    out << "// imageMapsZoomOffset = -1       // required for MapBox\n";
    out << "// MapAPIKey = {your API key}\n";
    out << "\n";
    out << "// Google Maps example:\n";
    out << "// imageMapsUrl = http://maps.googleapis.com/maps/api/staticmap?center={lat},{lon}&zoom={zoom}&size={res}x{res}&maptype=satellite&key=\n";
    out << "// MapAPIKey = {your API key}\n";

    out << "\n\n//// Route File Cleanup / Maintenance\n\n";
    writeSetting(out, "autoFix", "false", "repair TDB anomalies");
    writeSetting(out, "deepunderground", "-100", "flag pieces that are not on terrain");
    writeSetting(out, "deleteTrWatermarks", "true", "remove detail not used by ORTS");
    writeSetting(out, "deleteViewDbSpheres", "true", "remove detail not used by ORTS");
    writeSetting(out, "legacySupport", "false", "retain ViewDBSphere and VDBID when true");
    writeSetting(out, "listfiles", "true", "create lists of files used/unused on exit");
    writeSetting(out, "objectsToRemove", "", "requires listfiles and loadAllWFiles; comma-separated shapes", true);
    writeSetting(out, "routeRebuildTDB", "true", "requires unsafemode");
    writeSetting(out, "sortTileObjects", "true", "order items by detail level on save");

    out << "\n\n//// Advanced / Network / Multi-User\n\n";
    writeOptionalSetting(out, "fpsLimit", "59");
    writeOptionalSetting(out, "playerMode", "");
    writeOptionalSetting(out, "proceduralTracks", "true");
    writeOptionalSetting(out, "serverAuth", "yes");
    writeOptionalSetting(out, "serverLogin", "yes@yes.com");
    writeOptionalSetting(out, "useNetworkEng", "false");

    out << "\n\n//// Consist Editor\n\n";
    writeSetting(out, "ceWindowLayout", "cu1", "C = Consists, 1 = Main List, 2 = Second List", true);
    writeSetting(out, "colorConView", "#a2a2a2", "", true);
    writeSetting(out, "colorShapeView", "#a2a2a2", "", true);
    writeSetting(out, "includeFolder", "openrails", "optional override for OpenRailsCZSK Project", true);
    writeSetting(out, "loadConsists", "true", "set false to skip loading consists");
    writeSetting(out, "ortsEngEnable", "true", "give precedence to settings in /OpenRails folders");

    out << "\n\n//// Legacy / Disabled\n\n";
    out << "# season = \"Spring\"               // disabled in GenX; use the F2 Texture Set selector\n";
    out << "# seasonalEditing = on            // disabled in GenX; use the F2 Texture Set selector\n";

    file.close();
    accept();
    return true;
}

QString SettingsDialog::currentValue(const QString& key, const QString& fallback) const {
    QString lowKey = key.toLower();
    if (valueWidgetMap.contains(lowKey)) {
        QWidget* w = valueWidgetMap.value(lowKey);
        if (subValueWidgetMap.contains(lowKey)) {
            QLineEdit* first = qobject_cast<QLineEdit*>(w);
            QLineEdit* second = subValueWidgetMap.value(lowKey);
            if (first && second)
                return first->text().trimmed() + ", " + second->text().trimmed();
        } else if (QRadioButton* rb = qobject_cast<QRadioButton*>(w)) {
            return rb->isChecked() ? "true" : "false";
        } else if (QLineEdit* le = qobject_cast<QLineEdit*>(w)) {
            return le->text().trimmed();
        } else if (QTextEdit* te = qobject_cast<QTextEdit*>(w)) {
            return te->toPlainText().replace("\n", ",").trimmed();
        }
    }

    if (fileValueMap.contains(lowKey))
        return fileValueMap.value(lowKey);

    return fallback;
}

QString SettingsDialog::quotedValue(const QString& key, const QString& fallback) const {
    QString val = currentValue(key, fallback);
    val.remove("\"");
    return "\"" + val + "\"";
}

void SettingsDialog::writeSetting(QTextStream& out, const QString& key, const QString& fallback, const QString& comment, bool quote) const {
    QString val = quote ? quotedValue(key, fallback) : currentValue(key, fallback);
    out << QString("%1 = %2").arg(key, -30).arg(val);
    if (!comment.isEmpty())
        out << " // " << comment;
    out << "\n";
}

void SettingsDialog::writeOptionalSetting(QTextStream& out, const QString& key, const QString& fallback, const QString& comment, bool quote) const {
    QString val = currentValue(key, fallback);
    QString lowKey = key.toLower();
    bool enabled = fileActiveMap.value(lowKey, false);
    if (valueWidgetMap.contains(lowKey) && val.trimmed() != fileValueMap.value(lowKey).trimmed())
        enabled = !val.trimmed().isEmpty();
    QString prefix = enabled ? "" : "// ";
    if (quote && !val.trimmed().isEmpty()) {
        val.remove("\"");
        val = "\"" + val + "\"";
    }
    out << prefix << QString("%1 = %2").arg(key, -28).arg(val);
    if (!comment.isEmpty())
        out << " // " << comment;
    out << "\n";
}

bool SettingsDialog::backupSettingsFile(const QString& filename) {
    QFileInfo info(filename);
    if (!info.exists())
        return true;

    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString backupPath = info.absolutePath() + "/" + info.completeBaseName() + stamp + "." + info.suffix();
    if (QFile::copy(filename, backupPath))
        return true;

    QMessageBox::warning(this, "Settings Backup Failed",
        QString("Could not create backup:\n%1\n\nSettings were not saved.").arg(backupPath));
    return false;
}

void SettingsDialog::loadSettings() {
    fileValueMap.clear();
    fileActiveMap.clear();

    // 1. Fill defaults from Game Engine
    QMapIterator<QString, QWidget*> it(valueWidgetMap);
    while (it.hasNext()) {
        it.next();
        QString memVal = getGameValue(it.key());
        if (!memVal.isEmpty()) updateWidgetValue(it.key(), memVal);
    }

    // 2. Overlay settings from file (Case Insensitive)
    QFile file("settings.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QString cleanLine = line;
        bool activeLine = true;
        if (line.startsWith("#")) {
            cleanLine = line.mid(1).trimmed();
            activeLine = false;
        } else if (line.startsWith("//")) {
            cleanLine = line.mid(2).trimmed();
            activeLine = false;
        }

        int eqPos = cleanLine.indexOf('=');
        if (eqPos == -1) continue;

        QString key = cleanLine.left(eqPos).trimmed().toLower();
        QString val = cleanLine.mid(eqPos + 1).trimmed();
        
        int cp = val.indexOf(" #");
        if (cp == -1) cp = val.indexOf(" //");
        if (cp != -1) val = val.left(cp).trimmed();
        val.remove("\"");
        bool hasExisting = fileValueMap.contains(key);
        bool existingActive = fileActiveMap.value(key, false);
        bool shouldStore = !hasExisting || activeLine || !existingActive;

        if (shouldStore) {
            fileValueMap[key] = val;
            fileActiveMap[key] = activeLine;

            if (valueWidgetMap.contains(key)) {
                updateWidgetValue(key, val);
            }
        }
    }
    file.close();
}

void SettingsDialog::updateWidgetValue(const QString& key, const QString& val) {
    QWidget* w = valueWidgetMap[key];
    if (!w) return;

    if (subValueWidgetMap.contains(key)) {
        QStringList vals = val.split(",");
        qobject_cast<QLineEdit*>(w)->setText(vals.value(0).trimmed());
        subValueWidgetMap[key]->setText(vals.value(1).trimmed());
    } else if (QRadioButton* rb = qobject_cast<QRadioButton*>(w)) {
        bool isTrue = (val.toLower() == "true" || val.toLower() == "on" || val == "1");
        rb->setChecked(isTrue);
        QRadioButton* rbFalse = rb->property("buddy").value<QRadioButton*>();
        if (rbFalse) rbFalse->setChecked(!isTrue);
    } else if (QLineEdit* le = qobject_cast<QLineEdit*>(w)) {
        le->setText(val);
    } else if (QTextEdit* te = qobject_cast<QTextEdit*>(w)) {
        te->setPlainText(QString(val).replace(",", "\n"));
    }
}

QString SettingsDialog::getGameValue(const QString& key) {
    // Key is always passed in lowercase here
    if (key == "servermode") return Game::ServerMode ? "true" : "false";
    if (key == "serverlogin") return Game::serverLogin;
    if (key == "serverauth") return !Game::serverAuth.isEmpty() ? "true" : "false";
    if (key == "localtsectiononly") return Game::LocalTSectionOnly ? "true" : "false";
    if (key == "useworkingdir") return Game::UseWorkingDir ? "true" : "false";
    if (key == "startapp") return Game::startapp;
    if (key == "loadactivities") return Game::loadActivities ? "true" : "false";
    if (key == "loadconsists") return Game::loadConsists ? "true" : "false";
    if (key == "mainwindowlayout") return Game::mainWindowLayout;
    if (key == "cewindowlayout") return Game::ceWindowLayout;
    if (key == "playermode") return Game::playerMode ? "true" : "false";
    if (key == "usenetworkeng") return Game::useNetworkEng ? "true" : "false";
    if (key == "usequadtree") return Game::useQuadTree ? "true" : "false";
    if (key == "usetdbemptyitems") return Game::useTdbEmptyItems ? "true" : "false";
    if (key == "allowobjlag") return QString::number(Game::allowObjLag);
    if (key == "maxobjlag") return QString::number(Game::maxObjLag);
    if (key == "starttilex") return QString::number(Game::startTileX);
    if (key == "starttiley") return QString::number(Game::startTileY);
    if (key == "objectlod") return QString::number(Game::objectLod);
    if (key == "tilelod") return QString::number(Game::tileLod);
    if (key == "ignoremissingglobalshapes") return Game::ignoreMissingGlobalShapes ? "true" : "false";
    if (key == "deletetrwatermarks") return Game::deleteTrWatermarks ? "true" : "false";
    if (key == "deleteviewdbspheres") return Game::deleteViewDbSpheres ? "true" : "false";
    if (key == "createnewroutes") return Game::createNewRoutes ? "true" : "false";
    if (key == "writeenabled") return Game::writeEnabled ? "true" : "false";
    if (key == "writetdb") return Game::writeTDB ? "true" : "false";
    if (key == "systemtheme") return Game::systemTheme ? "true" : "false";
    if (key == "toolshidden") return Game::toolsHidden ? "true" : "false";
    if (key == "usennumpad") return Game::usenNumPad ? "true" : "false";
    if (key == "camerafov") return QString::number(Game::cameraFov);
    if (key == "cameraspeedmin") return QString::number(Game::cameraSpeedMin);
    if (key == "cameraspeedstd") return QString::number(Game::cameraSpeedStd);
    if (key == "cameraspeedmax") return QString::number(Game::cameraSpeedMax);
    if (key == "mousespeed") return QString::number(Game::mouseSpeed);
    if (key == "uiscale") return QString::number(Game::uiScale);
    if (key == "camerasticktoterrain") return Game::cameraStickToTerrain ? "true" : "false";
    if (key == "mstsshadows") return Game::mstsShadows ? "true" : "false";
    if (key == "viewmarkers") return Game::viewMarkers ? "true" : "false";
    if (key == "viewcompass") return Game::viewCompass ? "true" : "false";
    if (key == "warningbox") return Game::warningBox ? "true" : "false";
    if (key == "leavetrackshapeafterdelete") return Game::leaveTrackShapeAfterDelete ? "true" : "false";
    if (key == "rendertritems") return Game::renderTrItems ? "true" : "false";
    if (key == "consoleoutput") return Game::consoleOutput ? "true" : "false";
    if (key == "fpslimit") return QString::number(Game::fpsLimit);
    if (key == "ortsengenable") return Game::ortsEngEnable ? "true" : "false";
    if (key == "sorttileobjects") return Game::sortTileObjects ? "true" : "false";
    if (key == "ogldefaultlinewidth") return QString::number(Game::oglDefaultLineWidth);
    if (key == "shadowmapsize") return QString::number(Game::shadowMapSize);
    if (key == "shadowlowmapsize") return QString::number(Game::shadowLowMapSize);
    if (key == "shadowsenabled") return QString::number(Game::shadowsEnabled);
    if (key == "texturequality") return QString::number(Game::textureQuality);
    if (key == "snapableradius") return QString::number(Game::snapableRadius);
    if (key == "snapableonlyrot") return Game::snapableOnlyRot ? "true" : "false";
    if (key == "trackelevationmaxpm") return QString::number(Game::trackElevationMaxPm);
    if (key == "proceduraltracks") return Game::proceduralTracks ? "true" : "false";
    if (key == "fullscreen") return Game::fullscreen ? "true" : "false";
    if (key == "hudenabled") return Game::hudEnabled ? "true" : "false";
    if (key == "hudscale") return QString::number(Game::hudScale);
    if (key == "markerlines") return Game::markerLines ? "true" : "false";
    if (key == "loadallwfiles") return Game::loadAllWFiles ? "true" : "false";
    if (key == "autofix") return Game::autoFix ? "true" : "false";
    if (key == "listfiles") return Game::listFiles ? "true" : "false";
    if (key == "mapimageresolution") return QString::number(Game::mapImageResolution);
    if (key == "usesuperelevation") return Game::useSuperelevation ? "true" : "false";
    if (key == "soundenabled") return Game::soundEnabled ? "true" : "false";
    if (key == "scosoundenabled") return Game::scoSoundEnabled ? "true" : "false";
    if (key == "aasamples") return QString::number(Game::AASamples);
    if (key == "defaultelevationbox") return QString::number(Game::DefaultElevationBox);
    if (key == "defaultmovestep") return QString::number(Game::DefaultMoveStep);
    if (key == "seasonalediting") return Game::seasonalEditing ? "true" : "false";
    if (key == "numrecentitems") return QString::number(Game::numRecentItems);
    if (key == "useonlypositivequaternions") return Game::useOnlyPositiveQuaternions ? "true" : "false";
    if (key == "wirelineheight") return QString::number(Game::wireLineHeight);
    if (key == "sectionlineheight") return QString::number(Game::sectionLineHeight);
    if (key == "selectedwidth") return QString::number(Game::selectedWidth);
    if (key == "selectedterrwidth") return QString::number(Game::selectedTerrWidth);
    if (key == "lockcamera") return Game::lockCamera ? "true" : "false";
    if (key == "debugoutput") return Game::debugOutput ? "true" : "false";
    if (key == "legacysupport") return Game::legacySupport ? "true" : "false";
    if (key == "newsymbols") return Game::newSymbols ? "true" : "false";
    if (key == "maxautoplacement") return QString::number(Game::maxAutoPlacement);
    if (key == "imagemapszoomoffset") return QString::number(Game::imageMapsZoomOffset);
    if (key == "deepunderground") return QString::number(Game::deepUnderground);
    if (key == "viewtrlabels") return Game::viewTRLabels ? "true" : "false";
    if (key == "markerheight") return QString::number(Game::markerHeight);
    if (key == "markertext") return QString::number(Game::markerText);
    if (key == "sigoffset") return QString::number(Game::sigOffset);
    if (key == "imagesubstitution") return Game::imageSubstitution ? "true" : "false";
    if (key == "imageupgrade") return Game::imageUpgrade ? "true" : "false";
    if (key == "includefolder") return Game::includeFolder;
    if (key == "logfilemax") return QString::number(Game::logfileMax);
    if (key == "logfiledays") return QString::number(Game::logfileDays);
    if (key == "unsafemode") return Game::UnsafeMode ? "true" : "false";
    if (key == "routemergeterrain") return Game::routeMergeTerrain ? "true" : "false";
    if (key == "routemergetdb") return Game::routeMergeTDB ? "true" : "false";
    if (key == "routemergeterrtex") return Game::routeMergeTerrtex ? "true" : "false";
    if (key == "routerebuildtdb") return Game::routeRebuildTDB ? "true" : "false";
    
    if (key == "selectedcolor" && Game::selectedColor) return Game::selectedColor->name();
    if (key == "selectedterrcolor" && Game::selectedTerrColor) return Game::selectedTerrColor->name();
    if ((key == "terrbrushcolor" || key == "terrainbrushcolor") && Game::terrBrushColor) return Game::terrBrushColor->name();
    
    if (key == "railprofile") return QString("%1, %2").arg(Game::railProfile[0]).arg(Game::railProfile[1]);

    return "";
}
