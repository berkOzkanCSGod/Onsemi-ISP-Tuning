#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::init_table_view()
{
    findDBtables("table_select_inp");

    connect(ui->searchBar, &QLineEdit::textChanged, this, &MainWindow::handleSearch);

}

void MainWindow::handleSearch(const QString &text) {
    QTableWidget *tableWidget = ui->tableWidget;
    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        bool match = tableWidget->item(i, 0)->text().contains(text, Qt::CaseInsensitive);
        tableWidget->setRowHidden(i, !match);
    }
}

void MainWindow::on_loadTable_clicked()
{

    QString tableName = selectedTable;
    QSqlQuery query(db);
    QString queryString = QString("SELECT COUNT(*) FROM %1").arg(tableName);
    int rowCount;

    if (query.exec(queryString)) {
        if (query.next()) {
            rowCount = query.value(0).toInt();
        } else {
            qDebug() << "Failed to retrieve row count.";
        }
    } else {
        qDebug() << "Query execution failed.";
    }

    progressDialog = new QProgressDialog();
    progressDialog->setFixedSize(400,100);
    progressDialog->setLabelText("Loading Data from DB...");
    progressDialog->setCancelButton(nullptr);
    progressDialog->setRange(0, rowCount);
    progressDialog->show();

    loadWidgetTable();

    progressDialog->close();
    delete progressDialog;

}

void MainWindow::on_updateTable_clicked()
{
    QString tableName = selectedTable;
    QSqlQuery query(db);
    QString queryString = QString("SELECT COUNT(*) FROM %1").arg(tableName);
    int rowCount;

    if (query.exec(queryString)) {
        if (query.next()) {
            rowCount = query.value(0).toInt();
        } else {
            qDebug() << "Failed to retrieve row count.";
        }
    } else {
        qDebug() << "Query execution failed.";
    }

    progressDialog = new QProgressDialog();
    progressDialog->setFixedSize(400,100);
    progressDialog->setLabelText("Saving Data to DB...");
    progressDialog->setCancelButton(nullptr);
    progressDialog->setRange(0, rowCount);
    progressDialog->show();

    saveTableData();

    progressDialog->close();
    delete progressDialog;
}

void MainWindow::on_tableExe_clicked()
{
    QString tableName = selectedTable;
    QSqlQuery query(db);
    QString queryString = QString("SELECT COUNT(*) FROM %1").arg(tableName);
    int rowCount;

    if (query.exec(queryString)) {
        if (query.next()) {
            rowCount = query.value(0).toInt();
        } else {
            qDebug() << "Failed to retrieve row count.";
        }
    } else {
        qDebug() << "Query execution failed.";
    }

    progressDialog = new QProgressDialog();
    progressDialog->setFixedSize(400,100);
    progressDialog->setLabelText("Saving Data to DB...");
    progressDialog->setCancelButton(nullptr);
    progressDialog->setRange(0, rowCount);
    progressDialog->show();

    executeSendCmdForTable(selectedTable);

    progressDialog->close();
    delete progressDialog;
}

void MainWindow::on_table_select_inp_activated(int index)
{
    selectedTable = ui->table_select_inp->currentText();
}

void MainWindow::loadWidgetTable(){
    QTableWidget* tbW = ui->tableWidget;
    tbW->setSortingEnabled(false);
    clearTableWidget();

    if (selectedTable.isEmpty()){
        return;
    }

    QSqlQuery query;
    if (query.exec("SELECT address, size, data FROM " + selectedTable)) {

        tbW->setHorizontalHeaderLabels({ "Address", "Data", "Slider" });
        tbW->setColumnCount(3);
        int row = 0;

        while (query.next()) {
            QString address = query.value(0).toString();
            QString cmd_select = query.value(1).toString();
            QString data = query.value(2).toString().toUpper();

            QSlider* sl = new QSlider();
            sl->setOrientation(Qt::Horizontal);
            sl->setAutoFillBackground(true);
            sl->setSingleStep(1);

            unsigned int minValue = 0; // Minimum value of the hexadecimal range
            unsigned int maxValue = 99; // Maximum value of the hexadecimal range
            unsigned int incrementValue = 1; // Specify the increment value in hexadecimal

            sl->setRange(minValue, maxValue);
            sl->setSingleStep(incrementValue);

            connect(sl, &QSlider::valueChanged, this, [this, sl, tbW, address, data, cmd_select](int value) mutable {
                int row = -1;
                for (int i = 0; i < tbW->rowCount(); i++) {
                    QWidget* cellWidget = tbW->cellWidget(i, 2);
                    if (cellWidget == sl) {
                        row = i;
                        break;
                    }
                }
                if (row != -1) {
                    sliderActivate(row, address, value, cmd_select);
                }
            });

//            data = data.leftJustified(8, '0');
            int sliderValue = scaleHexToInt(data, cmd_select);
            sl->setValue(sliderValue);

//            data = data.rightJustified(8,'0');

            tbW->insertRow(tbW->rowCount());

            tbW->setItem(tbW->rowCount()-1,
                         0,
                         new QTableWidgetItem(address));
            tbW->setItem(tbW->rowCount()-1,
                         1,
                         new QTableWidgetItem(data));
            tbW->setCellWidget(tbW->rowCount()-1,
                               2,
                               sl);

            progressDialog->setValue(row);
            QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents | QEventLoop::ProcessEventsFlag::ExcludeSocketNotifiers);

            row++;
        }
    } else {
        qDebug() << "Error executing SQL query: " << query.lastError().text();
    }
}

void MainWindow::clearTableWidget() {
    QTableWidget* tbW = ui->tableWidget;

    // Remove sliders and delete items
    int rowCount = tbW->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        // Retrieve the slider from the cell widget
        QSlider* slider = qobject_cast<QSlider*>(tbW->cellWidget(row, 2));
        if (slider) {
            delete slider; // Delete the slider
        }

        // Delete the QTableWidgetItem
        delete tbW->takeItem(row, 0);
        delete tbW->takeItem(row, 1);
    }

    // Clear the table widget
    tbW->clear();
    tbW->setRowCount(0);
    tbW->setColumnCount(0);
}

void MainWindow::sliderActivate(int row, QString address, int value, QString cmd_select){
    QTableWidget* tbW = ui->tableWidget;
    QTableWidgetItem* item = tbW->item(row, 1);
    delete item;

    QString v = scaleIntToHex(value, cmd_select);
    item = new QTableWidgetItem(v); // Create a new QTableWidgetItem if it doesn't exist
    tbW->setItem(row, 1, item);
    createSendData(address,v.rightJustified(8,'0'),cmd_select);
    sendCmd(serialData);
}

QString MainWindow::scaleIntToHex(double value, QString cmd_select) {
    unsigned int maxValue;
    int justifyVal = 0;
    if (cmd_select == "1 Byte Write") {
        maxValue = 0xFF;
        justifyVal = 2;
    } else if (cmd_select == "2 Byte Write") {
        maxValue = 0xFFFF;
        justifyVal = 4;
    } else if (cmd_select == "3 Byte Write") {
        maxValue = 0xFFFFFF;
        justifyVal = 6;
    } else if (cmd_select == "4 Byte Write") {
        maxValue = 0xFFFFFFFF;
        justifyVal = 8;
    }


    unsigned int scaledValue = static_cast<unsigned int>(value / 99.0 * maxValue);
    QString hexString = QString::number(scaledValue,16);
    hexString = hexString.rightJustified(justifyVal, '0').toUpper();
    qDebug() << "Hexadecimal: " << hexString;
    return hexString;
}

double MainWindow::scaleHexToInt(QString hexString, QString cmd_select) {
    unsigned int maxValue;

    if (cmd_select == "1 Byte Write") {
        maxValue = 0xFF;
    } else if (cmd_select == "2 Byte Write") {
        maxValue = 0xFFFF;
    } else if (cmd_select == "3 Byte Write") {
        maxValue = 0xFFFFFF;
    } else if (cmd_select == "4 Byte Write") {
        maxValue = 0xFFFFFFFF;
    }
    unsigned int hexValue = hexString.toUInt(nullptr, 16);
    double percentage = static_cast<double>(hexValue) / maxValue * 100.0;
//    qDebug() << "Percentage: " << QString::number(percentage, 'f', 2) << "%";
    return percentage;
}

void MainWindow::saveTableData() {
    if (selectedTable.isEmpty()) {
        qDebug() << "No table or selected table is empty.";
        return;
    }

    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE " + selectedTable + " SET data = :data WHERE address = :address");

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QString address = ui->tableWidget->item(row, 0)->text();
        QString data = ui->tableWidget->item(row, 1)->text();
        // Cut off data at 8 characters
        if (data.length() > 8) {
            data = data.left(8).toUpper();
        }

        bool isHex = false;
        data.toUInt(&isHex, 16);
        if (!isHex) {
            data = "00000000";
        }

        updateQuery.bindValue(":data", data);
        updateQuery.bindValue(":address", address);

        if (!updateQuery.exec()) {
            qDebug() << "Error updating data in the database: " << updateQuery.lastError().text();
            return;
        }
        progressDialog->setValue(row);
        QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents | QEventLoop::ProcessEventsFlag::ExcludeSocketNotifiers);
    }

    qDebug() << "Table updated successfully.";
}

void MainWindow::on_readAll_clicked()
{
    if (connected){
        QString tableName = selectedTable;
        QSqlQuery query(db);
        QString queryString = QString("SELECT COUNT(*) FROM %1").arg(tableName);
        int rowCount;

        if (query.exec(queryString)) {
            if (query.next()) {
                rowCount = query.value(0).toInt();
            } else {
                qDebug() << "Failed to retrieve row count.";
            }
        } else {
            qDebug() << "Query execution failed.";
        }
        //^^^ if need to count all rows just put 1980 or first count each table's rows and add them up
        progressDialog = new QProgressDialog();
        progressDialog->setFixedSize(400,100);
        progressDialog->setLabelText("Saving Data to DB...");
        progressDialog->setCancelButton(nullptr);
        progressDialog->setRange(0, rowCount);
        progressDialog->show();

        readAll();

        progressDialog->close();
        delete progressDialog;
    } else {
        QMessageBox::critical(this, "Not Connected", "Connect to port before executing operation!");
    }
}

void MainWindow::readAll()
{
    if (!connected) {
        QMessageBox::critical(this, "Not Connected", "Connect to port before executing operation!");
    }

    selectedTable = ui->table_select_inp->currentText();
    if (selectedTable.isEmpty()){
        return;
    }

    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM " + selectedTable))
    {
        qDebug() << "Error executing SQL query: " << query.lastError().text();
        return;
    }

    while (query.next())
    {
        QString address = query.value(0).toString();
        QString cmd_select = query.value(1).toString();
        QString data = query.value(2).toString();

        QString cmd = createSendData(address, data, "Read");
        sendCmd(serialData);
        QString response = readString;
        qDebug() << "Hex: " << response;
//        response.rightJustified(8,'0');


        QTableWidgetItem* item = new QTableWidgetItem(response);

        int row = query.at();
        delete ui->tableWidget->takeItem(row,1);
        ui->tableWidget->setItem(row, 1, item);

        progressDialog->setValue(row);
        QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents | QEventLoop::ProcessEventsFlag::ExcludeSocketNotifiers);

    }
    saveTableData();
}










