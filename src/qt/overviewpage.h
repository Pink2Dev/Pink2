#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

#if QT_VERSION >= 0x050000
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#endif

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
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setModel(WalletModel *model);
    void showOutOfSyncWarning(bool fShow);


public slots:
    void setBalance(qint64 balance, qint64 minted, qint64 stake, qint64 unconfirmedBalance, qint64 confirmingBalance, qint64 immatureBalance);
	#if QT_VERSION >= 0x050000
	void sendRequest();
    void handlePriceReply(QNetworkReply *reply);
    void updateBtcValueLabel(double nPrice, double nPriceUSD);
	#endif
signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::OverviewPage *ui;
    WalletModel *model;
    qint64 currentBalance;
    qint64 currentStake;
    qint64 currentUnconfirmedBalance;
    qint64 currentConfirmingBalance;
    qint64 currentImmatureBalance;
    qint64 nBtcBalance;
    qint64 nTotalMinted;
    double nLastPrice;
    double nLastPriceUSD;
    int FontID;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
};

#endif // OVERVIEWPAGE_H
