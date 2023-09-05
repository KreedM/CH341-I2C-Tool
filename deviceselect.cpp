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

#include "deviceselect.h"
#include "ui_deviceselect.h"

#include <QMessageBox>

#include "CH341DLL_EN.H"

DeviceSelect::DeviceSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceSelect)
{
    ui->setupUi(this);
    this->setWindowTitle("CH341 Device Select");
}

DeviceSelect::~DeviceSelect()
{
    delete ui;
}

void DeviceSelect::on_pushButton_clicked()
{
    this->deviceNum = ui->spinBox->value();

    qDebug() << "CH341 LIBRARY VERSION:" << CH341GetVersion();
    qDebug().nospace() << "OPENING CH341 DEVICE #" << this->deviceNum;

    HANDLE result = CH341OpenDevice(this->deviceNum);

    if((INT64)result < 0) {
        qDebug().nospace() << "FAILED TO OPEN CH341 DEVICE #" << this->deviceNum << "!\n";
        QMessageBox::critical(this, " ", "Failed to open CH341 device #" + QString::number(this->deviceNum) + "!");

        this->deviceNum = -1;
    }
    else {
        qDebug().nospace() << "OPENED CH341 DEVICE #" << this->deviceNum << "!";
        qDebug() << "CH341 DRIVER VERSION:" << CH341GetDrvVersion() << "\n";

        this->close();
    }
}

