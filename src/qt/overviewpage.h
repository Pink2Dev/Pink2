#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <tuple>

#include <QWidget>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Ui {
    class OverviewPage;
}
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);
    ~OverviewPage();

    void setModel(WalletModel *model);
    void showOutOfSyncWarning(bool fShow);


public slots:
    void setBalance(qint64 balance, qint64 minted, qint64 stake, qint64 unconfirmedBalance, qint64 confirmingBalance, qint64 immatureBalance);
    void handlePriceReply(QNetworkReply *reply);
    void updateValueLabel(double priceBTC, double priceOther, const QString &currency);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::OverviewPage *ui;
    WalletModel *model;
    QNetworkAccessManager *networkManager;
    qint64 currentBalance;
    qint64 currentStake;
    qint64 currentUnconfirmedBalance;
    qint64 currentConfirmingBalance;
    qint64 currentImmatureBalance;
    qint64 nBtcBalance;
    qint64 nTotalMinted;
    uint nLastPriceCheck;
    QJsonObject currentPrices;
    bool currenciesListDone;
    int FontID;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

    std::tuple<double, double> getCachedPrices(const QString &currency);
    void updatePrices(const QString &currency);
    void constructPricesList();
    void sendRequest();

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
};

#endif // OVERVIEWPAGE_H
