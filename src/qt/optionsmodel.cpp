#include "optionsmodel.h"
#include "bitcoinunits.h"
#include <QSettings>

#include "init.h"
#include "optionsdialog.h"
#include "walletdb.h"
#include "guiutil.h"

#include <QMessageBox>

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    Init();
}

bool static ApplyProxySettings()
{
    QSettings settings;
    CService addrProxy(settings.value("addrProxy", "127.0.0.1:9050").toString().toStdString());
    int nSocksVersion(settings.value("nSocksVersion", 5).toInt());
    if (!settings.value("fUseProxy", false).toBool()) {
        addrProxy = CService();
        nSocksVersion = 0;
        return false;
    }
    if (nSocksVersion && !addrProxy.IsValid())
        return false;
    if (!IsLimited(NET_IPV4))
        SetProxy(NET_IPV4, addrProxy, nSocksVersion);
    if (nSocksVersion > 4) {
        if (!IsLimited(NET_IPV6))
            SetProxy(NET_IPV6, addrProxy, nSocksVersion);
        SetNameProxy(addrProxy, nSocksVersion);
    }
    return true;
}

void OptionsModel::Init()
{
    QSettings settings;

    // These are Qt-only settings:
    nDisplayUnit = settings.value("nDisplayUnit", BitcoinUnits::BTC).toInt();
    fMinimizeToTray = settings.value("fMinimizeToTray", false).toBool();
    fMinimizeOnClose = settings.value("fMinimizeOnClose", false).toBool();
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();
    fNotificationLevel = settings.value("fNotificationLevel", "Notify on All").toString();
    fTargetFPOS = settings.value("fTargetFPOS", false).toBool();
    nTransactionFee = settings.value("nTransactionFee").toLongLong();
    nReserveBalance = settings.value("nReserveBalance").toLongLong();

    language = settings.value("language", "").toString();

    // These are shared with core Bitcoin; we want
    // command-line options to override the GUI settings:
    if (settings.contains("fUseUPnP"))
        SoftSetBoolArg("-upnp", settings.value("fUseUPnP").toBool());
    if (settings.contains("addrProxy") && settings.value("fUseProxy").toBool())
        SoftSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
    if (settings.contains("nSocksVersion") && settings.value("fUseProxy").toBool())
        SoftSetArg("-socks", settings.value("nSocksVersion").toString().toStdString());
    if (settings.contains("detachDB"))
        SoftSetBoolArg("-detachdb", settings.value("detachDB").toBool());
    if (!language.isEmpty())
        SoftSetArg("-lang", language.toStdString());

    // Combine/Split command line/config file options set UI options.
    // 1000/2000 are defaults that would be changed if these values were set elsewhere.
    // Override command line options if targetFPOS toggle is set (sets 100k/200k)

    int64_t valSplitThreshold = settings.value("nSplitThreshold").toLongLong();
    int64_t valCombineThreshold = settings.value("nCombineThreshold").toLongLong();

    if (valCombineThreshold == 0 || valSplitThreshold == 0)
    {
        valSplitThreshold = 2000;
        valCombineThreshold = 1000;
    }


    if (nSplitThreshold == 2000 || valSplitThreshold == 200000)
    {
        nSplitThreshold = valSplitThreshold;
    } else {
        settings.setValue("nSplitThreshold", (qint64) nSplitThreshold);  // it's already been set, so lets update our settings.
    }

    if (nCombineThreshold == 1000 || valCombineThreshold == 100000)
    {
        nCombineThreshold = valCombineThreshold;
    } else {
        settings.setValue("nCombineThreshold", (qint64) nCombineThreshold); // it's already been set, so lets update our settings.

    }


}

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return QVariant(GUIUtil::GetStartOnSystemStartup());
        case MinimizeToTray:
            return QVariant(fMinimizeToTray);
        case MapPortUPnP:
            return settings.value("fUseUPnP", GetBoolArg("-upnp", true));
        case MinimizeOnClose:
            return QVariant(fMinimizeOnClose);
        case ProxyUse:
            return settings.value("fUseProxy", false);
        case ProxyIP: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(QString::fromStdString(proxy.first.ToStringIP()));
            else
                return QVariant(QString::fromStdString("127.0.0.1"));
        }
        case ProxyPort: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(proxy.first.GetPort());
            else
                return QVariant(9050);
        }
        case ProxySocksVersion:
            return settings.value("nSocksVersion", 5);
        case Fee:
            return QVariant((qint64) nTransactionFee);
        case ReserveBalance:
            return QVariant((qint64) nReserveBalance);
        case DisplayUnit:
            return QVariant(nDisplayUnit);
        case DetachDatabases:
            return QVariant(bitdb.GetDetach());
        case Language:
            return settings.value("language", "");
        case CoinControlFeatures:
            return QVariant(fCoinControlFeatures);
        case NotificationLevel:
            return settings.value("fNotificationLevel", "Notify on All");
        case TargetFPOS:
            return QVariant(fTargetFPOS);
        case CombineThreshold:
            return QVariant((qint64) nCombineThreshold);
        case SplitThreshold:
            return QVariant((qint64) nSplitThreshold);
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MapPortUPnP:
            fUseUPnP = value.toBool();
            settings.setValue("fUseUPnP", fUseUPnP);
            MapPort();
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;
        case ProxyUse:
            settings.setValue("fUseProxy", value.toBool());
            ApplyProxySettings();
            break;
        case ProxyIP: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", 9050);
            GetProxy(NET_IPV4, proxy);

            CNetAddr addr(value.toString().toStdString());
            proxy.first.SetIP(addr);
            settings.setValue("addrProxy", proxy.first.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxyPort: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", 9050);
            GetProxy(NET_IPV4, proxy);

            proxy.first.SetPort(value.toInt());
            settings.setValue("addrProxy", proxy.first.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxySocksVersion: {
            proxyType proxy;
            proxy.second = 5;
            GetProxy(NET_IPV4, proxy);

            proxy.second = value.toInt();
            settings.setValue("nSocksVersion", proxy.second);
            successful = ApplyProxySettings();
        }
        break;
        case Fee:
            nTransactionFee = value.toLongLong();
            settings.setValue("nTransactionFee", (qint64) nTransactionFee);
            emit transactionFeeChanged(nTransactionFee);
            break;
        case ReserveBalance:
            nReserveBalance = value.toLongLong();
            settings.setValue("nReserveBalance", (qint64) nReserveBalance);
            emit reserveBalanceChanged(nReserveBalance);
            break;
        case DisplayUnit:
            nDisplayUnit = value.toInt();
            settings.setValue("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(nDisplayUnit);
            break;
        case DetachDatabases: {
            bool fDetachDB = value.toBool();
            bitdb.SetDetach(fDetachDB);
            settings.setValue("detachDB", fDetachDB);
            }
            break;
        case Language:
            settings.setValue("language", value);
            break;
        case CoinControlFeatures: {
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            emit coinControlFeaturesChanged(fCoinControlFeatures);
            }
            break;
        case NotificationLevel: {
            fNotificationLevel = value.toString();
            settings.setValue("fNotificationLevel", fNotificationLevel);
            emit notificationLevelChanged(fNotificationLevel);
            }
            break;
        case TargetFPOS: {
            fTargetFPOS = value.toBool();
            settings.setValue("fTargetFPOS", fTargetFPOS);
            emit targetFPOSChanged(fTargetFPOS);
            }
            break;
        case SplitThreshold:
            if (value.toLongLong() < dlg->getUiCombineThreshold() || value.toLongLong() < 200)
            {
                successful=false;
                QMessageBox::information(dlg, "Invalid Split Threshold", "Split Threshold must be greater than Combine Threshold and at least 200.");
                break;
            }

            nSplitThreshold = value.toLongLong();
            settings.setValue("nSplitThreshold", (qint64) nSplitThreshold);
            emit splitThresholdChanged(nSplitThreshold);
            break;
        case CombineThreshold:
            if (value.toLongLong() > dlg->getUiSplitThreshold() || value.toLongLong() < 100)
            {
                successful=false;
                QMessageBox::information(dlg, "Invalid Combine Threshold", "Combine Threshold must be less than Split Threshold and at least 100.");
                break;
            }
            nCombineThreshold = value.toLongLong();
            settings.setValue("nCombineThreshold", (qint64) nCombineThreshold);
            emit combineThresholdChanged(nCombineThreshold);
            break;
        default:
            break;
        }
    }
    emit dataChanged(index, index);

    return successful;
}


qint64 OptionsModel::getTransactionFee()
{
    return nTransactionFee;
}

qint64 OptionsModel::getReserveBalance()
{
    return nReserveBalance;
}

bool OptionsModel::getCoinControlFeatures()
{
    return fCoinControlFeatures;
}

bool OptionsModel::getTargetFPOS()
{
    return fTargetFPOS;
}

qint64 OptionsModel::getCombineThreshold()
{
    return nCombineThreshold;
}

qint64 OptionsModel::getSplitThreshold()
{
    return nSplitThreshold;
}

bool OptionsModel::getMinimizeToTray()
{
    return fMinimizeToTray;
}

bool OptionsModel::getMinimizeOnClose()
{
    return fMinimizeOnClose;
}

int OptionsModel::getDisplayUnit()
{
    return nDisplayUnit;
}
