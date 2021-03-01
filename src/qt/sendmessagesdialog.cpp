#include "sendmessagesdialog.h"
#include "ui_sendmessagesdialog.h"
//#include "init.h"
#include "walletmodel.h"
#include "messagemodel.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendmessagesentry.h"
//#include "guiutil.h"

#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>
#include <QDataWidgetMapper>

SendMessagesDialog::SendMessagesDialog(Mode mode, Type type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendMessagesDialog),
    model(nullptr),
    mode(mode),
    type(type)
{

    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

     /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    // CHECK: Can we move it now??
    if(mode == SendMessagesDialog::Encrypted)
        ui->addressFrom->setPlaceholderText(tr("Enter a Pinkcoin address (e.g. 2XywGBZBowrppUwwNUo1GCRDTibzJi7g2M)"));

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    fNewRecipientAllowed = true;

    if(mode == SendMessagesDialog::Anonymous)
        ui->frameAddressFrom->hide();

    if(type == SendMessagesDialog::Page)
        ui->closeButton->hide();
}

void SendMessagesDialog::setModel(MessageModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());

        if(entry)
            entry->setModel(model);
    }
}

void SendMessagesDialog::loadRow(int row)
{
    if(model->data(model->index(row, model->Type, QModelIndex()), Qt::DisplayRole).toString() == MessageModel::Received)
        ui->addressFrom->setText(model->data(model->index(row, model->ToAddress, QModelIndex()), Qt::DisplayRole).toString());
    else
        ui->addressFrom->setText(model->data(model->index(row, model->FromAddress, QModelIndex()), Qt::DisplayRole).toString());

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());

        if(entry)
            entry->loadRow(row);
    }
}

bool SendMessagesDialog::checkMode(Mode mode)
{
    return (mode == this->mode);
}

bool SendMessagesDialog::validate()
{
    if(mode == SendMessagesDialog::Encrypted && ui->addressFrom->text() == "")
    {
        ui->addressFrom->setValid(false);

        return false;
    }

    return true;
}

SendMessagesDialog::~SendMessagesDialog()
{
    delete ui;
}

void SendMessagesDialog::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->addressFrom->setText(QApplication::clipboard()->text());
}

void SendMessagesDialog::on_addressBookButton_clicked()
{
    if(!model)
        return;

    AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);

    dlg.setModel(model->getWalletModel()->getAddressTableModel());

    if(dlg.exec())
    {
        ui->addressFrom->setText(dlg.getReturnValue());
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(0)->widget());
        entry->setFocus();
               // findChild( const QString "sentTo")->setFocus();
    }
}

void SendMessagesDialog::on_sendButton_clicked()
{
    QList<SendMessagesRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    valid = validate();

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
                recipients.append(entry->getValue());
            else
                valid = false;
        }
    }

    if(!valid || recipients.isEmpty())
        return;

    // Format confirmation message
    QStringList formatted;
    foreach(const SendMessagesRecipient &rcp, recipients)
    {
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(rcp.message, Qt::escape(rcp.label), rcp.address));
    }

    fNewRecipientAllowed = false;

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send messages"),
                          tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    MessageModel::StatusCode sendstatus;

    if(mode == SendMessagesDialog::Anonymous)
        sendstatus = model->sendMessages(recipients);
    else
        sendstatus = model->sendMessages(recipients, ui->addressFrom->text());

    switch(sendstatus)
    {
    case MessageModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Message"),
            tr("The recipient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::InvalidMessage:
        QMessageBox::warning(this, tr("Send Message"),
            tr("The message can't be empty."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::DuplicateAddress:
        QMessageBox::warning(this, tr("Send Message"),
            tr("Duplicate address found, can only send to each address once per send operation."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::MessageCreationFailed:
        QMessageBox::warning(this, tr("Send Message"),
            tr("Error: Message creation failed."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::MessageCommitFailed:
        QMessageBox::warning(this, tr("Send Message"),
            tr("Error: The message was rejected."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::Aborted:             // User aborted, nothing to do
        break;
    case MessageModel::FailedErrorShown:    // Send failed, error message was displayed
        break;
    case MessageModel::OK:
        accept();
        break;
    }

    fNewRecipientAllowed = true;
}

void SendMessagesDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
        delete ui->entries->takeAt(0)->widget();

    addEntry();

    updateRemoveEnabled();

    ui->sendButton->setDefault(true);
}

void SendMessagesDialog::reject()
{
    if(type == SendMessagesDialog::Dialog)
        done(1);
    else
        clear();
}

void SendMessagesDialog::accept()
{
    if(type == SendMessagesDialog::Dialog)
        done(0);
    else
        clear();

}

void SendMessagesDialog::done(int retval)
{
    if(type == SendMessagesDialog::Dialog)
        QDialog::done(retval);
    else
        clear();
}

SendMessagesEntry *SendMessagesDialog::addEntry()
{
    SendMessagesEntry *entry = new SendMessagesEntry(this);

    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendMessagesEntry*)), this, SLOT(removeEntry(SendMessagesEntry*)));
    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();

    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QCoreApplication::instance()->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();

    if(bar)
        bar->setSliderPosition(bar->maximum());

    return entry;
}

void SendMessagesDialog::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            entry->setRemoveEnabled(enabled);
    }

    setupTabChain(nullptr);
}

void SendMessagesDialog::removeEntry(SendMessagesEntry* entry)
{
    delete entry;

    updateRemoveEnabled();
}

QWidget *SendMessagesDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }

    QWidget::setTabOrder(prev, ui->addButton);
    QWidget::setTabOrder(ui->addButton, ui->sendButton);

    return ui->sendButton;
}

void SendMessagesDialog::pasteEntry(const SendMessagesRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendMessagesEntry *entry = nullptr;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendMessagesEntry *first = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(0)->widget());

        if(first->isClear())
            entry = first;
    }

    if(!entry)
        entry = addEntry();

    entry->setValue(rv);
}
