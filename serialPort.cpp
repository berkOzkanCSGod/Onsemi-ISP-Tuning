#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::findPorts(){
    ports =  QSerialPortInfo::availablePorts();
    ui->com_select->clear();
    for(int i = 0; i < ports.length(); i++) {
        ui->com_select->addItem(ports[i].portName());
    }
}

bool MainWindow::connectPort(QString comName, int baud_rate){

    if (port->isOpen())
        return true;

    port->setPortName(comName);
    port->setBaudRate(baud_rate);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);

    if (port->open(QIODevice::ReadWrite)) {
        return true;
    } else {
        return false;
    }
}

void MainWindow::readSerial() {
    readData += port->readAll();

    QString hexString = readData.toHex();

    if (hexString.isEmpty()) {
        hexString = "Error!";
        return;
    }

    hexString.remove("4f4b0a", Qt::CaseInsensitive);
    hexString.rightJustified(4, '0');
    hexString = hexString.toUpper();
    hexString = hexString.right(2) + hexString.left(2);


    readString = hexString;
    qDebug() << "Hex: " << hexString;

    ui->reg_data_out->setText("0x" + hexString);

    hexString.clear();
    readData.clear();
}

void MainWindow::on_refresh_port_clicked()
{
    findPorts();
}

void MainWindow::on_open_port_clicked()
{
    if (ui->baud_input->text().isEmpty()){
        QMessageBox::information(this, "Input Required", "Please enter a baud rate to continue.");
    } else if (ui->com_select->currentIndex() == -1){
        QMessageBox::information(this, "Input Required", "Please select a COM PORT to continue.");
    } else {
        if (port->isOpen()){
            ui->port_status->setText("OFFLINE");
            ui->port_status->setStyleSheet("color: red; background-color: black; padding: 5px;border: none;outline: none;");
            ui->open_port->setText("Open");
            QObject::disconnect(timer, &QTimer::timeout, this, &MainWindow::checkConnection);
            QObject::disconnect(port, &QSerialPort::readyRead, this, &MainWindow::readSerial);
            port->close(); //just in case
            return;
        }
        connected = connectPort(ui->com_select->currentText(),(ui->baud_input->text()).toInt());
        if (!connected){
            QMessageBox::critical(this, "Error", "Application could not connect to COM PORT " + ui->com_select->currentText());
            return;
        } else {

            ui->open_port->setText("Close");
            ui->port_status->setText("ONLINE");
            ui->port_status->setStyleSheet("color: green;background-color: black;padding: 5px;border: none;outline: none;");
            QObject::connect(timer, &QTimer::timeout, this, &MainWindow::checkConnection);
            QObject::connect(port, &QSerialPort::readyRead, this, &MainWindow::readSerial);
        }
    }

}

void MainWindow::checkConnection(){
    connected = connectPort(ui->com_select->currentText(),(ui->baud_input->text()).toInt());
    if (connected){
        ui->port_status->setText("ONLINE");
        ui->port_status->setStyleSheet("color: green;background-color: black;padding: 5px;border: none;outline: none;");
    } else {
        ui->port_status->setText("OFFLINE");
        ui->port_status->setStyleSheet("color: red; background-color: black;padding: 5px;border: none;outline: none;");
        QObject::disconnect(port, &QSerialPort::readyRead, this, &MainWindow::readSerial);
        QObject::disconnect(timer, &QTimer::timeout, this, &MainWindow::checkConnection);
    }
}

void MainWindow::on_execute_cmd_clicked()
{
    if (connected) {
        QString data = ui->data_inp->text().rightJustified(8,'0');
        QString cmd = createSendData(ui->addr_inp->text(),data,ui->cmd_select->currentText());
        ui->cmd_hex_out->setText(cmd);
        if (sendCmd(serialData) != NULL)
            addToDB(ui->addr_inp->text(),ui->data_inp->text(),ui->cmd_select->currentText());
    } else {
        QMessageBox::critical(this, "Not Connected", "Connect to port before executing operation!");
    }
}

QString MainWindow::createSendData(QString addr_inp, QString data_inp, QString cmd_select) {

    serialData.clear();

    QString syncWord = "AB";
    QString cmdType = "000";
    QString address = addr_inp;
    QString data = data_inp;


    if (cmd_select == "Read") {
        cmdType[0] = '1'; // Set the most significant bit to 1 for read command
        cmdType[2] = '1';
    } else if (cmd_select == "2 Byte Write") {
        cmdType[2] = '1'; // Set the first bit to 1 for 2-byte write command
    } else if (cmd_select == "3 Byte Write") {
        cmdType[1] = '1'; // Set the first bit to 1 for 3-byte write command
    } else if (cmd_select == "4 Byte Write") {
        cmdType[2] = '1'; // Set the first bit to 1 for 4-byte write command
        cmdType[1] = '1'; // Set the second bit to 1 for 4-byte write command
    }

    serialData.append(static_cast<char>(syncWord.toUInt(nullptr, 16)));
    serialData.append(static_cast<char>(cmdType.toUInt(nullptr, 2)));
    serialData.append(static_cast<char>(address.toUInt(nullptr, 16) & 0xFF));
    serialData.append(static_cast<char>((address.toUInt(nullptr, 16) >> 8) & 0xFF));

    int maxLength = 8;

    while (data.length() < maxLength) {
        data.prepend('0');
    }

    QByteArray BA;
    for (int i = data.length() - 2; i >= 0; i -= 2) {
        QString pair = data.mid(i, 2); // Extract a pair of characters
        bool ok;
        quint8 value = pair.toUInt(&ok, 16); // Convert the pair to a uint8 value
        if (ok) {
            BA.append(value); // Append the value to the QByteArray
        } else {
            // Handle conversion error
            qDebug() << "Failed to convert pair" << pair << "to uint8";
            break;
        }
    }
    serialData.append(BA);

    if (cmdType == "111") {
        serialData.append(static_cast<char>((data.toUInt(nullptr, 16) >> 16) & 0xFF));
        serialData.append(static_cast<char>((data.toUInt(nullptr, 16) >> 24) & 0xFF));
    } else {/*
        serialData.append(static_cast<char>(0x00));
        serialData.append(static_cast<char>(0x00))*/;
    }

    return serialData.toHex().toUpper();
}

QByteArray MainWindow::sendCmd(QByteArray data){
    port->write(data);
    if (!port->waitForBytesWritten(250))
    {
        qDebug() << "Failed to send command";
        return QByteArray();
    } else {
        qDebug() << "Command Sent.";
        port->waitForReadyRead(100);
    }

    return QByteArray();
}

