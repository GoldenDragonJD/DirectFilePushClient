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

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, [this]() {
        currentFileMessage->setStatus(
            "Transferring: (" + formatByteSpeed(current_file_size) +
            " / " + formatByteSpeed(current_total_file_size) +
            ") @ " + formatByteSpeed(bytesReceivedThisSecond) + "/s"
            );
        bytesReceivedThisSecond = 0;
    });

    connect(socket, &QTcpSocket::connected, this, [this]
            {
                ui->IpEnter->setVisible(false);
                ui->connectButton->setText("Disconnect");
            });

    connect(socket, &QTcpSocket::readyRead, this, [this]() {
        switch (mode){
        case 0:{
            QByteArray data = socket->readAll();
            int id;
            memcpy(&id, data.constData(), sizeof(int));
            ui->ClientID->setText("Client ID: " + QString::number(id));
            mode = 1;
            myId = id;
            break;
        }
        case 1:{
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(get_data_string(socket), &parseError);

            if (parseError.error != QJsonParseError::NoError)
            {
                QMessageBox::information(this, "Json Error", "Error parsing json: " + parseError.errorString());
            }

            QJsonObject obj = doc.object();

            int fromId;

            fromId = obj["from"].toInt();

            if (obj["type"].toString() == "request")
            {
                QMessageBox::StandardButton reply;

                reply = QMessageBox::question(
                    this,
                    "Request to Pair",
                    "The client " + QString::number(fromId) + " has sent a request to pair.",
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes)
                {
                    QJsonObject obj;
                    obj["type"] = "accept";
                    obj["to"] = fromId;

                    QJsonDocument doc(obj);
                    QString jsonMessage = doc.toJson(QJsonDocument::Compact) + "\n";

                    ui->PairIDInput->setText("Paired to " + QString::number(fromId));
                    ui->PairIDInput->setReadOnly(true);
                    ui->pairButton->setText("Un-Pair");

                    socket->write(jsonMessage.toUtf8());

                    pairPartnerId = fromId;
                }
                else if (reply == QMessageBox::No)
                {
                    QJsonObject obj;
                    obj["type"] = "reject";
                    obj["to"] = fromId;

                    QJsonDocument doc(obj);
                    QString jsonMessage = doc.toJson(QJsonDocument::Compact) + "\n";

                    socket->write(jsonMessage.toUtf8());
                }
            }
            else if (obj["type"].toString() == "accept")
            {
                pairPartnerId = fromId;
                ui->PairIDInput->setText("Paired to " + QString::number(fromId));
                ui->PairIDInput->setReadOnly(true);
                ui->pairButton->setText("Un-Pair");
            }
            else if (obj["type"].toString() == "un-pair")
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

            }
            else if (obj["type"].toString() == "message")
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
            }
            else if (obj["type"].toString() == "file_metadata")
            {
                if (fromId != pairPartnerId)
                {
                    QMessageBox::information(this, "Warning", "An Unrecognized client tried to send you a file!");
                    return;
                }

                QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
                currentFileMessage = new fileMessage;

                currentFileMessage->setFileName(obj["file_name"].toString());
                currentFileMessage->setToAndFromClient(QString::number(fromId), QString::number(myId));
                currentFileMessage->setStatus("Transfering");
                currentFileMessage->setProgress(0);

                current_total_file_size = obj["file_size"].toInteger();
                current_file_size = 0;
                current_file_name = obj["file_name"].toString();

                // QMessageBox::information(this, "Sizes of recieved file.", "Total file size: " + QString::number(current_total_file_size) + " Current size: " + QString::number(current_file_size));

                item->setSizeHint(currentFileMessage->sizeHint());
                ui->MessageList->addItem(item);
                ui->MessageList->setItemWidget(item, currentFileMessage);
                scrollToBottom();

                QDir dir;
                if (dir.mkpath("Files-Received")) {
                    qDebug() << "Folder created:" << "Files-Received";
                } else {
                    qDebug() << "Failed to create folder:" << "Files-Received";
                }

                current_file = new QFile;
                current_file->setFileName(dir.filePath("Files-Received/" + current_file_name));
                if (!current_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    QMessageBox::warning(this, "Error", "Could not open file for writing!");
                    return;
                }

                bytesReceivedThisSecond = 0;
                updateTimer->start(1000);  // 1 second interval

                mode = 2;
            }
            else if (obj["type"].toString() == "file_transfer_finished")
            {
                currentFileMessage->setStatus("File Transfered");
            }

            break;
        }
        case 2: { // receiving raw file bytes
            const qint64 MAX_CHUNK = 1024;
            while (socket->bytesAvailable() > 0 && current_file_size < current_total_file_size) {
                qint64 remaining = current_total_file_size - current_file_size;
                qint64 toRead = qMin(MAX_CHUNK, remaining);

                QByteArray bytes = socket->read(toRead);
                if (bytes.isEmpty()) break; // no more data right now

                qint64 written = 0;
                if (current_file && current_file->isOpen()) {
                    written = current_file->write(bytes);
                    if (written != bytes.size()) {
                        // handle partial/failed write to disk
                        qWarning() << "Short write to file: wrote" << written << "expected" << bytes.size();
                    }
                    // flush occasionally
                    flushIndex++;
                    if (flushIndex >= 25) {
                        current_file->flush();
                        flushIndex = 0;
                    }
                }

                current_file_size += bytes.size();
                bytesReceivedThisSecond += bytes.size();

                // update progress
                double progress = (double)current_file_size / (double)current_total_file_size * 100.0;
                static int lastProgress = -1;
                int intProgress = static_cast<int>(progress);
                if (intProgress != lastProgress) {
                    currentFileMessage->setProgress(intProgress);
                    lastProgress = intProgress;
                }

                // finished?
                if (current_file_size >= current_total_file_size) {
                    mode = 1;
                    current_file->flush();
                    current_file->close();

                    currentFileMessage->setStatus("File Transferred");
                    updateTimer->stop();

                    // Clean up safely
                    delete current_file;
                    current_file = nullptr;
                    current_file_size = 0;
                    current_total_file_size = 0;
                    flushIndex = 0;
                    break;
                }
            }
            break;
        }
        }
    });

    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "Disconnected from server.";
        ui->connectButton->setText("Connect");
        ui->IpEnter->setVisible(true);
        ui->PairIDInput->setReadOnly(false);
        ui->PairIDInput->setText("");
        ui->pairButton->setText("Pair");
        mode = 0;
        pairPartnerId = -1;

        ui->MessageList->clear();

        sendingFile= false;

        if (current_file) {
            if (current_file->isOpen())
                current_file->close();
            delete current_file;
            current_file = nullptr;
        }

        ui->sendButton->setEnabled(true);
        ui->pairButton->setEnabled(true);
        ui->MessageInput->setEnabled(true);

    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_fileDialogButton_clicked()
{
    selectedFilePath = QFileDialog::getOpenFileName(
        this, "Open File", "", "All Files (*.*)");

    if (selectedFilePath.isEmpty()) {
        qDebug() << "Selected file:" << selectedFilePath;
        return;
    }

    ui->FilePathDisplay->setText(selectedFilePath);
    ui->RemoveFileButton->setVisible(true);
}

void MainWindow::on_RemoveFileButton_clicked()
{
    selectedFilePath = "";
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

    QStringList serverInfo = ui->IpEnter->text().split(":");
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
    if (pairPartnerId != -1)
    {
        ui->PairIDInput->setText("");
        ui->PairIDInput->setReadOnly(false);
        ui->pairButton->setText("Pair");
        QJsonObject jsonMessage;
        jsonMessage["type"] = "un-pair";
        jsonMessage["to"] = pairPartnerId;
        QJsonDocument doc(jsonMessage);
        QString jsonString = doc.toJson(QJsonDocument::Compact) + "\n";

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
        QString jsonString = doc.toJson(QJsonDocument::Compact) + "\n";
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
    QFileInfo info(selectedFilePath);

    // 1. Send metadata first
    QJsonObject jsonMessage;
    jsonMessage["type"] = "file_metadata";
    jsonMessage["file_name"] = info.fileName();
    jsonMessage["file_size"] = info.size();
    jsonMessage["to"] = pairPartnerId;

    QJsonDocument doc(jsonMessage);
    QString jsonString = doc.toJson(QJsonDocument::Compact) + '\n';
    socket->write(jsonString.toUtf8());

    // 2. Setup state
    current_file_size = 0;
    current_total_file_size = info.size();

    QListWidgetItem *item = new QListWidgetItem(ui->MessageList);
    currentFileMessage = new fileMessage;
    currentFileMessage->setFileName(info.fileName());
    currentFileMessage->setToAndFromClient(QString::number(myId), QString::number(pairPartnerId));
    currentFileMessage->setStatus("Transferring");
    currentFileMessage->setProgress(0);

    item->setSizeHint(currentFileMessage->sizeHint());
    ui->MessageList->addItem(item);
    ui->MessageList->setItemWidget(item, currentFileMessage);

    current_file = new QFile(selectedFilePath);
    if (!current_file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for reading!");
        delete current_file;
        current_file = nullptr;
        return;
    }

    updateTimer->start(1000);
    sendingFile = true;

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

void MainWindow::sendFileChunk()
{
    if (!sendingFile || !current_file) return;

    const int chunkSize = 1024;
    QByteArray bytes = current_file->read(chunkSize);

    if (bytes.isEmpty()) {
        // === Finished ===
        sendingFile = false;
        updateTimer->stop();

        if (current_file->isOpen())
            current_file->close();
        delete current_file;
        current_file = nullptr;

        // Disconnect so no future writes trigger us
        disconnect(uploadConn);

        currentFileMessage->setStatus("File Transferred");
        currentFileMessage->setProgress(100);

        ui->sendButton->setEnabled(true);
        ui->pairButton->setEnabled(true);
        ui->MessageInput->setEnabled(true);
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

    ui->MessageInput->setText("");
    selectedFilePath = "";
    ui->FilePathDisplay->setText(selectedFilePath);
    ui->RemoveFileButton->setVisible(false);
}
