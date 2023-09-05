// This file is part of CH341-I2C-Tool
// Copyright (C) 2023  Derek Meng

// CH341-I2C-Tool is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// CH341-I2C-Tool is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with CH341-I2C-Tool.  If not, see <https://www.gnu.org/licenses/>.

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <map>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Command {
    QString name, address, reg, data;
    int readLength;
    ULONG speedMode;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, ULONG deviceNum = 0);
    ~MainWindow();

private slots:
    void on_runButton_clicked();

    void on_actionExit_triggered();

    void on_actionOpen_triggered();

    void on_commandsLoadButton_clicked();

    void on_commandsDeleteButton_clicked();

    void on_actionAbout_Device_triggered();

    void on_actionSave_triggered();

    void on_actionSave_As_triggered();

    void on_commandsAddButton_clicked();

    void on_actionReconnect_Device_triggered();

private:
    Ui::MainWindow *ui;
    bool saved = false;
    QString currPath = "";
    std::map<QString, Command> commands;

    ULONG deviceNum = 0;
    BOOL setBusSpeed() const;
    void addCommands();
};
#endif // MAINWINDOW_H
