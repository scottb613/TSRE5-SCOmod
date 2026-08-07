/*  This file is part of TSRE5.
 *
 *  Phase 1 Activity Builder workspace.
 */

#ifndef ACTIVITYBUILDERWINDOW_H
#define ACTIVITYBUILDERWINDOW_H

#include <QMainWindow>
#include <QTimer>

class ActivityTools;
class ActivityTrackViewer;
class QDockWidget;
class QLabel;
class Path;
class Route;

class ActivityBuilderWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ActivityBuilderWindow(ActivityTools *tools, QWidget *parent = NULL);

public slots:
    void routeLoaded(Route *route);
    void setSelectedPath(Path *path);
    void setPinned(bool pinned);
    void savePinnedGeometry();
    void showJunctionDetails(QString text);
    void beginPathCreation(Path *path);
    void setPathControlsActive(bool active);
    void refreshPathMetadata(Path *path);

signals:
    void visibilityChanged(bool visible);
    void userToggleSoundRequested();
    void userPlacementSoundRequested();
    void userErrorSoundRequested();

protected:
    void closeEvent(QCloseEvent *event);
    void moveEvent(QMoveEvent *event);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private:
    void restoreInitialGeometry();
    void moveToDefaultPosition();

    ActivityTrackViewer *viewer = NULL;
    QDockWidget *pathControlsDock = NULL;
    QLabel *pathInfo = NULL;
    QLabel *junctionInfo = NULL;
    QAction *pinAction = NULL;
    QTimer geometrySaveTimer;
    QSize defaultSize;
    bool initialGeometryApplied = false;
    bool dockWidthsInitialized = false;
    bool advancedWaitPlacement = false;
};

#endif
