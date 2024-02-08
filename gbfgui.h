#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_gbfgui.h"

class gbfgui : public QMainWindow
{
    Q_OBJECT

public:
    gbfgui(QWidget *parent = nullptr);
    ~gbfgui();

private:
    Ui::gbfguiClass ui;
};
