#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::connectToDB(){
    db.setDatabaseName(dbName);
    if (db.open()) {
        findDBtables("db_select_inp");
    } else {
        QMessageBox::critical(this, "Error", "Could NOT connect to DB!");
    }
}

void MainWindow::addToDB(QString addr_inp, QString data_inp, QString cmd_select)
{

    if (!addr_inp.startsWith("0x"))
        addr_inp = "0x" + addr_inp;

    QStringList tableNames = db.tables(); // Get a list of all table names in the database

    bool addressExists = false; // Flag to track if the address exists in any table

    for (const QString& tableName : tableNames) {
        QSqlQuery query;
        query.prepare("SELECT * FROM " + tableName + " WHERE address = ?");
        query.addBindValue(addr_inp);

        if (query.exec() && query.next()) {
            // The addr_inp exists in the current table, update the row with data_inp
            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE " + tableName + " SET data = ? WHERE address = ?");
            updateQuery.addBindValue(data_inp);
            updateQuery.addBindValue(addr_inp);

            if (updateQuery.exec() && updateQuery.isActive()) {
                qDebug() << "Row updated successfully in table: " << tableName;
                addressExists = true;
            } else {
                qDebug() << "Failed to update row in table: " << tableName;
            }
        }
    }

    if (!addressExists) {
        qDebug() << "The address does not exist in any table. Creating a new row is not supported.";
    }
}

void MainWindow::clearTable(QString tableName)
{
    QSqlQuery query;
    query.exec("DELETE FROM "+tableName);
    if (query.isActive()) {
        qDebug() << "Table '"+tableName+"' cleared successfully!";
    } else {
        qDebug() << "Failed to clear table '"+tableName+"'";
    }
}

void MainWindow::findDBtables(QString nameOfTable)
{
    QStringList tableNames;
    QComboBox* comboBox = ui->centralwidget->findChild<QComboBox*>(nameOfTable);

    if (db.isValid() && db.isOpen()) {
        QSqlQuery query(db);

        // Retrieve table names from the database
        query.exec("SELECT name FROM sqlite_master WHERE type='table'");
        while (query.next()) {
            QString tableName = query.value(0).toString();
            tableNames.append(tableName);
        }
    }

    comboBox->clear();
    comboBox->addItems(tableNames);
}

void MainWindow::executeSendCmdForTable(const QString& tableName)
{
    QSqlQuery query;
    query.exec("SELECT * FROM " + tableName);
    int row = 0;
    while (query.next()) {
        QString address = query.value("address").toString();
        QString data_inp = query.value("data").toString();
        QString cmd_select = query.value("size").toString();

        QString sendData = createSendData(address, data_inp.rightJustified(8,'0'), cmd_select);
        QByteArray cmd = QByteArray::fromHex(sendData.toLatin1());
        sendCmd(serialData);

        progressDialog->setValue(row);
        QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents | QEventLoop::ProcessEventsFlag::ExcludeSocketNotifiers);
        row++;
    }
}

bool MainWindow::updateTableColData(const QString &tableName, const QString& dataValue)
{
    QSqlQuery query;

    // Update the data column in the tableName table
    query.prepare("UPDATE "+tableName+" SET data = ?");
    query.addBindValue(dataValue);

    // Execute the query and check for success
    if (query.exec() && query.isActive()) {
        qDebug() << tableName+"data column updated successfully!";
        return true;
    } else {
        qDebug() << "Failed to update data column!";
        return false;
    }
}

//Import/Export
void MainWindow::exportAllToCSV(const QString& exportFolderPath)
{
    QStringList tableNames = db.tables(); // Get a list of all table names in the database

    QString folderPath = exportFolderPath + "/cam_settings";
    QDir().mkdir(folderPath); // Create the "cam_settings" folder

    for (const QString& tableName : tableNames) {
        QString fullFilePath = QDir::toNativeSeparators(folderPath + "/" + tableName + ".csv");
        exportToCSV(tableName, fullFilePath);
    }

    qDebug() << "All tables exported to CSV in folder:" << folderPath;
}

void MainWindow::exportToCSV(const QString& tableName, const QString& filePath)
{
    QSqlQuery query;
    query.exec("SELECT * FROM " + tableName);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        // Write column headers
        QSqlRecord record = query.record();
        int columnCount = record.count();
        for (int i = 0; i < columnCount; ++i) {
            stream << record.fieldName(i);
            if (i < columnCount - 1) {
                stream << ",";
            }
        }
        stream << "\n";

        // Write table data
        while (query.next()) {
            for (int i = 0; i < columnCount; ++i) {
                stream << query.value(i).toString();
                if (i < columnCount - 1) {
                    stream << ",";
                }
            }
            stream << "\n";
        }

        file.close();
        qDebug() << "Table" << tableName << "exported to" << filePath;
    } else {
        qDebug() << "Failed to export table" << tableName << "to" << filePath;
    }
}

void MainWindow::exportTableToCSV(const QString& tableName)
{
    QString selectedFolderPath = QFileDialog::getExistingDirectory(this, "Select Folder");

    if (!selectedFolderPath.isEmpty()) {
        QString fullFilePath = QDir::toNativeSeparators(selectedFolderPath + "/" + tableName + ".csv");

        QSqlQuery query;
        query.exec("SELECT * FROM " + tableName);

        QFile file(fullFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);

            // Write column headers
            QSqlRecord record = query.record();
            int columnCount = record.count();
            for (int i = 0; i < columnCount; ++i) {
                stream << record.fieldName(i);
                if (i < columnCount - 1) {
                    stream << ",";
                }
            }
            stream << "\n";

            // Write table data
            while (query.next()) {
                for (int i = 0; i < columnCount; ++i) {
                    stream << query.value(i).toString();
                    if (i < columnCount - 1) {
                        stream << ",";
                    }
                }
                stream << "\n";
            }

            file.close();
            qDebug() << "Table" << tableName << "exported to" << fullFilePath;
        } else {
            qDebug() << "Failed to export table" << tableName << "to" << fullFilePath;
        }
    } else {
        // User canceled the folder selection or no folder was selected
    }
}

void MainWindow::importTableFromCSV(const QString& tableName)
{
    QString selectedFilePath = QFileDialog::getOpenFileName(this, "Select CSV File", QString(), "CSV Files (*.csv)");

    if (!selectedFilePath.isEmpty()) {
        QFile file(selectedFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QSqlQuery query;

            // Delete all items in the current table
            query.exec("DELETE FROM " + tableName);

            QTextStream stream(&file);
            QString line = stream.readLine(); // Skip the column headers

            while (!stream.atEnd()) {
                line = stream.readLine();
                QStringList values = line.split(',');

                if (values.size() >= 3) {
                    QString address = values[0].trimmed();
                    QString cmd_select = values[1].trimmed();
                    QString data_inp = values[2].trimmed();

                    // Insert the imported data into the current table
                    query.prepare("INSERT INTO "+tableName+" (address, size, data) VALUES (?, ?, ?)");
                    query.addBindValue(address);
                    query.addBindValue(cmd_select);
                    query.addBindValue(data_inp);
                    if (!query.exec()) {
                        qDebug() << "Failed to insert data into the " + tableName;
                    }
                }
            }

            file.close();
            qDebug() << "Table" << tableName << "imported from" << selectedFilePath << "to "+tableName+" table.";
        } else {
            qDebug() << "Failed to open the selected CSV file!";
        }
    } else {
        // User canceled the file selection or no file was selected
    }
}

void MainWindow::importFromCSV(const QString& importFolderPath)
{
    QStringList tableNames = db.tables();

    QString folderPath = importFolderPath;
    QDir directory(folderPath);

    if (directory.exists()) {
        QStringList csvFiles = directory.entryList(QStringList() << "*.csv", QDir::Files);
        for (const QString& csvFile : csvFiles) {
            QString tableName = csvFile.left(csvFile.lastIndexOf('.'));
            if (tableNames.contains(tableName)) {
                QString fullFilePath = QDir::toNativeSeparators(folderPath + "/" + csvFile);
                importFromCSVToTable(tableName, fullFilePath);
            }
        }

        qDebug() << "Tables updated from CSV files in folder:" << folderPath;
    } else {
        qDebug() << "Folder not found:" << folderPath;
    }
}

void MainWindow::importFromCSVToTable(const QString& tableName, const QString& filePath)
{
    QSqlQuery query;
    query.exec("DELETE FROM " + tableName);

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QStringList columnHeaders = stream.readLine().split(',');

        QString insertQuery = "INSERT INTO " + tableName + " (";
        for (const QString& header : columnHeaders) {
            insertQuery += header + ",";
        }
        insertQuery.chop(1); // Remove the last comma
        insertQuery += ") VALUES (";

        for (const QString& header : columnHeaders) {
            insertQuery += ":" + header + ",";
        }
        insertQuery.chop(1); // Remove the last comma
        insertQuery += ")";

        query.prepare(insertQuery);

        while (!stream.atEnd()) {
            QStringList rowValues = stream.readLine().split(',');

            for (int i = 0; i < rowValues.size(); ++i) {
                query.bindValue(":" + columnHeaders[i], rowValues[i]);
            }

            if (!query.exec()) {
                qDebug() << "Failed to insert row into table" << tableName;
                qDebug() << "Error:" << query.lastError().text();
                break;
            }
        }

        file.close();
        qDebug() << "Table" << tableName << "updated from" << filePath;
    } else {
        qDebug() << "Failed to open CSV file for table" << tableName << ":" << filePath;
    }
}

void MainWindow::on_db_export_clicked()
{
    exportTableToCSV(ui->db_select_inp->currentText());
}

void MainWindow::on_db_import_clicked()
{
    importTableFromCSV(ui->db_select_inp->currentText());
}

void MainWindow::on_exportAll_clicked()
{
    QString selectedFolderPath = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (!selectedFolderPath.isEmpty()) {
        exportAllToCSV(selectedFolderPath);
    } else {
        // User canceled the folder selection or no folder was selected
    }

}

void MainWindow::on_importAll_clicked()
{
    QString selectedFolderPath = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (!selectedFolderPath.isEmpty()) {
        importFromCSV(selectedFolderPath);
    } else {
        // User canceled the folder selection or no folder was selected
    }

}
