#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::onAboutToQuit);
    findPorts(); //init ports
    port = new QSerialPort(this);
    //init timer:
    timer = new QTimer(this);
    timer->start(1000);
    //connect to db
    connectToDB();
    //table
    init_table_view();
    selectedTable = ui->table_select_inp->currentText();




}

MainWindow::~MainWindow()
{
    delete ui;
    delete port;
    delete timer;

}

void MainWindow::onAboutToQuit()
{
    ports.clear();
    if (port->isOpen())
        port->close();
    if (db.isOpen())
        db.close();
}

void MainWindow::on_reset_clicked()
{
    serialData.clear();

    QString cmd = createSendData("521A", "FFFFFFFF","2 Byte Write");
    ui->cmd_hex_out->setText(cmd);
    sendCmd(serialData);

    serialData.clear();
}








void MainWindow::on_clr_data_clicked()
{
    ui->data_inp->setText("00000000");
}


void MainWindow::on_min_data_clicked()
{
    QString hexString = ui->data_inp->text().rightJustified(8,'0').toUpper();
    bool ok;
    quint32 value = hexString.toUInt(&ok, 16);

    if (ok) {
        value--;
        value &= 0xFFFFFFFF;
        QString updatedHexString = QString("%1").arg(value, 8, 16, QChar('0')).toUpper();
        ui->data_inp->setText(updatedHexString);
    }
    else {
        qDebug() << "Invalid Hex String";
    }
}



void MainWindow::on_plus_data_clicked()
{
    QString hexString = ui->data_inp->text().rightJustified(8,'0').toUpper();
    bool ok;
    quint32 value = hexString.toUInt(&ok, 16);

    if (ok) {
        value++;
        value &= 0xFFFFFFFF;
        QString updatedHexString = QString("%1").arg(value, 8, 16, QChar('0')).toUpper();
        ui->data_inp->setText(updatedHexString);
    }
    else {
        qDebug() << "Invalid Hex String";
    }
}

