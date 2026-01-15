#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "message.h"
#include "filemessage.h"
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QFileInfo>
#include <QScrollBar>
#include <QCoreApplication>
#include "addipwindow.h"
#include <QStandardPaths>
#include "notificationmanager.h"
#include <QDirIterator>
#include <QVector>
#include <QCloseEvent>
#include <QCoreApplication>

QByteArray get_data_string(QTcpSocket* socket)
{
    int deadEndCounter = 0;
    QByteArray output;

    while(deadEndCounter < 4096)
    {
        QByteArray data = socket->read(1);
        char c = reinterpret_cast<char>(data[0]);
        if (c == '\n') break;

        output += data[0];

        deadEndCounter++;
    }

    if (deadEndCounter >= 4096)
    {
        return "{}";
    }

    return output;
}

QString formatByteSpeed(qint64 bytes)
{
    QStringList byteFormats {"B", "KB", "MB", "GB", "TB"};
    int byteIndex = 0;
    double speed = bytes;

    qint64 terraSize = 1099511627776;
    qint64 gigaSize = 1073741824;
    qint64 megaSize = 1047576;
    qint64 kilaSize = 1024;

    if (bytes > terraSize) {speed = (double)bytes / (double)terraSize; byteIndex = 4;}
    else if (bytes > gigaSize) {speed = (double)bytes / (double)gigaSize; byteIndex = 3;}
    else if (bytes > megaSize) {speed = (double)bytes / (double)megaSize; byteIndex = 2;}
    else if (bytes > kilaSize) {speed = (double)bytes / (double)kilaSize; byteIndex = 1;}

    return QString::number(speed, 'f', 2) + " " + byteFormats[byteIndex];
}

void MainWindow::UpdateLastIp()
{
    QFile file(cache_file_path + "/lastIp.txt");
    if (file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
    {
        QTextStream out(&file);
        out << ui->ipSelector->currentText();
        file.close();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->RemoveFileButton->setVisible(false);
    socket = new QTcpSocket(this);
    connect(ui->MessageInput, &QLineEdit::returnPressed, ui->sendButton, &QPushButton::click);
    connect(ui->PairIDInput, &QLineEdit::returnPressed, ui->pairButton, &QPushButton::click);
    ui->MessageList->setWordWrap(true);   // allow wrapping
    ui->MessageList->setResizeMode(QListView::Adjust);  // items adjust width

    notificationManager = new NotificationManager(this);

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]{
        if (notificationManager) notificationManager->shutdown();
    });

    QString document_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    cache_file_path = document_path + "/DirectFilePushClient/cache";

    QDir dir;
    dir.mkdir(document_path + "/DirectFilePushClient");
    dir.mkdir(cache_file_path);

    QFile currentIpFile(cache_file_path + "/lastIp.txt");
    QString lastIp;

    if (currentIpFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&currentIpFile);
        lastIp = in.readLine().trimmed();  // trim removes spaces, \n, and \r
        currentIpFile.close();
    }

    // Load IP list
    QFile file(cache_file_path + "/iplist.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty())
                ui->ipSelector->addItem(line);
        }
        file.close();
    }

    // Apply last IP
    if (!lastIp.isEmpty()) {
        int index = ui->ipSelector->findText(lastIp, Qt::MatchExactly);
        if (index != -1)
            ui->ipSelector->setCurrentIndex(index);
        else {
            ui->ipSelector->addItem(lastIp);
            ui->ipSelector->setCurrentText(lastIp);
        }
    }

    connect(ui->ipSelector, &QComboBox::currentTextChanged,
            this, &MainWindow::UpdateLastIp);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, [this]() { 
        currentFileMessage->setStatus(
            "Transferring: (" + formatByteSpeed(current_file_size) +
            " / " + formatByteSpeed(current_total_file_size) +
            ") @ " + formatByteSpeed(bytesReceivedThisSecond) + "/s" +
            (filesToRecieve > 0? " Files Left: " + QString::number(filesToRecieve) : "")
            );
        bytesReceivedThisSecond = 0;
    });

    connect(socket, &QTcpSocket::connected, this, [this]
            {
                ui->ipSelector->setVisible(false);
                ui->connectButton->setText("Disconnect");
            });

    connect(socket, &QTcpSocket::readyRead, this, [this]() {

        // Drain buffered data. This prevents "tiny files" from stalling when
        // file bytes + next JSON line arrive in the same TCP packet.
        while (true)
        {
            // =========================
            // MODE 0: read client ID int
            // =========================
            if (mode == 0)
            {
                // Only read exactly sizeof(int). Do NOT readAll() or you can
                // accidentally consume buffered JSON that arrived right after the id.
                if (socket->bytesAvailable() < (qint64)sizeof(int))
                    return;

                QByteArray data = socket->read(sizeof(int));
                int id = 0;
                memcpy(&id, data.constData(), sizeof(int));

                if (id == -1) {
                    ui->statusbar->showMessage("Server Full", 3000);
                }

                ui->ClientID->setText("Client ID: " + QString::number(id));
                myId = id;
                mode = 1;

                // Keep looping in case JSON is already buffered
                continue;
            }

            // =========================
            // MODE 1: newline-delimited JSON control messages
            // =========================
            if (mode == 1)
            {
                // Only parse when we have a full line ending in '\n'
                if (!socket->canReadLine())
                    return;

                QByteArray line = socket->readLine();
                line = line.trimmed(); // remove '\n' and possible '\r'

                if (line.isEmpty())
                    continue;

                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

                if (parseError.error != QJsonParseError::NoError)
                {
                    QMessageBox::information(this, "Json Error",
                                             "Error parsing json: " + parseError.errorString() +
                                                 "\nRaw: " + QString::fromUtf8(line));
                    // Skip bad line and continue draining
                    continue;
                }

                QJsonObject obj = doc.object();
                int fromId = obj["from"].toInt();
                const QString type = obj["type"].toString();

                if (type == "request")
                {
                    QMessageBox::StandardButton reply;

                    reply = QMessageBox::question(
                        this,
                        "Request to Pair",
                        "The client " + QString::number(fromId) + " has sent a request to pair.",
                        QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes)
                    {
                        QJsonObject outObj;
                        outObj["type"] = "accept";
                        outObj["to"] = fromId;

                        QJsonDocument outDoc(outObj);
                        QString jsonMessage = outDoc.toJson(QJsonDocument::Compact) + "\n";

                        ui->PairIDInput->setText("Paired to " + QString::number(fromId));
                        ui->PairIDInput->setReadOnly(true);
                        ui->pairButton->setText("Un-Pair");

                        socket->write(jsonMessage.toUtf8());
                        pairPartnerId = fromId;
                    }
                    else
                    {
                        QJsonObject outObj;
                        outObj["type"] = "reject";
                        outObj["to"] = fromId;

                        QJsonDocument outDoc(outObj);
                        QString jsonMessage = outDoc.toJson(QJsonDocument::Compact) + "\n";

                        socket->write(jsonMessage.toUtf8());
                    }

                    continue; // keep draining
                }
                else if (type == "accept")
                {
                    pairPartnerId = fromId;
                    ui->PairIDInput->setText("Paired to " + QString::number(fromId));
                    ui->PairIDInput->setReadOnly(true);
                    ui->pairButton->setText("Un-Pair");
                    continue;
                }
                else if (type == "un-pair")
                {
                    if (fromId != pairPartnerId)
                    {
                        QMessageBox::information(this, "Warning", "An Unrecognized client tried to un-pair you!");
                        return;
                    }

                    ui->PairIDInput->setText("");
                    ui->PairIDInput->setReadOnly(false);
                    ui->pairButton->setText("Pair");
                    pairPartnerId = -1;
                    ui->MessageList->clear();

                    if (current_file) {
                        if (current_file->isOpen())
                            current_file->close();
                        delete current_file;
                        current_file = nullptr;
                    }

                    fileQueue.clear();
                    sendingFolder = false;
                    filesToRecieve = 0;

                    continue;
                }
                else if (type == "message")
                {
                    if (fromId != pairPartnerId)
                    {
                        QMessageBox::information(this, "Warning", "An Unrecognized client tried to send you a message!");
                        return;
                    }

                    QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
                    message *newMessage = new message;

                    newMessage->setId("Client " + QString::number(fromId));
                    newMessage->setText(obj["message"].toString());

                    item->setSizeHint(newMessage->sizeHint());

                    ui->MessageList->addItem(item);
                    ui->MessageList->setItemWidget(item, newMessage);
                    scrollToBottom();

                    notificationManager->notifyIfNotFocused("Message from Client " + QString::number(fromId),
                                                            obj["message"].toString());
                    continue;
                }
                else if (type == "file_metadata")
                {
                    if (fromId != pairPartnerId)
                    {
                        QMessageBox::information(this, "Warning", "An Unrecognized client tried to send you a file!");
                        return;
                    }

                    if (!sendingFolder)
                    {
                        QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
                        currentFileMessage = new fileMessage;
                        item->setSizeHint(currentFileMessage->sizeHint());
                        ui->MessageList->addItem(item);
                        ui->MessageList->setItemWidget(item, currentFileMessage);
                        scrollToBottom();
                    }

                    currentFileMessage->setFileName(obj["file_name"].toString());
                    currentFileMessage->setToAndFromClient(QString::number(fromId), QString::number(myId));
                    currentFileMessage->setStatus("Transferring");
                    currentFileMessage->setProgress(0);

                    current_total_file_size = obj["file_size"].toInteger();
                    current_file_size = 0;
                    current_file_name = obj["file_name"].toString();

                    if (!sendingFolder)
                    {
                        notificationManager->notifyIfNotFocused("Recieving File",
                                                                "Recieving " + obj["file_name"].toString() + " " +
                                                                    formatByteSpeed(current_total_file_size));
                    }

                    // Ensure Files-Received exists
                    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/Files-Received");
                    transfer_file_path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/Files-Received";

                    // NOTE: your path logic kept as-is
                    current_file = new QFile(transfer_file_path + "/" +
                                             (sendingFolder ? root_folder_name + current_file_name : current_file_name));

                    if (!current_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        QMessageBox::warning(this, "Error", "Could not open file for writing!");
                        return;
                    }

                    bytesReceivedThisSecond = 0;
                    updateTimer->start(1000); // back to 1s; if you want 10ms, change it, but 1000 makes sense for speed display

                    // Reset progress-tracking for new file
                    lastProgress = -1; // <-- you need this member int in your class, see note below

                    mode = 2;

                    // CRITICAL: do NOT return; file bytes might already be buffered.
                    continue;
                }
                else if (type == "directory_builder")
                {
                    if (fromId != pairPartnerId)
                    {
                        QMessageBox::information(this, "Warning", "An Unrecognized client tried to send you a file!");
                        return;
                    }

                    QJsonArray directories = obj["directories"].toArray();
                    QStringList folders;
                    folders.reserve(directories.size());
                    for (int i = 0; i < directories.size(); ++i)
                        folders.append(directories.at(i).toString());

                    QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
                    currentFileMessage = new fileMessage;
                    item->setSizeHint(currentFileMessage->sizeHint());
                    ui->MessageList->addItem(item);
                    ui->MessageList->setItemWidget(item, currentFileMessage);
                    scrollToBottom();

                    filesToRecieve = obj["file_count"].toInt();
                    sendingFolder = true;

                    transfer_file_path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/Files-Received";

                    QString rootPath = transfer_file_path + "/" + obj["root_path"].toString();
                    root_folder_name = obj["root_path"].toString();

                    // Use mkpath so nested dirs always create
                    QDir().mkpath(rootPath);
                    for (const QString &folder : folders)
                        QDir().mkpath(rootPath + folder);

                    if (filesToRecieve <= 0)
                    {
                        currentFileMessage->setFileName("");
                        currentFileMessage->setStatus("Directories Finished Transfering!");
                        currentFileMessage->setProgress(100);
                    }

                    continue;
                }
                else if (type == "file_transfer_finished")
                {
                    currentFileMessage->setStatus("File Transfered");

                    if (filesToRecieve <= 0) {
                        sendingFolder = false;
                    } else {
                        filesToRecieve -= 1;
                    }

                    continue;
                }

                // Unknown / unhandled type: just continue draining
                continue;
            }

            // =========================
            // MODE 2: raw file bytes
            // =========================
            if (mode == 2)
            {
                // Consume as many raw bytes as are currently buffered, up to the file size.
                while (socket->bytesAvailable() > 0 && current_file_size < current_total_file_size)
                {
                    qint64 remaining = current_total_file_size - current_file_size;
                    qint64 toRead = qMin<qint64>(CHUNK_SIZE, remaining);

                    QByteArray bytes = socket->read(toRead);
                    if (bytes.isEmpty())
                        break;

                    if (current_file && current_file->isOpen())
                    {
                        qint64 written = current_file->write(bytes);
                        if (written != bytes.size())
                            qWarning() << "Short write to file: wrote" << written << "expected" << bytes.size();

                        flushIndex++;
                        if (flushIndex >= 125) {
                            current_file->flush();
                            flushIndex = 0;
                        }
                    }

                    current_file_size += bytes.size();
                    bytesReceivedThisSecond += bytes.size();

                    // progress update
                    double progress = (double)current_file_size / (double)current_total_file_size * 100.0;
                    int intProgress = static_cast<int>(progress);

                    if (intProgress != lastProgress) {
                        currentFileMessage->setProgress(intProgress);
                        lastProgress = intProgress;
                    }
                }

                // Finished file?
                if (current_file_size >= current_total_file_size)
                {
                    if (current_file) {
                        current_file->flush();
                        current_file->close();
                        delete current_file;
                        current_file = nullptr;
                    }

                    currentFileMessage->setStatus(formatByteSpeed(current_total_file_size) + " File Finished Transfering!");
                    updateTimer->stop();

                    if (!sendingFolder)
                    {
                        notificationManager->notifyIfNotFocused(
                            "File Transfer Done",
                            formatByteSpeed(current_total_file_size) + " File Finished Transfering!"
                            );
                    } else
                    {
                        if (filesToRecieve <= 1)
                            notificationManager->notifyIfNotFocused(
                                "Folder Transfer Done",
                                QString::number(filesToRecieve) + " Files Finished Transfering!"
                                );
                    }

                    // reset
                    current_file_size = 0;
                    current_total_file_size = 0;
                    flushIndex = 0;
                    lastProgress = -1;

                    // folder bookkeeping
                    if (filesToRecieve > 0) filesToRecieve -= 1;
                    if (filesToRecieve <= 0) sendingFolder = false;

                    // Switch back to JSON parsing and KEEP DRAINING.
                    mode = 1;
                    continue;
                }

                // Not finished: wait for more bytes to arrive
                return;
            }

            // Unknown mode - stop
            return;
        }
    });

    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "Disconnected from server.";
        ui->connectButton->setText("Connect");
        ui->ipSelector->setVisible(true);
        ui->PairIDInput->setReadOnly(false);
        ui->PairIDInput->setText("");
        ui->pairButton->setText("Pair");
        ui->ClientID->setText("0");
        mode = 0;
        pairPartnerId = -1;

        ui->MessageList->clear();

        sendingFile= false;
        filesToRecieve = 0;

        if (current_file) {
            if (current_file->isOpen())
                current_file->close();
            delete current_file;
            current_file = nullptr;
        }

        ui->sendButton->setEnabled(true);
        ui->pairButton->setEnabled(true);
        ui->MessageInput->setEnabled(true);

        fileQueue.clear();

    });
}

MainWindow::~MainWindow()
{
    if (notificationManager) notificationManager->shutdown();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Force a real quit when user clicks the X
    QCoreApplication::quit();
    event->accept();
}

void MainWindow::on_AddIpButton_clicked()
{
    addIpWindow dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        QString ip = dialog.getIpAddress();
        QString port = dialog.getPort();
        QString entry = QString("%1:%2").arg(ip, port);

        ui->ipSelector->addItem(entry);
        ui->ipSelector->setCurrentIndex(ui->ipSelector->count() - 1);

        // Make sure cache directory exists
        QDir dir;
        if (!dir.exists(cache_file_path))
            dir.mkpath(cache_file_path);

        // Open the file for appending
        QFile file(cache_file_path + "/iplist.txt");
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << entry << "\n";
            file.close();
        } else {
            QMessageBox::information(this, "Opening Error", "file failed to open");
        }
    }
}

void MainWindow::on_fileDialogButton_clicked()
{
    selectedFolderPath = "";
    selectedFilePath = QFileDialog::getOpenFileName(
        this, "Open File", "", "All Files (*.*)");

    if (selectedFilePath.isEmpty()) {
        qDebug() << "Selected file:" << selectedFilePath;
        return;
    }

    ui->FilePathDisplay->setText(selectedFilePath);
    ui->RemoveFileButton->setVisible(true);
}

void MainWindow::on_folderDialogButton_clicked()
{
    selectedFilePath = "";
    selectedFolderPath = QFileDialog::getExistingDirectory();

    ui->FilePathDisplay->setText(selectedFolderPath);
    ui->RemoveFileButton->setVisible(true);
}

void MainWindow::on_RemoveFileButton_clicked()
{
    selectedFilePath = "";
    selectedFolderPath = "";
    ui->FilePathDisplay->setText(selectedFilePath);
    ui->RemoveFileButton->setVisible(false);
}

void MainWindow::on_connectButton_clicked()
{
    if (socket->state() == QAbstractSocket::ConnectedState ||
        socket->state() == QAbstractSocket::ConnectingState)
    {
        socket->disconnectFromHost();
        return;
    }

    QStringList serverInfo = ui->ipSelector->currentText().split(":");
    QString ipAddress = serverInfo[0];
    int port = serverInfo[1].toInt();

    socket->connectToHost(ipAddress, port);

    if (!socket->waitForConnected(3000)) {
        QMessageBox::critical(this, "Error", "Failed to connect: " + socket->errorString());
        return;
    }
}

void MainWindow::on_pairButton_clicked()
{
    filesToRecieve = 0;

    if (pairPartnerId != -1)
    {
        ui->PairIDInput->setText("");
        ui->PairIDInput->setReadOnly(false);
        ui->pairButton->setText("Pair");
        QJsonObject jsonMessage;
        jsonMessage["type"] = "un-pair";
        jsonMessage["to"] = pairPartnerId;
        QJsonDocument doc(jsonMessage);
        QString jsonString = doc.toJson(QJsonDocument::Compact) + '\n';

        socket->write(jsonString.toUtf8());
        pairPartnerId = -1;
        ui->MessageList->clear();
        if (current_file) {
            if (current_file->isOpen())
                current_file->close();
            delete current_file;
            current_file = nullptr;
        }


        return;
    }

    try {
        int targetId = ui->PairIDInput->text().toInt();
        QJsonObject jsonMessage;
        jsonMessage["type"] = "request";
        jsonMessage["target"] = targetId;

        QJsonDocument doc(jsonMessage);
        QString jsonString = doc.toJson(QJsonDocument::Compact) + '\n';
        socket->write(jsonString.toUtf8());
    } catch (...) {
        QMessageBox::information(this, "Type Error", "Please make sure to enter a number.");
    }
}

void MainWindow::scrollToBottom()
{
    QScrollBar* scrollbar = ui->MessageList->verticalScrollBar();
    int distanceFromBottom = scrollbar->maximum() - scrollbar->value();

    const int autoScrollThreshold = 100;

    if (distanceFromBottom < autoScrollThreshold)
        ui->MessageList->scrollToBottom();
}

void MainWindow::sendFile()
{
    if (sendingFolder && fileQueue.isEmpty())
    {
        return;
    }

    if (!fileQueue.isEmpty()) selectedFilePath = fileQueue.dequeue();

    QFileInfo info(selectedFilePath);

    // 1. Send metadata first
    QJsonObject jsonMessage;
    jsonMessage["type"] = "file_metadata";
    jsonMessage["file_name"] = sendingFolder? QString(selectedFilePath).replace(selectedFolderPath, "") : info.fileName();
    jsonMessage["file_size"] = info.size();
    jsonMessage["to"] = pairPartnerId;

    QJsonDocument doc(jsonMessage);
    QString jsonString = doc.toJson(QJsonDocument::Compact) + '\n';
    socket->write(jsonString.toUtf8());

    // 2. Setup state
    current_file_size = 0;
    current_total_file_size = info.size();

    if (!sendingFolder)
    {
        QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
        currentFileMessage = new fileMessage;
        item->setSizeHint(currentFileMessage->sizeHint());
        ui->MessageList->addItem(item);
        ui->MessageList->setItemWidget(item, currentFileMessage);
    }

    currentFileMessage->setFileName(sendingFolder? QString(selectedFilePath).replace(selectedFolderPath, "") : info.fileName());
    currentFileMessage->setToAndFromClient(QString::number(myId), QString::number(pairPartnerId));
    currentFileMessage->setStatus("Transferring");
    currentFileMessage->setProgress(0);

    current_file = new QFile(selectedFilePath);
    if (!current_file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for reading!");
        delete current_file;
        current_file = nullptr;
        return;
    }

    updateTimer->start(1000);
    sendingFile = true;

    if (!ui->HardSendCheck->isChecked())
    {
        // 3. Connect socket signal → chunk sender
        uploadConn = connect(socket, &QTcpSocket::bytesWritten,
                             this, &MainWindow::sendFileChunk);

        // 4. Kick off first chunk immediately

        ui->sendButton->setEnabled(false);
        ui->pairButton->setEnabled(false);
        ui->MessageInput->setEnabled(false);

        scrollToBottom();
        sendFileChunk();
    }
    else
    {
        ui->sendButton->setEnabled(false);
        ui->pairButton->setEnabled(false);
        ui->MessageInput->setEnabled(false);
        scrollToBottom();

        while (!current_file->atEnd())
        {
            QByteArray bytes = current_file->read(CHUNK_SIZE);
            current_file_size += bytes.size();
            bytesReceivedThisSecond += bytes.size();
            qint64 written = socket->write(bytes);

            if (written == -1) {
                QMessageBox::warning(this, "Error", "Failed to write to socket!"); // break;
            }

            if (!socket->waitForBytesWritten(-1)) {
                QMessageBox::warning(this, "Error", "Failed to send data chunk!");
                break;
            }

            double progress = (double)current_file_size / (double)current_total_file_size * 100.0;
            static int lastProgress = -1;
            int intProgress = static_cast<int>(progress);

            if (intProgress != lastProgress) {
                currentFileMessage->setProgress(intProgress);
                lastProgress = intProgress;
            }
            QCoreApplication::processEvents();
        }

        updateTimer->stop();
        if (current_file) {
            if (current_file->isOpen())
            {
                current_file->close();
                delete current_file;
                current_file = nullptr;
            }
        }

        sendingFile = false;
        updateTimer->stop();

        currentFileMessage->setStatus(formatByteSpeed(current_file_size) + " File Finished Transfering!");
        currentFileMessage->setProgress(100);

        ui->sendButton->setEnabled(true);
        ui->pairButton->setEnabled(true);
        ui->MessageInput->setEnabled(true);

        if (!fileQueue.isEmpty()) sendFile();
        else {
            selectedFilePath = "";
            selectedFolderPath = "";
        }
    }
}

void MainWindow::sendFileChunk()
{
    if (!sendingFile || !current_file) return;

    const int chunkSize = CHUNK_SIZE;
    QByteArray bytes = current_file->read(chunkSize);

    if (bytes.isEmpty()) {
        // === Finished ===
        sendingFile = false;
        updateTimer->stop();

        if (current_file->isOpen())
            current_file->close();

        delete current_file;
        current_file = nullptr;

        if (sendingFolder && fileQueue.isEmpty())
            sendingFolder = false;

        // Disconnect so no future writes trigger us
        disconnect(uploadConn);

        currentFileMessage->setStatus(formatByteSpeed(current_file_size) + " File Finished Transfering!");
        currentFileMessage->setProgress(100);

        ui->sendButton->setEnabled(true);
        ui->pairButton->setEnabled(true);
        ui->MessageInput->setEnabled(true);

        if (!fileQueue.isEmpty()) sendFile();
        else {
            selectedFilePath = "";
            selectedFolderPath = "";
        }

        return;
    }

    // Write chunk
    qint64 written = socket->write(bytes);
    if (written == -1) {
        QMessageBox::warning(this, "Error", "Failed to write to socket!");
        return;
    }

    current_file_size += bytes.size();
    bytesReceivedThisSecond += bytes.size();

    // Update progress
    double progress = (double)current_file_size / (double)current_total_file_size * 100.0;
    static int lastProgress = -1;
    int intProgress = static_cast<int>(progress);

    if (intProgress != lastProgress) {
        currentFileMessage->setProgress(intProgress);
        lastProgress = intProgress;
    }
}

void MainWindow::sendMessage(const QString messageToSend)
{
    fileQueue.clear();
    QJsonObject jsonMessage;
    jsonMessage["type"] = "message";
    jsonMessage["message"] = messageToSend;
    jsonMessage["to"] = pairPartnerId;

    QJsonDocument doc(jsonMessage);
    QString jsonString = doc.toJson(QJsonDocument::Compact) + '\n';
    socket->write(jsonString.toUtf8());

    QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
    message *newMessage = new message;

    newMessage->setId("Client " + QString::number(myId));
    newMessage->setText(messageToSend);

    item->setSizeHint(newMessage->sizeHint());
    ui->MessageList->addItem(item);
    ui->MessageList->setItemWidget(item, newMessage);
    scrollToBottom();
}

void MainWindow::sendDirectories()
{
    fileQueue.clear();
    QDirIterator itfolders(selectedFolderPath, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QJsonArray folderPaths;

    QVector<QString> filesToSend;

    while (itfolders.hasNext())
    {
        auto entity = itfolders.nextFileInfo();

        if (entity.isDir()) folderPaths.append(entity.absoluteFilePath().replace(selectedFolderPath, ""));
        else filesToSend.append(entity.filePath());
    }

    QJsonObject jsonMessage;
    jsonMessage["type"] = "directory_builder";
    jsonMessage["root_path"] = selectedFolderPath.split("/").last();
    jsonMessage["directories"] = folderPaths;
    jsonMessage["file_count"] = filesToSend.count();
    jsonMessage["to"] = pairPartnerId;

    ui->statusbar->showMessage(QString::number(filesToSend.count()), 10000);

    QJsonDocument doc(jsonMessage);
    QString jsonString = doc.toJson(QJsonDocument::Compact) + '\n';
    socket->write(jsonString.toUtf8());

    sendingFolder = true;

    QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
    currentFileMessage = new fileMessage;
    item->setSizeHint(currentFileMessage->sizeHint());
    ui->MessageList->addItem(item);
    ui->MessageList->setItemWidget(item, currentFileMessage);

    foreach (QString f, filesToSend)
    {
        fileQueue.enqueue(f);
    }

    if (ui->HardSendCheck->isChecked())
    {
        while (!fileQueue.isEmpty()) {
            sendFile();
        }
    } else sendFile();

    if (filesToSend.count() <= 0)
    {
        currentFileMessage->setFileName("");
        currentFileMessage->setStatus("Directories Finished Transfering!");
        currentFileMessage->setProgress(100);
    }
}

void MainWindow::on_sendButton_clicked()
{
    if (pairPartnerId == -1)
    {
        return;
    }

    QString messageToSend = ui->MessageInput->text();

    messageToSend.remove("\n");

    if (!messageToSend.isEmpty()) sendMessage(messageToSend);
    if (!selectedFilePath.isEmpty()) sendFile();
    else if (!selectedFolderPath.isEmpty()) sendDirectories();

    ui->MessageInput->setText("");
    selectedFilePath = "";
    ui->FilePathDisplay->setText(selectedFilePath);
    ui->RemoveFileButton->setVisible(false);
}
