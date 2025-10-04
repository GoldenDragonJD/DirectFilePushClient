#include "message.h"
#include "ui_message.h"

message::message(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::message)
{
    ui->setupUi(this);
}

message::~message()
{
    delete ui;
}

void message::setId(const QString &text)
{
    ui->ClientID->setText(text);
}

void message::setText(const QString &text)
{
    // 1️⃣ Create a copy so we can safely modify it
    QString formatted = text;

    // 2️⃣ Replace literal "\n" sequences with actual newlines
    formatted.replace("\\n", "\n");

    // 3️⃣ Convert Markdown → HTML for display
    QTextDocument doc;
    doc.setMarkdown(formatted);

    // 4️⃣ Set the rendered HTML in your text display widget
    ui->MessageBox->setText(doc.toHtml());
    ui->MessageBox->setTextFormat(Qt::RichText);
    ui->MessageBox->setWordWrap(true);
}
