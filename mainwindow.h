#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QVariant>
#include <QThread>
#include <QWidget>
#include <QLayout>
#include <iostream>
#include <QSlider>
#include <QLineEdit>
#include <QtSerialPort/QSerialPort>
#include <QtSql/QSqlDatabase>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QtSql/QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QSqlRecord>
#include <QFileDialog>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QtSerialPort/QSerialPortInfo>
#include <QProgressDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString hasAccessToThisFile;


private slots:
    void readSerial();
    void on_refresh_port_clicked();
    void on_open_port_clicked();
    void on_execute_cmd_clicked();
    void onAboutToQuit();
    void on_db_export_clicked();
    void on_db_import_clicked();
    void on_reset_clicked();
    void on_updateTable_clicked();
    void on_table_select_inp_activated(int index);
    void on_exportAll_clicked();
    void on_importAll_clicked();
    void on_loadTable_clicked();
    void on_tableExe_clicked();

    void on_readAll_clicked();

    void on_clr_data_clicked();

    void on_min_data_clicked();

    void on_plus_data_clicked();

    void handleSearch(const QString &text);
private:

    //Ports
    void findPorts();
    bool connectPort(QString comName, int baud_rate);
    void checkConnection();
    QString createSendData(QString addr_inp, QString data_inp, QString cmd_select);
    QByteArray sendCmd(QByteArray data);

    //Database
    void connectToDB();
    void addToDB(QString addr_inp, QString data_inp, QString cmd_select);
    void clearTable(QString tableName); //database table
    void findDBtables(QString nameOfTable);
    void executeSendCmdForTable(const QString &tableName);
    bool updateTableColData(const QString &tableName, const QString &dataValue);

    //Export/Import
    void exportTableToCSV(const QString &tableName);
    void exportAllToCSV(const QString &folderPath);
    void exportToCSV(const QString &tableName, const QString &folderPath);
    void importTableFromCSV(const QString &tableName);
    void importFromCSV(const QString &importFolderPath);
    void importFromCSVToTable(const QString &tableName, const QString &filePath);

    //Table
    void init_table_view();
    void filterTableData(const QString &searchText);
    void loadWidgetTable();
    void sliderActivate(int row, QString address, int value, QString cmd_select);
    QString scaleIntToHex(double value, QString cmd_select);
    double scaleHexToInt(QString hexString, QString cmd_select);
    void clearTableWidget();
    void saveTableData();
    void readAll();



    //main window
    Ui::MainWindow *ui;
    //port info
    QSerialPort* port;
    QString portName = "";
    QByteArray serialData;
    QByteArray readData;
    QString readString;
    QList<QSerialPortInfo> ports;
    bool connected = false;
    //timer
    QTimer* timer;
    //DB
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    bool connectedDB = false;
    QString dbName = "register_db.sqlite";
    //table
    QString selectedTable;

    //progress
    QProgressDialog* progressDialog;


};
#endif // MAINWINDOW_H
