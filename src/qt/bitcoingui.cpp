/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2012
 */
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookpage.h"

#include "messagepage.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "messagemodel.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "wallet.h"

#include "clabel.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QFormLayout>
#include <QBitmap>
#include <QPixmap>
#include <QPainter>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>
#include <QTextDocument>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QSignalMapper>
#include <QSettings>
#include <QWidgetAction>
#include <QToolButton>
#include <QFontDatabase>

#include <iostream>

extern CWallet* pwalletMain;
extern int64_t nLastCoinStakeSearchInterval;

double GetPoSKernelPS();

ActiveLabel::ActiveLabel(const QString & text, QWidget * parent):
    QLabel(parent){

    setAttribute(Qt::WA_Hover);

}

bool ActiveLabel::event(QEvent *e)
{
    if(e->type() == QEvent::MouseButtonPress)
    {
        // Hack: Swallows mouse press event to prevent
        // from passing it to BitcoinGUI::mousePressEvent
        // (conflicts with main window grabbing)
        return true;
    }
    else if(e->type() == QEvent::HoverMove)
    {
        emit hovered();
        return true;
    }
    else if(e->type() == QEvent::HoverLeave)
    {
        emit unhovered();
        return true;
    }
    else if(e->type() == QEvent::MouseButtonRelease)
    {
        emit clicked();
        return true;
    }

    return QLabel::event(e);
}


BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    unlockWalletAction(0),
    lockWalletAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    nWeight(0)
{
    setFixedSize(1050, 600);

    #ifdef Q_OS_WIN
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    #else
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    #endif

    setWindowTitle(tr("Pinkcoin") + " - " + tr("Wallet"));

    qApp->setStyleSheet(R"(
        QMainWindow
        {
            padding-left: 0px;
            padding-right: 0px;
            padding-bottom: 0px;
            padding-top: 0px;
            border: 0px;
        }

        #frame { }
        QToolBar QLabel { padding-top: 0px; padding-bottom: 0px; spacing: 0px; border: 0px; }
        QToolBar QLabel:item { padding-top: 0px; padding-bottom: 0px; spacing: 0px; border: 0px; }

        #spacer2 {
            background: rgb(26, 0, 13);
            border: none;
            margin-top: 1px;
            margin-bottom: 1px;
            margin-left: 1px;
            margin-right: 4px;
        }
        #spacer { background: rgb(152, 50, 101); border: none; }
        #toolbar2
        {
            border: none;
            width: 0px;
            height: 0px;
            padding-top: 0px;
            padding-bottom: 0px;
            background-color: rgb(0, 0, 0);
        }
        #toolbar
        {
            border: 0px;
            height: 100%;
            padding-top: 0px;
            padding-left: 0px;
            padding-right: 0px;
            background: rgb(255, 255, 255);
            text-align: left;
            color: black;
            max-width: 13em;
        }
        QToolBar QToolButton
        {
            font-family: Rubik;
            font-size: 1em;
            font-weight: bold;
            border: 0px;
            padding-left: 2px;
            padding-top: 8px;
            padding-bottom: 8px;
            padding-right: 4px;
            color: black;
            text-align: left;
            background-color: rgb(255, 255, 255);
        }
        #toolbar QToolButton:hover, #toolbar QToolButton:checked {
            background-color:qlineargradient(x1: 0, y1: 0, x2: 0.5, y2: 0.5, stop: 0 rgb(155, 101, 183), stop: 1 rgb(255, 141, 183));
            border: none;
        }
        #labelMiningIcon
        {
            padding-left: 5px;
            font-family: Century Gothic;
            width: 100%;
            font-size: 0.7em;
            text-align: center;
            color: white;
        }
        QMenu {
            background: rgb(0, 0, 0);
            color: white;
            padding-top: 3px;
            padding-bottom: 3px;
            padding-left: 3px;
            padding-right: 3px;
        }
        QMenu::item {
            color: white;
            background-color: transparent;
            border: 0px;
            padding-top: 5px;
            padding-bottom: 5px;
            padding-right: 5px;
            min-width: 150px;
        }
        QMenu::item:selected {
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0.5, y2: 0.5,stop: 0 rgb(255, 101, 183), stop: 1 rgb(255, 101, 183));
            border: 0px;
            padding-top: 5px;
            padding-bottom: 5px;
            padding-right: 5px;
            min-width: 150px;
        }
        QMenu::item:disabled {
            color: darkGrey;
            background-color: DimGrey;
        }
        QMenuBar { background: rgb(0, 0, 0); color: white; }
        QMenuBar::item
        {
            font-size: 0.4em;
            padding-bottom: 8px;
            padding-top: 8px;
            padding-left: 15px;
            padding-right: 15px;
            color: white;
            background-color: transparent;
        }
        QMenuBar::item:selected {
            background-color:qlineargradient(x1: 0, y1: 0, x2: 0.5, y2: 0.5,stop: 0 rgb(255, 101, 183), stop: 1 rgb(255, 101, 183));
        }
    )");
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":icons/bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif


    // Discover themes - zeewolf: Hot swappable wallet themes */
    listThemes(themesList);

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create the tray icon (or setup the dock icon)
    createTrayIcon();

    // Create tabs

    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox = new QHBoxLayout();

    transactionView = new TransactionView(this);

    vbox->addWidget(transactionView);

    hbox->setContentsMargins(29, 5 ,0, 0);
    hbox->addLayout(vbox);

    transactionsPage->setLayout(hbox);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    stakeCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::StakingTab);

    sendCoinsPage = new SendCoinsDialog(this);
    messagePage   = new MessagePage(this);

    signVerifyMessageDialog = new SignVerifyMessageDialog(this);

    centralWidget = new QStackedWidget(this);
    centralWidget->setObjectName("StackedWidget");
    // Left side gradient shadow on panels.
    centralWidget->setStyleSheet(R"(
        #StackedWidget {
            background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:.04, y2:0, stop:0 rgba(225, 225, 220, 255), stop:1 rgba(255, 255, 255, 255));
        }
    )");
    centralWidget->addWidget(overviewPage);
    centralWidget->addWidget(transactionsPage);
    centralWidget->addWidget(stakeCoinsPage);
    centralWidget->addWidget(addressBookPage);
    centralWidget->addWidget(receiveCoinsPage);
    centralWidget->addWidget(sendCoinsPage);
    centralWidget->addWidget(messagePage);


    setCentralWidget(centralWidget);

    QPainterPath painterPath;
    painterPath.addRoundedRect(0, 0, 1050, 600, 3, 3);
    QPolygon maskPolygon = painterPath.toFillPolygon().toPolygon();

    QRegion maskedRegion(maskPolygon, Qt::OddEvenFill);

    setMask(maskedRegion);

    // Create status bar

    labelEncryptionIcon = new ActiveLabel();

    labelStakingIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();

    labelConnectionsIcon->setStyleSheet(R"(
        QLabel {
            padding-right: 2px;
        }
    )");

    if (GetBoolArg("-staking", true))
    {
        QTimer *timerStakingIcon = new QTimer(labelStakingIcon);
        connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
        timerStakingIcon->start(30 * 1000);
        updateStakingIcon();
    }

    connect(labelEncryptionIcon, SIGNAL(clicked()), unlockWalletAction, SLOT(trigger()));

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setAlignment(Qt::AlignLeft);
    progressBar->setVisible(false);

    progressBar->setStyleSheet(R"(
        QProgressBar {
            background-color: white;
            border-radius: 4px;
            padding: 1px;
            text-align: center;
            font-size: 14px;
            font-family: Rubik;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1: 0, y1: 0, x2: 0.5, y2: 0.5, stop: 0 rgb(116, 85, 195), stop: 1 rgb(172, 143, 238));
            border-radius: 4px;
            margin: 0px;
        }
    )");

    addToolBarBreak(Qt::BottomToolBarArea);
    QToolBar *toolbar2 = addToolBar(tr("Tabs toolbar"));
    addToolBar(Qt::BottomToolBarArea,toolbar2);
    toolbar2->setOrientation(Qt::Horizontal);
    toolbar2->setMovable(false);
    toolbar2->setObjectName("toolbar2");
    toolbar2->setFixedHeight(28);
    toolbar2->setIconSize(QSize(28,28));
    QFrame* spacer2 = new QFrame();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *frameSpacer2 = new QHBoxLayout(spacer2);

    frameSpacer2->setContentsMargins(0,0,0,0);
    spacer2->setContentsMargins(0,0,0,0);


    progressBar->setFixedWidth(600);
    QFontDatabase fontData;
    progressBarLabel->setFont(fontData.font("Rubik", "Medium", 10));

    progressBarLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 14px;
            font-weight: 400;
        }
    )");

    frameSpacer2->addWidget(progressBarLabel);
    frameSpacer2->addWidget(progressBar);
    frameSpacer2->addStretch();


    toolbar2->addWidget(spacer2);
    spacer2->setObjectName("spacer2");

    toolbar2->addWidget(labelBlocksIcon);
    toolbar2->addWidget(labelEncryptionIcon);
    toolbar2->addWidget(labelStakingIcon);
    toolbar2->addWidget(labelConnectionsIcon);

    syncIconMovie = new QMovie(":/movies/update_spinner", "gif", this);

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));
    // prevents an oben debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

    // Clicking on "Verify Message" in the address book sends you to the verify message tab
    connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive coins page sends you to the sign message tab
    connect(receiveCoinsPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));

    gotoOverviewPage();
}

BitcoinGUI::~BitcoinGUI()
{

    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
#endif
}

/** Hack to make main toolbar width adjusts
    to the width of the widest button in a group.
 */
void BitcoinGUI::updateMainToolbar()
{
    this->show();
    this->layout()->invalidate();
    this->hide();

    QAction* tabActions[] = {
        overviewAction, sendCoinsAction, receiveCoinsAction, addressBookAction,
        stakeCoinsAction, historyAction, messageAction
    };

    // Finds maximum width in set of tab buttons.
    int maxWidth = 0;
    for (auto action : tabActions) {
        QToolButton *button = dynamic_cast<QToolButton*>(mainToolbar->widgetForAction(action));
        if (maxWidth < button->width()) {
            maxWidth = button->width();
        }
    }

    // Resize all tab buttons to match maximum width in a group.
    for (auto action : tabActions) {
        QToolButton *button = dynamic_cast<QToolButton*>(mainToolbar->widgetForAction(action));
        button->setMinimumWidth(maxWidth);
        button->setMaximumWidth(maxWidth);
    }

    // Resizes main toolbar.
    mainToolbar->resize(maxWidth, mainToolbar->height());
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    QIcon *iOverView = new QIcon();

    iOverView->addFile(":/icons/overview", QSize(), QIcon::Normal, QIcon::Off );
    iOverView->addFile(":/icons/overview_s", QSize(), QIcon::Active, QIcon::Off);
    iOverView->addFile(":/icons/overview_s", QSize(), QIcon::Active, QIcon::On);

    QIcon *iSend = new QIcon();

    iSend->addFile(":/icons/send", QSize(), QIcon::Normal, QIcon::Off );
    iSend->addFile(":/icons/send_s", QSize(), QIcon::Active, QIcon::Off);
    iSend->addFile(":/icons/send_s", QSize(), QIcon::Active, QIcon::On);

    QIcon *iReceive= new QIcon();

    iReceive->addFile(":/icons/receiving_addresses", QSize(), QIcon::Normal, QIcon::Off );
    iReceive->addFile(":/icons/receiving_addresses_s", QSize(), QIcon::Active, QIcon::Off);
    iReceive->addFile(":/icons/receiving_addresses_s", QSize(), QIcon::Active, QIcon::On);

    QIcon *iSideStake = new QIcon();

    iSideStake->addFile(":/icons/sidestake", QSize(), QIcon::Normal, QIcon::Off );
    iSideStake->addFile(":/icons/sidestake_s", QSize(), QIcon::Active, QIcon::Off);
    iSideStake->addFile(":/icons/sidestake_s", QSize(), QIcon::Active, QIcon::On);

    QIcon *iAddressBook = new QIcon();

    iAddressBook->addFile(":/icons/address-book", QSize(), QIcon::Normal, QIcon::Off );
    iAddressBook->addFile(":/icons/address-book_s", QSize(), QIcon::Active, QIcon::Off);
    iAddressBook->addFile(":/icons/address-book_s", QSize(), QIcon::Active, QIcon::On);

    QIcon *iHistory = new QIcon();

    iHistory->addFile(":/icons/history", QSize(), QIcon::Normal, QIcon::Off );
    iHistory->addFile(":/icons/history_s", QSize(), QIcon::Active, QIcon::Off);
    iHistory->addFile(":/icons/history_s", QSize(), QIcon::Active, QIcon::On);

    QIcon *iMessage = new QIcon();

    iMessage->addFile(":/icons/message", QSize(), QIcon::Normal, QIcon::Off );
    iMessage->addFile(":/icons/message_s", QSize(), QIcon::Active, QIcon::Off);
    iMessage->addFile(":/icons/message_s", QSize(), QIcon::Active, QIcon::On);

    QFontDatabase fontData;

    overviewAction = new QAction(*iOverView, tr("&Overview"), this);
    overviewAction->setToolTip(tr("Show general overview of wallet"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    overviewAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(*iSend, tr("&Send coins"), this);
    sendCoinsAction->setToolTip(tr("Send coins to a Pinkcoin address"));
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    sendCoinsAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(*iReceive, tr("&Receive coins"), this);
    receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving payments"));
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    receiveCoinsAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(receiveCoinsAction);

    addressBookAction = new QAction(*iAddressBook, tr("&Address Book"), this);
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    addressBookAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(addressBookAction);

    stakeCoinsAction = new QAction(*iSideStake, tr("&Side Stakes"), this);
    stakeCoinsAction->setToolTip(tr("Edit the list of addresses for staking out."));
    stakeCoinsAction->setCheckable(true);
    stakeCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    stakeCoinsAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(stakeCoinsAction);

    historyAction = new QAction(*iHistory, tr("&Transactions"), this);
    historyAction->setToolTip(tr("Browse transaction history"));
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    historyAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(historyAction);

    messageAction = new QAction(*iMessage, tr("&Messages"), this);
    messageAction->setToolTip(tr("View and Send Private Messages"));
    messageAction->setCheckable(true);
    messageAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    messageAction->setFont(fontData.font("Rubik", "Regular", 10));
    tabGroup->addAction(messageAction);

    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));

    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));

    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));

    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));

    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));

    connect(stakeCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(stakeCoinsAction, SIGNAL(triggered()), this, SLOT(gotoStakeCoinsPage()));

    connect(messageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(gotoMessagePage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/bitcoin"), tr("&About Pinkcoin"), this);
    aboutAction->setToolTip(tr("Show information about Pinkcoin"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(QIcon(":/icons/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for Pinkcoin"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/bitcoin"), tr("&Show / Hide"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setToolTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(QIcon(":/icons/lock_open"), tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Lock Wallet"), this);
    lockWalletAction->setToolTip(tr("Lock wallet"));
    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    verifyMessageAction = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));

    minimizeAction = new QAction(this);
    closeAction = new QAction(this);

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), this, SLOT(encryptWallet(bool)));
    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(unlockWalletAction, SIGNAL(triggered()), this, SLOT(unlockWallet()));
    connect(lockWalletAction, SIGNAL(triggered()), this, SLOT(lockWallet()));
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));

    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));

	/* zeewolf: Hot swappable wallet themes */
    if (themesList.count()>0)
    {
        QSignalMapper* signalMapper = new QSignalMapper (this) ;

        // Add custom themes (themes directory)
        for( int i=0; i < themesList.count(); i++ )
        {
            QString theme=themesList[i];
            customActions[i] = new QAction(QIcon(":/icons/options"), theme, this);
            customActions[i]->setToolTip(QString("Switch to " + theme + " theme"));
            customActions[i]->setStatusTip(QString("Switch to " + theme + " theme"));
            signalMapper->setMapping(customActions[i], theme);
            connect(customActions[i], SIGNAL(triggered()), signalMapper, SLOT (map()));
        }
        connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(changeTheme(QString)));
    }
    /* zeewolf: Hot swappable wallet themes */
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus

    appMenuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QWidget *w = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(w);

    QLabel* pinkCorner = new QLabel(w);
    pinkCorner->setText("<html><head/><body><p><img src=\":/icons/pinkcoin-32\"/></p></body></html>");
    pinkCorner->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(pinkCorner);

    layout->addWidget(appMenuBar);
    w->setStyleSheet("background-color: rgba(0,0,0,255)");

    labelMinimizeIcon = new ActiveLabel();
    labelCloseIcon = new ActiveLabel();

    labelMinimizeIcon->setText("<html><head/><body><p><img src=\":/icons/min\"/></p></body></html>");
    labelCloseIcon->setText("<html><head/><body><p><img src=\":/icons/close\"/></p></body></html>");

    connect(labelMinimizeIcon, SIGNAL(clicked()), minimizeAction, SLOT(trigger()));
    connect(labelMinimizeIcon, SIGNAL(hovered()), this, SLOT(minHover()));
    connect(labelMinimizeIcon, SIGNAL(unhovered()), this, SLOT(minUnhover()));

    connect(labelCloseIcon, SIGNAL(clicked()), closeAction, SLOT(trigger()));
    connect(labelCloseIcon, SIGNAL(hovered()), this, SLOT(closeHover()));
    connect(labelCloseIcon, SIGNAL(unhovered()), this, SLOT(closeUnhover()));

    layout->addStretch();

    layout->addWidget(labelMinimizeIcon);
    layout->addWidget(labelCloseIcon);

    layout->setContentsMargins(0,0,0,0);


    setMenuWidget(w);


    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(backupWalletAction);
    file->addAction(signMessageAction);
    file->addAction(verifyMessageAction);
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    settings->addAction(encryptWalletAction);
    settings->addAction(changePassphraseAction);
    settings->addAction(unlockWalletAction);
    settings->addAction(lockWalletAction);
    settings->addSeparator();
    settings->addAction(optionsAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);


	/* zeewolf: Hot swappable wallet themes */
    if (themesList.count()>0)
    {
        QMenu *themes = appMenuBar->addMenu(tr("&Themes"));
        for (int i = 0; i < themesList.count(); i++) {
            themes->addAction(customActions[i]);
        }
    }
    /* zeewolf: Hot swappable wallet themes */
}


void BitcoinGUI::createToolBars()
{

    mainToolbar = addToolBar(tr("Tabs toolbar"));
    mainToolbar->setObjectName("toolbar");
    addToolBar(Qt::LeftToolBarArea, mainToolbar);
    mainToolbar->setOrientation(Qt::Vertical);
    mainToolbar->setMovable(false);
    mainToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mainToolbar->addAction(overviewAction);
    mainToolbar->widgetForAction(overviewAction)->setObjectName("OverviewButton");
    mainToolbar->addAction(sendCoinsAction);
    mainToolbar->widgetForAction(sendCoinsAction)->setObjectName("SendButton");
    mainToolbar->addAction(receiveCoinsAction);
    mainToolbar->widgetForAction(receiveCoinsAction)->setObjectName("ReceiveButton");
    mainToolbar->addAction(addressBookAction);
    mainToolbar->widgetForAction(addressBookAction)->setObjectName("AddressBookButton");
    mainToolbar->addAction(stakeCoinsAction);
    mainToolbar->widgetForAction(stakeCoinsAction)->setObjectName("SideStakeButton");
    mainToolbar->addAction(historyAction);
    mainToolbar->widgetForAction(historyAction)->setObjectName("HistoryButton");
    mainToolbar->addAction(messageAction);
    mainToolbar->widgetForAction(messageAction)->setObjectName("MessageButton");
    mainToolbar->setContextMenuPolicy(Qt::NoContextMenu);

    mainToolbar->layout()->setContentsMargins(0, 0, 0, 0);
    mainToolbar->layout()->setSpacing(0);
}

void BitcoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Replace some strings and icons, when using the testnet
        if(clientModel->isTestNet())
        {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(":icons/bitcoin_testnet"));
            setWindowIcon(QIcon(":icons/bitcoin_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcoin_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(tr("Pinkcoin client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
                toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            }

            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
        connect(clientModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));

        // Report errors from network/worker thread
        connect(clientModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        rpcConsole->setClientModel(clientModel);
        addressBookPage->setOptionsModel(clientModel->getOptionsModel());
        stakeCoinsPage->setOptionsModel(clientModel->getOptionsModel());
        receiveCoinsPage->setOptionsModel(clientModel->getOptionsModel());
    }
}

void BitcoinGUI::setWalletModel(WalletModel *walletModel, WalletModel *stakeModel)
{
    this->walletModel = walletModel;
    this->stakeModel = stakeModel;
    if(walletModel)
    {
        // Report errors from wallet thread
        connect(walletModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);

        overviewPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveCoinsPage->setModel(walletModel->getAddressTableModel());
        sendCoinsPage->setModel(walletModel);
        signVerifyMessageDialog->setModel(walletModel);

        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));

    }
    if(stakeModel)
        stakeCoinsPage->setModel(stakeModel->getAddressTableModel());
}

void BitcoinGUI::setMessageModel(MessageModel *messageModel)
{
    this->messageModel = messageModel;
    if(messageModel)
    {
        // Report errors from message thread
        connect(messageModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        // Put transaction list in tabs
        messagePage->setModel(messageModel);

        // Balloon pop-up for new message
        connect(messageModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingMessage(QModelIndex,int,int)));
    }
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("Pinkcoin client"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon, this);
}

#ifndef Q_OS_MAC
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void BitcoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel(), &dlg);
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::minHover()
{
    labelMinimizeIcon->setText("<html><head/><body><p><img src=\":/icons/min_s\"/></p></body></html>");
}

void BitcoinGUI::minUnhover()
{
    labelMinimizeIcon->setText("<html><head/><body><p><img src=\":/icons/min\"/></p></body></html>");
}

void BitcoinGUI::closeHover()
{
    labelCloseIcon->setText("<html><head/><body><p><img src=\":/icons/close_s\"/></p></body></html>");
}

void BitcoinGUI::closeUnhover()
{
    labelCloseIcon->setText("<html><head/><body><p><img src=\":/icons/close\"/></p></body></html>");
}

void BitcoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to Pinkcoin network", "", count));
}

void BitcoinGUI::setNumBlocks(int count, int nTotalBlocks)
{
    // don't show / hide progress bar and its label if we have no connection to the network
    if (!clientModel || clientModel->getNumConnections() == 0)
    {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);

        return;
    }

    QString strStatusBarWarnings = clientModel->getStatusBarWarnings();
    QString tooltip;

    if(count < nTotalBlocks)
    {
        int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);

        if (strStatusBarWarnings.isEmpty())
        {
            progressBarLabel->setText(tr("Synchronizing with network..."));
            progressBarLabel->setVisible(true);
            progressBar->setFormat(tr("~%n block(s) remaining", "", nRemainingBlocks));
            progressBar->setMaximum(nTotalBlocks);
            progressBar->setValue(count);
            progressBar->setVisible(true);
        }

        tooltip = tr("Downloaded %1 of %2 blocks of transaction history (%3% done).").arg(count).arg(nTotalBlocks).arg(nPercentageDone, 0, 'f', 2);
    }
    else
    {
        if (strStatusBarWarnings.isEmpty())
            progressBarLabel->setVisible(false);

        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    // Override progressBarLabel text and hide progress bar, when we have warnings to display
    if (!strStatusBarWarnings.isEmpty())
    {
        progressBarLabel->setText(strStatusBarWarnings);
        progressBarLabel->setVisible(true);
        progressBar->setVisible(false);
    }

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    QString text;

    // Represent time from last generated block in human readable text
    if(secs <= 0)
    {
        // Fully up to date. Leave text empty.
    }
    else if(secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    }
    else if(secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    }
    else if(secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    }
    else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60 && count >= nTotalBlocks)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        overviewPage->showOutOfSyncWarning(false);
    }
    else
    {
        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        labelBlocksIcon->setMovie(syncIconMovie);
        syncIconMovie->start();

        overviewPage->showOutOfSyncWarning(true);
    }

    if(!text.isEmpty())
    {
        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1.").arg(text);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::error(const QString &title, const QString &message, bool modal)
{
    // Report errors from network/worker thread
    if(modal)
    {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void BitcoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage =
        tr("This transaction is over the size limit.  You can still send it for a fee of %1, "
          "which goes to the nodes that process your transaction and helps to support the network.  "
          "Do you want to pay the fee?").arg(
                BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitcoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    if(!walletModel || !clientModel)
        return;
    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();

    QString fNotificationLevel;

    fNotificationLevel = walletModel->getOptionsModel()->getNotificationLevel();

    if(fNotificationLevel != tr("Disable Notifications"))
    {
        if(!clientModel->inInitialBlockDownload())
        {
            // On new transaction, make an info balloon
            // Unless the initial block download is in progress, to prevent balloon-spam
            QString date = ttm->index(start, TransactionTableModel::Date, parent)
                            .data().toString();
            QString type = ttm->index(start, TransactionTableModel::Type, parent)
                            .data().toString();
            QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                            .data().toString();
            QIcon icon = qvariant_cast<QIcon>(ttm->index(start,
                                TransactionTableModel::ToAddress, parent)
                            .data(Qt::DecorationRole));
            bool skip = false;
            if((fNotificationLevel  == tr("Notify on Send/Receipt of Coins Only")) && (type == tr("Minted")))
                skip = true;
            if((fNotificationLevel  == tr("Notify on Send/Receipt of Coins Only")) && (type == tr("Mined")))
                skip = true;

            if (!skip)
            {
                notificator->notify(Notificator::Information,
                                (amount)<0 ? tr("Sent transaction") :
                                             tr("Incoming transaction"),
                                  tr("Date: %1\n"
                                     "Amount: %2\n"
                                     "Type: %3\n"
                                     "Address: %4\n")
                                .arg(date)
                                .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                                .arg(type)
                                .arg(address), icon);
            }
        }
    }
}

void BitcoinGUI::incomingMessage(const QModelIndex & parent, int start, int end)
{
    if(!messageModel)
        return;

    MessageModel *mm = messageModel;

    if (mm->index(start, MessageModel::TypeInt, parent).data().toInt() == MessageTableEntry::Received)
    {
        QString sent_datetime = mm->index(start, MessageModel::ReceivedDateTime, parent).data().toString();
        QString from_address  = mm->index(start, MessageModel::FromAddress,      parent).data().toString();
        QString to_address    = mm->index(start, MessageModel::ToAddress,        parent).data().toString();
        QString message       = mm->index(start, MessageModel::Message,          parent).data().toString();
        QTextDocument html;
        html.setHtml(message);
        QString messageText(html.toPlainText());
        notificator->notify(Notificator::Information,
                            tr("Incoming Message"),
                            tr("Date: %1\n"
                               "From Address: %2\n"
                               "To Address: %3\n"
                               "Message: %4\n")
                              .arg(sent_datetime)
                              .arg(from_address)
                              .arg(to_address)
                              .arg(messageText));
    };
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralWidget->setCurrentWidget(overviewPage);

}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(transactionsPage);

}

void BitcoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(addressBookPage);

}

void BitcoinGUI::gotoStakeCoinsPage()
{
    stakeCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(stakeCoinsPage);

}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(receiveCoinsPage);

}

void BitcoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(sendCoinsPage);

}

void BitcoinGUI::gotoMessagePage()
{
    messageAction->setChecked(true);
    centralWidget->setCurrentWidget(messagePage);

}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    // call show() in showTab_SM()
    signVerifyMessageDialog->showTab_SM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    // call show() in showTab_VM()
    signVerifyMessageDialog->showTab_VM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            if (sendCoinsPage->handleURI(uri.toString()))
                nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            gotoSendCoinsPage();
        else
            notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Pinkcoin address or malformed URI parameters."));
    }

    event->acceptProposedAction();
}

void BitcoinGUI::handleURI(QString strURI)
{
    // URI has to be valid
    if (sendCoinsPage->handleURI(strURI))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
    }
    else
        notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Pinkcoin address or malformed URI parameters."));
}

void BitcoinGUI::mainToolbarOrientation(Qt::Orientation orientation)
{
    if(orientation == Qt::Horizontal)
    {
        messageAction->setIconText(tr("Private &Messages"));
    }
}

//void BitcoinGUI::secondaryToolbarOrientation(Qt::Orientation orientation)
//{
//    secondaryToolbar->setStyleSheet(orientation == Qt::Horizontal ? HORIZONTAL_TOOLBAR_STYLESHEET : VERTICAL_TOOBAR_STYLESHEET);
//}

void BitcoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        disconnect(labelEncryptionIcon, SIGNAL(clicked()), unlockWalletAction, SLOT(trigger()));
        disconnect(labelEncryptionIcon, SIGNAL(clicked()),   lockWalletAction, SLOT(trigger()));
        connect   (labelEncryptionIcon, SIGNAL(clicked()),   lockWalletAction, SLOT(trigger()));
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>")); // TODO: Click to lock + translations
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        disconnect(labelEncryptionIcon, SIGNAL(clicked()), unlockWalletAction, SLOT(trigger()));
        disconnect(labelEncryptionIcon, SIGNAL(clicked()),   lockWalletAction, SLOT(trigger()));
        connect   (labelEncryptionIcon, SIGNAL(clicked()), unlockWalletAction, SLOT(trigger()));
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>")); // TODO: Click to unlock + translations
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}



void BitcoinGUI::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt:
                                     AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void BitcoinGUI::backupWallet()
{
    QString saveDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));
    if(!filename.isEmpty()) {
        if(!walletModel->backupWallet(filename)) {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the wallet data to the new location."));
        }
    }
}

void BitcoinGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void BitcoinGUI::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog::Mode mode = sender() == unlockWalletAction ?
              AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
        AskPassphraseDialog dlg(mode, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
    AddressTableModel *UpdateAddresses = walletModel->getAddressTableModel();
    UpdateAddresses->refreshAddresses();
}

void BitcoinGUI::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
    AddressTableModel *UpdateAddresses = walletModel->getAddressTableModel();
    UpdateAddresses->refreshAddresses();
}

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcoinGUI::updateWeight()
{
    if (!pwalletMain)
        return;

    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;

    TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
    if (!lockWallet)
        return;

    uint64_t nMinWeight = 0, nMaxWeight = 0;
    pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);
}

void BitcoinGUI::updateStakingIcon()
{
    updateWeight();

    if (nLastCoinStakeSearchInterval && nWeight)
    {
        uint64_t nNetworkWeight = GetPoSKernelPS();

        uint64_t chanceToStake;
        if (nNetworkWeight > 0)
        {
            uint64_t diff;

            chanceToStake = (nWeight * 10000) / nNetworkWeight;

            time_t rawtime;
            time ( &rawtime );
            uint64_t nStakesPerHour;
            if(IsFlashStake(rawtime))
                nStakesPerHour = 60;
            else
                nStakesPerHour = 10;

            for (uint64_t i=0; i<nStakesPerHour; i++ )
            {
                diff = 10000 - chanceToStake;
                chanceToStake += (nWeight * diff) / nNetworkWeight;
            }

            chanceToStake = chanceToStake / 100;

            if (chanceToStake > 100)
                chanceToStake = 100;
        } else {
            chanceToStake = 100;
        }

        labelStakingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Staking.<br>Your weight is: %1<br>Network weight is: %2<br>Chance to stake within an hour: %3\%").arg(nWeight).arg(nNetworkWeight).arg(chanceToStake));
    }
    else
    {
        labelStakingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        if (pwalletMain && pwalletMain->IsLocked())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is locked"));
        else if (vNodes.empty())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is offline"));
        else if (IsInitialBlockDownload())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is syncing"));
        else if (!nWeight)
            labelStakingIcon->setToolTip(tr("Not staking because you don't have mature coins"));
        else
            labelStakingIcon->setToolTip(tr("Not staking"));
    }
}

/* zeewolf: Hot swappable wallet themes */
void BitcoinGUI::changeTheme(QString theme)
{
    // load Default theme first (if present) to apply default styles
    loadTheme("Celestial");

    if (theme != "Celestial") {
        loadTheme(theme);
    }
}

void BitcoinGUI::loadTheme(QString theme)
{
    // template variables : key => value
    QMap<QString, QString> variables;

    // path to selected theme dir - for simpler use, just use $theme-dir in qss : url($theme-dir/image.png)
    QString themeDir = themesDir + "/" + theme;

    // if theme selected
    if (theme != "") {
        QFile qss(themeDir + "/styles.qss");
        // open qss
        if (qss.open(QFile::ReadOnly))
        {
            // read stylesheet
            QString styleSheet = QString(qss.readAll());
            QTextStream in(&qss);
            // rewind
            in.seek(0);
            bool readingVariables = false;

            // seek for variables
            while(!in.atEnd()) {
                QString line = in.readLine();
                // variables starts here
                if (line == "/** [VARS]") {
                    readingVariables = true;
                }
                // variables end here
                if (line == "[/VARS] */") {
                    break;
                }
                // if we're reading variables - store them in a map
                if (readingVariables == true) {
                    // skip empty lines
                    if (line.length()>3 && line.contains('=')) {
                        QStringList fields = line.split("=");
                        QString var = fields.at(0).trimmed();
                        QString value = fields.at(1).trimmed();
                        variables[var] = value;
                    }
                }
            }

            // replace path to themes dir
            styleSheet.replace("$theme-dir", themeDir);
            styleSheet.replace("$themes-dir", themesDir);

            QMapIterator<QString, QString> variable(variables);
            variable.toBack();
            // iterate backwards to prevent overwriting variables
            while (variable.hasPrevious()) {
                variable.previous();
                // replace variables
                styleSheet.replace(variable.key(), variable.value());
            }

            qss.close();

            // Apply the result qss file to Qt

            qApp->setStyleSheet(styleSheet);
        }
    } else {
        // If not theme name given - clear styles
        qApp->setStyleSheet(QString(""));
    }

    // set selected theme and store it in registry
    selectedTheme = theme;
    QSettings settings;
    settings.setValue("Template", selectedTheme);
}

void BitcoinGUI::listThemes(QStringList& themes)
{
    QDir currentDir(qApp->applicationDirPath());
    // try app dir
    if (currentDir.cd("themes")) {
    // got it! (win package)
    } else if (currentDir.cd("src/qt/res/themes")) {
        // got it
    } else if (currentDir.cd("../src/qt/res/themes")) {
        // got it
    } else {
        // themes not found :(
        return;
    }
    themesDir = currentDir.path();
    currentDir.setFilter(QDir::Dirs);
    QStringList entries = currentDir.entryList();
    for( QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry )
    {
        QString themeName=*entry;
        if(themeName != tr(".") && themeName != tr(".."))
        {
            themes.append(themeName);
        }
    }

    // get selected theme from registry (if any)
    QSettings settings;
    selectedTheme = settings.value("Template").toString();
    // or use default theme
    if (selectedTheme=="") {
        selectedTheme = "Default";
    }
    // load it!
    loadTheme(selectedTheme);
}

void BitcoinGUI::keyPressEvent(QKeyEvent * e)
{
    switch (e->type())
     {
       case QEvent::KeyPress:
         // $ key
         if (e->key() == 36) {
             // dev feature: key reloads selected theme
             loadTheme(selectedTheme);
         }
         break;
       default:
         break;
     }

}

/* zeewolf: Hot swappable wallet themes */


void BitcoinGUI::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        mMoving = true;
        // Difference between window position and global position of mouse cursor
        mDiffWindowPosition = this->pos() - event->globalPos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void BitcoinGUI::mouseMoveEvent(QMouseEvent* event)
{
    if( event->buttons().testFlag(Qt::LeftButton) && mMoving)
    {
        this->move(mDiffWindowPosition + event->globalPos());
    }
}

void BitcoinGUI::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        mMoving = false;
        unsetCursor();
    }
}
