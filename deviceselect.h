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

#ifndef DEVICESELECT_H
#define DEVICESELECT_H

#include <QDialog>
#include <windows.h>

namespace Ui {
class DeviceSelect;
}

class DeviceSelect : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceSelect(QWidget *parent = nullptr);
    ~DeviceSelect();

    int deviceNum = -1;

private slots:
    void on_pushButton_clicked();

private:
    Ui::DeviceSelect *ui;
};

#endif // DEVICESELECT_H
