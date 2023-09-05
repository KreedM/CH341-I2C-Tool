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

#include "mainwindow.h"
#include "deviceselect.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DeviceSelect* deviceSelect = new DeviceSelect(NULL);
    deviceSelect->exec();
    int deviceNum = deviceSelect->deviceNum;
    delete deviceSelect;

    if(deviceNum == -1)
        return 0;

    MainWindow w;
    w.show();

    return a.exec();
}
