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
#include "./ui_mainwindow.h"

#include <fstream>
#include <algorithm>
#include <sstream>
#include <bitset>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

#include "deviceselect.h"
#include "CH341DLL_EN.H"

MainWindow::MainWindow(QWidget *parent, ULONG deviceNum)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("CH341 I2C Tool");
    this->deviceNum = deviceNum;
}

MainWindow::~MainWindow()
{
    qDebug().nospace() << "CLOSING CH341 DEVICE #" << this->deviceNum;
    CH341CloseDevice(this->deviceNum);
    delete ui;
}

bool isBinary(const std::string& token, const std::size_t& length) {
    if(token.length() > length || token.empty())
        return false;

    for(size_t i = 0; i < token.length(); ++i) {
        if(token[i] != '1' && token[i] != '0')
            return false;
    }

    return true;
}

bool isBinaryByte(const std::string& token) {
    return isBinary(token, 8);
}

bool isValidAddress(const std::string& token) {
    return isBinary(token, 7);
}

bool isValidWriteData(const std::string& tokens, std::vector<std::string>* bytes) {
    if(tokens.empty())
        return true;

    std::istringstream iss(tokens);

    std::string token;
    while(iss >> token) {
        if(!isBinaryByte(token)) {
            if(bytes)
                bytes->push_back(token);

            return false;
        }

        for(std::size_t i = token.length(); i < 8; ++i)
            token = '0' + token;

        if(bytes)
            bytes->push_back(token);
    }

    return true;
}

BOOL MainWindow::setBusSpeed() const {                                  // HELPER FUNCTION FOR RUN COMMMAND BUTTON
    bool result;

    if(ui->busSpeedRadioButton_0->isChecked()) { // 20 kHz
        result = CH341SetStream(this->deviceNum, 0);
        qDebug() << "BUS SPEED: 20 kHz";
    }
    else if(ui->busSpeedRadioButton_1->isChecked()) { // 100 kHz
        result = CH341SetStream(this->deviceNum, 1);
        qDebug() << "BUS SPEED: 100 kHz";
    }
    else if(ui->busSpeedRadioButton_2->isChecked()) { // 400 kHz
        result = CH341SetStream(this->deviceNum, 2);
        qDebug() << "BUS SPEED: 400 kHz";
    }
    else {                                            // 750 kHz
        result = CH341SetStream(this->deviceNum, 3);
        qDebug() << "BUS SPEED: 750 kHz";
    }

    return result;
}

void MainWindow::on_runButton_clicked()                                 // RUN COMMAND BUTTON
{
    ui->readTextEdit->clear();

    // SET BUS SPEED
    if(!this->setBusSpeed()) {
        qDebug() << "Failed to set bus speed, please reconnect the CH341 device!\n";
        QMessageBox::critical(this, " ", "Failed to set bus speed, please reconnect the CH341 device!");
        return;
    }

    // PROCESS ADDRESS
    std::string addressStr = ui->addressLineEdit->text().toStdString();
    if(!isValidAddress(addressStr)) {
        qDebug().nospace().noquote() << "Invalid device address (" << addressStr << ")!\n";
        QMessageBox::warning(this, " ", "Invalid device address (" + QString::fromStdString(addressStr) + ")!");
        return;
    }

    UCHAR address = std::stoi(addressStr, NULL, 2);

    qDebug().nospace() << "ADDRESS: " << Qt::bin << address;

    // PROCESS REGISTER
    std::string regStr = ui->registerLineEdit->text().toStdString();
    if(!regStr.empty() && !isBinaryByte(regStr)) {
        qDebug().nospace().noquote() << "Invalid register address (" << regStr << ")!\n";
        QMessageBox::warning(this, " ", "Invalid register address (" + QString::fromStdString(regStr) + ")!");
        return;
    }

    // PROCESS WRITE DATA
    std::string writeDataStr = ui->writeTextEdit->toPlainText().toStdString();

    if(!regStr.empty())
        writeDataStr = regStr + " " + writeDataStr;

    int readLength = ui->readSpinBox->value();

    if(writeDataStr.empty() && readLength == 0) {
        qDebug() << "Nothing to read/write!\n";
        return;
    }

    std::vector<std::string> strBytes;

    if(!isValidWriteData(writeDataStr, &strBytes)) {
        QString invalidByte = "";

        if(strBytes.size() != 0)
            invalidByte = " (" + QString::fromStdString(strBytes.back()) + ")";

        qDebug().nospace().noquote() << "Invalid write data" << invalidByte << "!\n";
        QMessageBox::warning(this, " ", "Invalid write data" + invalidByte + "!");
        return;
    }

    if(strBytes.size() > 1022) {
        qDebug() << "Exceeded write limit (1022 bytes)!\n";
        QMessageBox::warning(this, " ", "Exceeded write limit (1022 bytes)!");
        return;
    }

    std::vector<UCHAR> bytes; bytes.reserve(strBytes.size() + 1);

    bytes.push_back(address << 1);

    for(std::string& byte : strBytes)
        bytes.push_back(std::stoi(byte, NULL, 2));

    if(bytes.size() == 1) // If only reading
        ++bytes[0];
    else {
        std::size_t count = bytes.size();

        qDebug() << "WRITING:";
        for(std::size_t i = 1; i < count; ++i)
            qDebug().nospace() << "\t" << Qt::bin << bytes[i];
    }

    // SEND R/W REQUEST
    UCHAR* readBuffer = NULL;

    if(readLength != 0)
        readBuffer = new UCHAR[readLength];

    if(!CH341StreamI2C(this->deviceNum, bytes.size(), &bytes[0], readLength, readBuffer)) {
        qDebug() << "Failed to run command, please reconnect the CH341 device!\n";
        QMessageBox::critical(this, " ", "Failed to run command, please reconnect the CH341 device!");
        return;
    }
    else
        ui->statusbar->showMessage("Command success!", 5000);

    // DISPLAY READ DATA
    if(readBuffer) {
        qDebug() << "READING:";
        for(int i = 0; i < readLength; ++i)
            qDebug().nospace() << "\t" << Qt::bin << readBuffer[i];

        std::ostringstream oss;
        oss << std::bitset<8>{readBuffer[0]};

        for(int i = 1; i < readLength; ++i)
            oss << " " << std::bitset<8>{readBuffer[i]};

        ui->readTextEdit->setPlainText(QString::fromStdString(oss.str()));

        delete[] readBuffer;
    }

    qDebug() << "";
}

void MainWindow::addCommands() {                                        // HELPER FUNCTION TO DISPLAY COMMANDS IN COMBO BOX
    ui->commandsComboBox->clear();

    for(std::pair<const QString, Command>& command : this->commands)
        ui->commandsComboBox->addItem(command.first);
}

void MainWindow::on_commandsLoadButton_clicked()                        // LOAD BUTTON
{
    QString commandName = ui->commandsComboBox->currentText();

    if(this->commands.find(commandName) == this->commands.end())
        return;

    if(QMessageBox::question(this, " ", "Load command \"" + commandName + "\"?") == QMessageBox::No)
        return;

    Command& command = this->commands[commandName];

    ui->addressLineEdit->setText(command.address);
    ui->registerLineEdit->setText(command.reg);
    ui->writeTextEdit->setPlainText(command.data);
    ui->readSpinBox->setValue(command.readLength);

    switch (command.speedMode) {
        case 0: ui->busSpeedRadioButton_0->setChecked(true); break;
        case 1: ui->busSpeedRadioButton_1->setChecked(true); break;
        case 2: ui->busSpeedRadioButton_2->setChecked(true); break;
        case 3: ui->busSpeedRadioButton_3->setChecked(true);
    }

    qDebug().nospace() << "LOADED COMMAND " << commandName << "!\n";
    ui->statusbar->showMessage("Loaded command \"" + commandName + "\"!", 5000);
}

void MainWindow::on_commandsAddButton_clicked()                         // ADD BUTTON
{
    QString commandName = ui->commandsComboBox->currentText(); // Command check

    if(commandName.isEmpty())
        return;

    if(this->commands.find(commandName) != this->commands.end()) {
        if(QMessageBox::No == QMessageBox::warning(this, " ", "Command \"" + commandName + "\" already exists! Overwrite?",
                                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
            return;
        }
    }
    else {
        if(QMessageBox::question(this, " ", "Add command \"" + commandName + "\"?") == QMessageBox::No)
            return;
    }

    QString address = ui->addressLineEdit->text(); // Address check
    if(!isValidAddress(address.toStdString())) {
        qDebug().nospace().noquote() << "Failed to add command \"" << commandName << "\" (invalid device address: " << address << ")!\n";
        QMessageBox::warning(this, " ", "Failed to add command \"" + commandName + "\" (invalid device address: " + address + ")!");
        return;
    }

    for(size_t i = address.length(); i < 7; ++i)
        address = '0' + address;

    QString reg = ui->registerLineEdit->text(); // Register check
    if(!reg.isEmpty() && !isBinaryByte(reg.toStdString())) {
        qDebug().nospace().noquote() << "Failed to add command \"" << commandName << "\" (invalid register address: " << reg << ")!\n";
        QMessageBox::warning(this, " ", "Failed to add command \"" + commandName + "\" (invalid register address: " + reg + ")!");
        return;
    }

    if(!reg.isEmpty()) {
        for(size_t i = reg.length(); i < 8; ++i)
            reg = '0' + reg;
    }

    std::string writeDataStr = ui->writeTextEdit->toPlainText().toStdString(); // Write data check

    int readLength = ui->readSpinBox->value();

    if(reg.isEmpty() && writeDataStr.empty() && readLength == 0) {
        qDebug().nospace() << "Failed to add command " << commandName << " (nothing to read/write)!\n";
        QMessageBox::warning(this, " ", "Failed to add command \"" + commandName + "\" (nothing to read/write)!");
        return;
    }

    if(!writeDataStr.empty()) {
        std::vector<std::string> bytes;

        if(!isValidWriteData(writeDataStr, &bytes)) {
            qDebug().nospace().noquote() << "Failed to add command \"" << commandName << "\" (invalid write data: " << bytes.back() << ")!\n";
            QMessageBox::warning(this, " ", "Failed to add command \"" + commandName + "\" (invalid write data: " + QString::fromStdString(bytes.back()) + ")!");
            return;
        }

        std::size_t byteLimit = 1022;
        if(!reg.isEmpty())
            --byteLimit;

        if(bytes.size() > byteLimit) {
            qDebug().nospace() << "Failed to add command " << commandName << " (exceeded write limit of 1022 bytes)!\n";
            QMessageBox::warning(this, " ", "Failed to add command \"" + commandName + "\" (exceeded write limit of 1022 bytes)!");
            return;
        }

        std::ostringstream oss;
        oss << bytes[0];    // Generating write data string
        for(std::size_t i = 1; i < bytes.size(); ++i)
            oss << " " << bytes[i];

        writeDataStr = oss.str();
    }

    ULONG speedMode;    // Get speed mode
    if(ui->busSpeedRadioButton_0->isChecked())      // 20 kHz
        speedMode = 0;
    else if(ui->busSpeedRadioButton_1->isChecked()) // 100 kHz
        speedMode = 1;
    else if(ui->busSpeedRadioButton_2->isChecked()) // 400 kHz
        speedMode = 2;
    else                                            // 750 kHz
        speedMode = 3;

    this->commands[commandName] = { commandName, address, reg, QString::fromStdString(writeDataStr), readLength, speedMode };
    this->addCommands();
    ui->commandsComboBox->setCurrentIndex(ui->commandsComboBox->findText(commandName));

    this->saved = false;

    qDebug().nospace() << "ADDED COMMAND " << commandName << "!\n";
    ui->statusbar->showMessage("Added command \"" + commandName + "\"!", 5000);
}

void MainWindow::on_commandsDeleteButton_clicked()                      // DELETE BUTTON
{
    QString commandName = ui->commandsComboBox->currentText();

    if(this->commands.find(commandName) == this->commands.end())
        return;

    if(QMessageBox::warning(this, " ", "Delete command \"" + commandName + "\"?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;

    this->commands.erase(this->commands.find(commandName));

    ui->commandsComboBox->removeItem(ui->commandsComboBox->findText(commandName));

    this->saved = false;

    qDebug().nospace() << "DELETED COMMAND " << commandName << "!\n";
    ui->statusbar->showMessage("Deleted command \"" + commandName + "\"!", 5000);
}

void MainWindow::on_actionOpen_triggered()                              // OPEN MENU BUTTON
{
    // OPENING FILE
    QString filePath = QFileDialog::getOpenFileName(this, "Open", QDir::homePath(), "Comma separated values (*.csv)");

    if(filePath.isEmpty())
        return;

    qDebug() << "OPENING CSV FILE";

    std::ifstream file(filePath.toStdWString().c_str());

    if(file.is_open()) {
        qDebug().nospace() << "OPENED " << filePath << "!";
        ui->statusbar->showMessage("Opened \"" + filePath + "\"!", 5000);
    }
    else {
        qDebug().nospace() << "Failed to open " << filePath << "!";
        QMessageBox::warning(this, " ", "Failed to open \"" + filePath + "\"!");
        return;
    }

    // PROCESSING FILE
    this->commands.clear();

    std::string line;

    std::getline(file, line); // Skip title line

    QString invalidCommands = "";

    std::istringstream iss;
    while(std::getline(file, line)) {
        iss.str(line);
        iss.clear();

        std::string name; // Name

        if(line[0] == '"') { // Handles the infuriating case of commas and/or double quotes in names
            std::size_t lastDoubleQuote = line.find_last_of('"');

            if(lastDoubleQuote < 2 || lastDoubleQuote + 2 > line.length())
                continue;

            for(size_t i = 1; i < lastDoubleQuote; ++i) {
                name += line[i];

                if(line[i] == '"') // Skip the next quote due to CSV format
                    ++i;
            }

            iss.str(line.substr(lastDoubleQuote + 2));
        }
        else if(!std::getline(iss, name, ',') || name.empty())
            continue;

        if(this->commands.find(QString::fromStdString(name)) != this->commands.end()) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        std::string address; // Address
        if(!std::getline(iss, address, ',') || !isValidAddress(address)) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        for(size_t i = address.length(); i < 7; ++i)
            address = '0' + address;

        std::string reg; // Register
        if(!std::getline(iss, reg, ',') || (!reg.empty() && !isBinaryByte(reg))) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        std::size_t byteLimit = 1022;

        if(!reg.empty()) {
            for(size_t i = reg.length(); i < 8; ++i)
                reg = '0' + reg;

            --byteLimit;
        }

        std::string writeDataStr; // Write data
        std::vector<std::string> bytes;

        if(!std::getline(iss, writeDataStr, ',') || !isValidWriteData(writeDataStr, &bytes) || bytes.size() > byteLimit) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        if(!writeDataStr.empty()) {
            std::ostringstream oss;
            oss << bytes[0];    // Generating write data string
            for(std::size_t i = 1; i < bytes.size(); ++i)
                oss << " " << bytes[i];

            writeDataStr = oss.str();
        }

        int readLength = 0; // Read length
        std::string readLengthStr;
        try {
            if(std::getline(iss, readLengthStr, ',')) {
                readLength = std::stoi(readLengthStr);

                if(readLength < 0 || readLength > 1023)
                    throw std::invalid_argument("Read length out of range!");
            }
            else
                throw std::invalid_argument("Read length not given!");
        }
        catch(const std::invalid_argument& e) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        if(reg.empty() && writeDataStr.empty() && readLength == 0) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        ULONG speedMode = 1; // Speed mode
        std::string speedModeStr;
        try {
            if(std::getline(iss, speedModeStr, ',')) {
                speedMode = std::stoi(speedModeStr);

                if(speedMode > 3)
                    throw std::invalid_argument("Speed mode out of range!");
            }
            else
                throw std::invalid_argument("Speed mode not given!");
        }
        catch(const std::invalid_argument& e) {
            invalidCommands += "\n\"" + QString::fromStdString(name) + "\"";
            continue;
        }

        this->commands[QString::fromStdString(name)] =
            {
                QString::fromStdString(name),
                QString::fromStdString(address),
                QString::fromStdString(reg),
                QString::fromStdString(writeDataStr),
                readLength,
                speedMode
            };
    }

    this->addCommands();

    if(!invalidCommands.isEmpty()) {
        qDebug().noquote() << "Failed to load these commands:" << invalidCommands;
        QMessageBox::warning(this, " ", "Failed to load these commands:" + invalidCommands);
    }

    file.close();

    this->currPath = filePath;
    this->saved = true;

    qDebug() << "";
}

const std::string titleLine = "Command Name,Device Address (7 bits),Register Address,\"Write Data (space separated, <1023 bytes including register address)\",Read Length (<1024 bytes),Speed Mode (0-3)";
void MainWindow::on_actionSave_As_triggered()                           // SAVE AS MENU BUTTON
{
    // OPENING FILE
    QString filePath = QFileDialog::getSaveFileName(this, "Save as", QDir::homePath(), "Comma separated values (*.csv)");

    if(filePath.isEmpty())
        return;

    qDebug() << "SAVING CSV FILE";

    std::ofstream file(filePath.toStdWString().c_str());

    if(file.is_open()) {
        qDebug().nospace() << "SAVED to " << filePath << "!";
        ui->statusbar->showMessage("Saved to \"" + filePath + "\"!", 5000);
    }
    else {
        qDebug().nospace() << "Failed to save to " << filePath << "!";
        QMessageBox::warning(this, " ", "Failed to save to \"" + filePath + "\"!");
        return;
    }

    // WRITING FILE
    file << titleLine;

    for(std::pair<const QString, Command>& command : this->commands) {
        file << "\n";

        std::string name = command.first.toStdString();

        if(name.find('"') != std::string::npos || name.find(',') != std::string::npos) { // Handles the infuriating case of commas and/or double quotes in names
            std::string temp = "\"";

            for(size_t i = 0; i < name.length(); ++i) {
                temp += name[i];

                if(name[i] == '"')
                    temp += '"';
            }

            name = temp + '"';
        }

        file << name << ",";
        file << command.second.address.toStdString() << ",";
        file << command.second.reg.toStdString() << ",";
        file << command.second.data.toStdString() << ",";
        file << command.second.readLength << ",";
        file << command.second.speedMode;
    }

    file.close();

    this->currPath = filePath;
    this->saved = true;

    qDebug() << "";
}

void MainWindow::on_actionSave_triggered()                              // SAVE MENU BUTTON
{
    // OPENING FILE
    if(this->currPath.isEmpty())
        return;

    if(this->saved)
        return;

    if(QMessageBox::question(this, " ", "Save commands to \"" + this->currPath + "\"?") == QMessageBox::No)
        return;

    qDebug() << "SAVING CSV FILE";

    std::ofstream file(this->currPath.toStdWString().c_str());

    if(file.is_open()) {
        qDebug().nospace() << "SAVED " << this->currPath << "!";
        ui->statusbar->showMessage("Saved \"" + this->currPath + "\"!", 5000);
    }
    else {
        qDebug().nospace() << "Failed to save " << this->currPath << "!";
        QMessageBox::warning(this, " ", "Failed to save \"" + this->currPath + "\"!");
        return;
    }

    // WRITING FILE
    file << titleLine;

    for(std::pair<const QString, Command>& command : this->commands) {
        file << "\n";

        std::string name = command.first.toStdString();

        if(name.find('"') != std::string::npos || name.find(',') != std::string::npos) { // Handles the infuriating case of commas and/or double quotes in names
            std::string temp = "\"";

            for(size_t i = 0; i < name.length(); ++i) {
                temp += name[i];

                if(name[i] == '"')
                    temp += '"';
            }

            name = temp + '"';
        }

        file << name << ",";
        file << command.second.address.toStdString() << ",";
        file << command.second.reg.toStdString() << ",";
        file << command.second.data.toStdString() << ",";
        file << command.second.readLength << ",";
        file << command.second.speedMode;
    }

    file.close();
    this->saved = true;

    qDebug() << "";
}

void MainWindow::on_actionReconnect_Device_triggered()                 // RECONNECT DEVICE MENU BUTTON
{
    qDebug().nospace() << "CLOSING CH341 DEVICE #" << this->deviceNum << "\n";
    CH341CloseDevice(this->deviceNum);

    this->hide();

    int deviceNum = -1;
    DeviceSelect* deviceSelect = new DeviceSelect;
    deviceSelect->exec();

    deviceNum = deviceSelect->deviceNum;
    delete deviceSelect;

    if(deviceNum == -1)
        QApplication::exit();
    else {
        this->deviceNum = deviceNum;
        this->show();
    }
}

void MainWindow::on_actionAbout_Device_triggered()                      // ABOUT DEVICE MENU BUTTON
{
    std::ostringstream oss;

    oss << "Device Number: " << this->deviceNum << "\n";
    oss << "Driver Version: " << CH341GetDrvVersion() << "\n";
    oss << "Library Version: " << CH341GetVersion();

    QMessageBox::information(this, " ", QString::fromStdString(oss.str()));
}

void MainWindow::on_actionExit_triggered()                              // EXIT MENU BUTTON
{
    QApplication::exit();
}
