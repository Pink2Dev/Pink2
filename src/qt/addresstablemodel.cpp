#include "addresstablemodel.h"
#include "guiutil.h"
#include "walletmodel.h"

#include "wallet.h"
#include "base58.h"
#include "stealth.h"
#include "smessage.h"

#include <QFont>
#include <QColor>

const QString AddressTableModel::Send = "S";
const QString AddressTableModel::Receive = "R";
const QString AddressTableModel::Stake = "X";

struct AddressTableEntry
{
    enum Type {
        Sending,
        Receiving,
        Staking
    };

    Type type;
    QString label;
    QString address;
    QString pmkey;
    bool stealth;
    QString percent;

    AddressTableEntry() {}
    AddressTableEntry(Type type, const QString &label, const QString &address, const QString &pmkey, const bool &stealth = false, const QString &nPercent = ""):
        type(type), label(label), address(address), pmkey(pmkey), stealth(stealth), percent(nPercent) {}
};

struct AddressTableEntryLessThan
{
    bool operator()(const AddressTableEntry &a, const AddressTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const AddressTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const AddressTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class AddressTablePriv
{
public:
    CWallet *wallet;
    QList<AddressTableEntry> cachedAddressTable;
    AddressTableModel *parent;

    AddressTablePriv(CWallet *wallet, AddressTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshAddressTable()
    {
        cachedAddressTable.clear();
        {
            LOCK(wallet->cs_wallet);
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, std::string)& item, wallet->mapAddressBook)
            {
                const CBitcoinAddress& address = item.first;
                const std::string& strName = item.second;
                bool fMine = IsMine(*wallet, address.Get());

                std::string a;
                std::string PMKey = "";
                a = address.ToString();
                std::string nPercent = "";

                if (fMine)
                {
                    int i = SecureMsgGetLocalPublicKey(a, PMKey);
                    if (i == 4)
                        PMKey = "Wallet is Locked.";
                    else if (i)
                        printf("Error getting PM Key %i\n", i);
                }

                AddressTableEntry::Type tThis = fMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending;

                if (wallet->mapAddressPercent.find(item.first) != wallet->mapAddressPercent.end())
                {
                    nPercent = wallet->mapAddressPercent[item.first];
                    tThis = AddressTableEntry::Staking;
                }

                cachedAddressTable.append(AddressTableEntry(tThis,
                                  QString::fromStdString(strName),
                                  QString::fromStdString(address.ToString()),
                                  QString::fromStdString(PMKey), false,
                                  QString::fromStdString(nPercent)));
            }

            std::set<CStealthAddress>::iterator it;
            for (it = wallet->stealthAddresses.begin(); it != wallet->stealthAddresses.end(); ++it)
            {
                bool fMine = !(it->scan_secret.size() < 1);

                cachedAddressTable.append(AddressTableEntry(fMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending,
                                  QString::fromStdString(it->label),
                                  QString::fromStdString(it->Encoded()),
                                  QString::fromStdString(""),
                                  true));
            };
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        qSort(cachedAddressTable.begin(), cachedAddressTable.end(), AddressTableEntryLessThan());
    }

    void updateEntry(const QString &address, const QString &label, bool isMine, int status, const QString &percent)
    {
        // Find address / label in model
        QList<AddressTableEntry>::iterator lower = qLowerBound(
            cachedAddressTable.begin(), cachedAddressTable.end(), address, AddressTableEntryLessThan());
        QList<AddressTableEntry>::iterator upper = qUpperBound(
            cachedAddressTable.begin(), cachedAddressTable.end(), address, AddressTableEntryLessThan());
        int lowerIndex = (lower - cachedAddressTable.begin());
        int upperIndex = (upper - cachedAddressTable.begin());
        bool inModel = (lower != upper);
        bool isStake = (percent != "");
        AddressTableEntry::Type newEntryType = isStake ? AddressTableEntry::Staking : (isMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending);

        switch(status)
        {
        case CT_NEW:
        {
            if(inModel)
            {
                if (fDebug)
                    OutputDebugStringF("Warning: AddressTablePriv::updateEntry: Got CT_NEW, but entry is already in model\n");
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);

            std::string pmkey = "";
            std::string nPercent = percent.toStdString();
            std::string a = address.toStdString();
            int i;
            if (isMine)
            {
                i = SecureMsgGetLocalPublicKey(a, pmkey);
                if (i)
                    printf("Can't get PM Key for some reason\n");
            }

            cachedAddressTable.insert(lowerIndex, AddressTableEntry(newEntryType, label, address, QString::fromStdString(pmkey), false, QString::fromStdString(nPercent)));
            parent->endInsertRows();
            break;
        }
        case CT_UPDATED:
            if(!inModel)
            {
                OutputDebugStringF("Warning: AddressTablePriv::updateEntry: Got CT_UPDATED, but entry is not in model\n");
                break;
            }
            lower->type = newEntryType;
            lower->label = label;
            if (percent != "")
                lower->percent = percent;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                OutputDebugStringF("Warning: AddressTablePriv::updateEntry: Got CT_DELETED, but entry is not in model\n");
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedAddressTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedAddressTable.size();
    }

    AddressTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedAddressTable.size())
        {
            return &cachedAddressTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

AddressTableModel::AddressTableModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Label") << tr("Address") << tr("PM Key") << tr("Percent");
    priv = new AddressTablePriv(wallet, this);
    priv->refreshAddressTable();
}

void AddressTableModel::refreshAddresses()
{
    priv->refreshAddressTable();
}

AddressTableModel::~AddressTableModel()
{
    delete priv;
}

int AddressTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AddressTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AddressTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            if(rec->label.isEmpty() && role == Qt::DisplayRole)
            {
                return tr("(no label)");
            }
            else
            {
                return rec->label;
            }
        case Address:
            return rec->address;
        case PMKey:
            return rec->pmkey;
        case Percent:
            return rec->percent;
        }
    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        if(index.column() == Address)
        {
            font = GUIUtil::bitcoinAddressFont();
        }
        return font;
    }
    else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case AddressTableEntry::Sending:
            return Send;
        case AddressTableEntry::Receiving:
            return Receive;
        case AddressTableEntry::Staking:
            return Stake;
        default: break;
        }
    }
    return QVariant();
}

bool AddressTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());

    editStatus = OK;
    
    std::string strTemp, strValue;
    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            // Do nothing, if old label == new label
            if(rec->label == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            
            strTemp = rec->address.toStdString();
            if (IsStealthAddress(strTemp))
            {
                strValue = value.toString().toStdString();
                wallet->UpdateStealthAddress(strTemp, strValue, false);
            } else if (rec->type == AddressTableEntry::Staking)
            {
                wallet->SetAddressBookStake(CBitcoinAddress(strTemp).Get(), value.toString().toStdString(), rec->percent.toStdString());
            } else
            {
                wallet->SetAddressBookName(CBitcoinAddress(strTemp).Get(), value.toString().toStdString());
            }
            
            break;
        case Percent:
            if(rec->percent == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }

            strTemp = rec->address.toStdString();
            if (IsStealthAddress(strTemp))
            {
                editStatus = NO_CHANGES;  // stealth addresses are not supported with side-staking
                return false;
            }

            if (!checkStakePercent(rec->address.toStdString(), value.toString().toStdString()))
            {
                editStatus = INVALID_PERCENTAGE;
                return false;
            }

            wallet->SetAddressBookStake(CBitcoinAddress(strTemp).Get(), rec->label.toStdString(), value.toString().toStdString());
            break;
        case Address:
            
            std::string sTemp = value.toString().toStdString();
            if (IsStealthAddress(sTemp))
            {
                printf("TODO\n");
                editStatus = INVALID_ADDRESS;
                return false;
            }
            // Do nothing, if old address == new address
            if(CBitcoinAddress(rec->address.toStdString()) == CBitcoinAddress(value.toString().toStdString()))
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Refuse to set invalid address, set error status and return false
            else if(!walletModel->validateAddress(value.toString()))
            {
                editStatus = INVALID_ADDRESS;
                return false;
            }
            // Check for duplicate addresses to prevent accidental deletion of addresses, if you try
            // to paste an existing address over another address (with a different label)
            else if(wallet->mapAddressBook.count(CBitcoinAddress(value.toString().toStdString()).Get()))
            {
                editStatus = DUPLICATE_ADDRESS;
                return false;
            }
            // Double-check that we're not overwriting a receiving address
            else if(rec->type == AddressTableEntry::Sending)
            {
                {
                    LOCK(wallet->cs_wallet);
                    // Remove old entry
                    wallet->DelAddressBookName(CBitcoinAddress(rec->address.toStdString()).Get());
                    // Add new entry with new address
                    wallet->SetAddressBookName(CBitcoinAddress(value.toString().toStdString()).Get(), rec->label.toStdString());
                }
            }
            else if(rec->type == AddressTableEntry::Staking)
            {
                    wallet->SetAddressBookStake(CBitcoinAddress(value.toString().toStdString()).Get(), rec->label.toStdString(), rec->percent.toStdString());
            }
            break;
        }
        return true;
    }
    return false;
}

QVariant AddressTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags AddressTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    // Can edit address and label for sending addresses,
    // and only label for receiving addresses.
    if(rec->type == AddressTableEntry::Sending ||
      (rec->type == AddressTableEntry::Receiving && index.column()==Label) ||
       rec->type == AddressTableEntry::Staking )
    {
        retval |= Qt::ItemIsEditable;
    }
    return retval;
}

QModelIndex AddressTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    AddressTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void AddressTableModel::updateEntry(const QString &address, const QString &label, bool isMine, int status, const QString &percent)
{
    // Update address book model from Bitcoin core
    priv->updateEntry(address, label, isMine, status, percent);
}

QString AddressTableModel::addRow(const QString &type, const QString &label, const QString &address, int addressType, const QString &percent)
{
    std::string strLabel = label.toStdString();
    std::string strAddress = address.toStdString();
    std::string nPercent = percent.toStdString();



    editStatus = OK;
    
    if (type == Send)
    {
        if (strAddress.length() > 75)
        {
            CStealthAddress sxAddr;
            if (!sxAddr.SetEncoded(strAddress))
            {
                editStatus = INVALID_ADDRESS;
                return QString();
            }
            
            // -- Check for duplicate addresses
            {
                LOCK(wallet->cs_wallet);
                
                if (wallet->stealthAddresses.count(sxAddr))
                {
                    editStatus = DUPLICATE_ADDRESS;
                    return QString();
                };
                
                sxAddr.label = strLabel;
                wallet->AddStealthAddress(sxAddr);
            }
        } else
        {
            if (!walletModel->validateAddress(address))
            {
                editStatus = INVALID_ADDRESS;
                return QString();
            };
            // Check for duplicate addresses
            {
                LOCK(wallet->cs_wallet);
                if (wallet->mapAddressBook.count(CBitcoinAddress(strAddress).Get()))
                {
                    editStatus = DUPLICATE_ADDRESS;
                    return QString();
                };
                wallet->SetAddressBookName(CBitcoinAddress(strAddress).Get(), strLabel);
            }
        }
    }
    else if(type == Stake)
    {
        if (!walletModel->validateAddress(address))
        {
            editStatus = INVALID_ADDRESS;
            return QString();
        };
        if (!checkStakePercent(address.toStdString(), nPercent))
        {
            editStatus = INVALID_PERCENTAGE;
            return QString();
        }
        // Check for duplicate addresses
        {
            LOCK(wallet->cs_wallet);
            if (wallet->mapAddressBook.count(CBitcoinAddress(strAddress).Get()))
            {
                editStatus = DUPLICATE_ADDRESS;
                return QString();
            };

            wallet->SetAddressBookStake(CBitcoinAddress(strAddress).Get(), strLabel, nPercent);
        }
    }
    else if(type == Receive)
    {
        // Generate a new address to associate with given label
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        
        if(!ctx.isValid())
        {
            // Unlock wallet failed or was cancelled
            editStatus = WALLET_UNLOCK_FAILURE;
            return QString();
        }
        
        if (addressType == AT_Stealth)
        {
            CStealthAddress newStealthAddr;
            std::string sError;
            if (!wallet->NewStealthAddress(sError, strLabel, newStealthAddr)
                || !wallet->AddStealthAddress(newStealthAddr))
            {
                editStatus = KEY_GENERATION_FAILURE;
                return QString();
            }
            strAddress = newStealthAddr.Encoded();
        } else
        {
            CPubKey newKey;
            if(!wallet->GetKeyFromPool(newKey, true))
            {
                editStatus = KEY_GENERATION_FAILURE;
                return QString();
            }
            strAddress = CBitcoinAddress(newKey.GetID()).ToString();
            
            {
                LOCK(wallet->cs_wallet);
                wallet->SetAddressBookName(CBitcoinAddress(strAddress).Get(), strLabel);
            }
        }
    }
    else
    {
        return QString();
    }

    return QString::fromStdString(strAddress);
}

bool AddressTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    AddressTableEntry *rec = priv->index(row);
    if(count != 1 || !rec || rec->type == AddressTableEntry::Receiving)
    {
        // Can only remove one row at a time, and cannot remove rows not in model.
        // Also refuse to remove receiving addresses.
        return false;
    }
    {     
        LOCK(wallet->cs_wallet);
        if (rec->type == AddressTableEntry::Staking)
            wallet->DelAddressBookStake(CBitcoinAddress(rec->address.toStdString()).Get());
        else
            wallet->DelAddressBookName(CBitcoinAddress(rec->address.toStdString()).Get());
    }
    return true;
}

/* Look up label for address in address book, if not found return empty string.
 */
QString AddressTableModel::labelForAddress(const QString &address) const
{
    {
        LOCK(wallet->cs_wallet);
        
        std::string sAddr = address.toStdString();
        
        if (sAddr.length() > 75)
        {
            CStealthAddress sxAddr;
            if (!sxAddr.SetEncoded(sAddr))
                return QString();
            
            std::set<CStealthAddress>::iterator it;
            it = wallet->stealthAddresses.find(sxAddr);
            if (it == wallet->stealthAddresses.end())
                return QString();
            
            return QString::fromStdString(it->label);
        } else
        {
            CBitcoinAddress address_parsed(sAddr);
            std::map<CTxDestination, std::string>::iterator mi = wallet->mapAddressBook.find(address_parsed.Get());
            if (mi != wallet->mapAddressBook.end())
            {
                return QString::fromStdString(mi->second);
            }
        }
    }
    return QString();
}

int AddressTableModel::lookupAddress(const QString &address) const
{
    QModelIndexList lst = match(index(0, Address, QModelIndex()),
                                Qt::EditRole, address, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void AddressTableModel::emitDataChanged(int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

bool AddressTableModel::checkStakePercent(std::string address, std::string percent)
{
    std::string sPercent = percent;

    if (sPercent == "")
        sPercent = "0";

    std::string sStack = "";
    for (unsigned int i = 0; i < sPercent.length(); i++) sStack += isdigit(sPercent[i]) ? sPercent[i] : (char)NULL;

    if (sPercent != sStack)
    {
        printf("\nPlease only use whole numbers between 0 and 100 and no special characters for Percentage\n");
        return false;
    }

    int nPercent = std::stoi(sPercent);


    if (nPercent < 0 || nPercent > 100)
    {
        printf("\nPlease use a percentage between 0 and 100");
        return false;
    }

    CBitcoinAddress a(address);
    if (!a.IsValid())
    {
        printf("\nInvalid Bitcoin Address.");
        return false;
    }

    int percentAvailable = 100;

    BOOST_FOREACH(CWallet::mapAddress mapPercent, wallet->mapAddressPercent)
    {
            std::string percent = mapPercent.second;
            if (percent == "")
                percent = "0";

            percentAvailable -= std::stoi(percent);
    }

    int updatingPercent = 0;
    if (wallet->mapAddressPercent.find(a.Get()) != wallet->mapAddressPercent.end())
    {
        std::string sUpdatingPercent = wallet->mapAddressPercent[a.Get()];
        if (sUpdatingPercent == "")
            sUpdatingPercent = "0";

        updatingPercent = std::stoi(sUpdatingPercent);
    }

    if (nPercent > (percentAvailable + updatingPercent))
    {
        printf("\nStake Percentage would increase total Stakeout over 100%\n"
                            "Please Reduce Stakeout Percentage or Delete another Stakeout to make room.\n"
                            "Current Available Stakeout Percentage: %s", std::to_string(percentAvailable).c_str());
        return false;
    }

    return true;
}
