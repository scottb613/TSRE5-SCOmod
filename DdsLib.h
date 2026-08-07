/*
 * This file is part of TSRE5.
 *
 * Licensed under GNU General Public License 3.0 or later.
 */

#ifndef DDSLIB_H
#define DDSLIB_H

#include <QThread>

class Texture;

class DdsLib : public QThread {
    Q_OBJECT

public:
    DdsLib();
    static bool IsThread;
    Texture *texture = nullptr;
    void run() override;
};

#endif
