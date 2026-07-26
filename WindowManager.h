
/* 
 * File:   WindowManager.h
 * Author: E
 *
 * Created on March 19, 2024, 2:12 PM
 */

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H


#include <QtWidgets>


class WindowManager : public QWidget {
    Q_OBJECT
    
public: 
        
public slots:
    void LoadConEditor();
    void LoadShapeViewer(QString arg);
    void LoadRouteEditor();

private:    


};

#endif /* WINDOWMANAGER_H */

