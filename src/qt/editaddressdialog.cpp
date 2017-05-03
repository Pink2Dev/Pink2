#include "editaddressdialog.h"
#include "ui_editaddressdialog.h"
#include "addresstablemodel.h"
#include "guiutil.h"

#include <QDataWidgetMapper>
#include <QMessageBox>

EditAddressDialog::EditAddressDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditAddressDialog), mapper(0), mode(mode), model(0)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->addressEdit, this);

    switch(mode)
    {
    case NewReceivingAddress:
        setWindowTitle(tr("New receiving address"));
        ui->addressEdit->setEnabled(false);
        ui->addressEdit->setVisible(false);
        ui->stealthCB->setEnabled(true);
        ui->stealthCB->setVisible(true);
        ui->Percent->setVisible(false);
        ui->spinPercent->setVisible(false);
        break;
    case NewSendingAddress:
        setWindowTitle(tr("New sending address"));
        ui->stealthCB->setVisible(false);
        ui->Percent->setVisible(false);
        ui->spinPercent->setVisible(false);
        break;
    case NewStakingAddress:
        setWindowTitle(tr("New Address for Side-Stake"));
        ui->Percent->setVisible(true);
        ui->spinPercent->setEnabled(true);
        ui->spinPercent->setVisible(true);
        ui->stealthCB->setVisible(false);
        break;
    case EditStakingAddress:
        setWindowTitle(tr("New Address for Side-Stake"));
        ui->Percent->setVisible(true);
        ui->spinPercent->setEnabled(true);
        ui->spinPercent->setVisible(true);
        ui->stealthCB->setVisible(false);
        break;
    case EditReceivingAddress:
        setWindowTitle(tr("Edit receiving address"));
        ui->addressEdit->setEnabled(false);
        ui->addressEdit->setVisible(true);
        ui->stealthCB->setEnabled(false);
        ui->stealthCB->setVisible(true);
        ui->Percent->setVisible(false);
        ui->spinPercent->setVisible(false);
        break;
    case EditSendingAddress:
        setWindowTitle(tr("Edit sending address"));
        ui->stealthCB->setVisible(false);
        ui->Percent->setVisible(false);
        ui->spinPercent->setVisible(false);
        break;
    }

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}

EditAddressDialog::~EditAddressDialog()
{
    delete ui;
}

void EditAddressDialog::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    mapper->setModel(model);
    mapper->addMapping(ui->labelEdit, AddressTableModel::Label);
    mapper->addMapping(ui->addressEdit, AddressTableModel::Address);
    mapper->addMapping(ui->spinPercent, AddressTableModel::Percent);
    mapper->addMapping(ui->stealthCB, AddressTableModel::Type);
}

void EditAddressDialog::loadRow(int row)
{
    mapper->setCurrentIndex(row);
}

bool EditAddressDialog::saveCurrentRow()
{
    if(!model)
        return false;

    switch(mode)
    {
    case NewReceivingAddress:
    case NewSendingAddress:
    {
        int typeInd  = ui->stealthCB->isChecked() ? AddressTableModel::AT_Stealth : AddressTableModel::AT_Normal;
        address = model->addRow(
                mode == NewSendingAddress ? AddressTableModel::Send : AddressTableModel::Receive,
                ui->labelEdit->text(),
                ui->addressEdit->text(),
                typeInd);
    }
        break;
    case NewStakingAddress:
    {

        address = model->addRow(
                    AddressTableModel::Stake,
                    ui->labelEdit->text(),
                    ui->addressEdit->text(),
                    AddressTableModel::AT_Normal,
                    ui->spinPercent->cleanText());

    }
        break;
    case EditReceivingAddress:
    case EditSendingAddress:
    case EditStakingAddress:
        if(mapper->submit())
        {
            address = ui->addressEdit->text();
        }
        break;
    }
    return !address.isEmpty();
}

void EditAddressDialog::accept()
{
    if(!model)
        return;

    if(!saveCurrentRow())
    {
        switch(model->getEditStatus())
        {
        case AddressTableModel::OK:
            // Failed with unknown reason. Just reject.
            break;
        case AddressTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case AddressTableModel::INVALID_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered address \"%1\" is not a valid Pinkcoin address.").arg(ui->addressEdit->text()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::DUPLICATE_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered address \"%1\" is already in the address book.").arg(ui->addressEdit->text()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::WALLET_UNLOCK_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                tr("Could not unlock wallet."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::KEY_GENERATION_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                tr("New key generation failed."),
                QMessageBox::Ok, QMessageBox::Ok);
        case AddressTableModel::INVALID_PERCENTAGE:
            QMessageBox::warning(this, windowTitle(),
                 tr("Percentage exceeds available limit."),
                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        }
        return;
    }
    QDialog::accept();
}

QString EditAddressDialog::getAddress() const
{
    return address;
}

void EditAddressDialog::setAddress(const QString &address)
{
    this->address = address;
    ui->addressEdit->setText(address);
}
