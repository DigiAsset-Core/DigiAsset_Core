#include "Config.h"
#include "Database.h"
#include "DigiByteCore.h"
#include "PlotManager.h"
#include "RPC/Server.h"
#include "RPCLoader.h"
#include "tabs/BalancesTab.h"
#include "tabs/CreateAssetTab.h"
#include "tabs/SendAssetTab.h"
#include "tabs/HistoryTab.h"
#include "tabs/ManageAssetTab.h"
#include "tabs/SyncTab.h"
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMovie>
#include <QProcess>
#include <QProgressBar>
#include <QTabWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>
#include <chrono>
#include <regex>

QProcess *coreProcess = nullptr;
QProgressBar *progressBar = nullptr;
QLabel *statusLabel = nullptr;
QLabel *timeLabel = nullptr;
QLabel *loadingSpinner = nullptr;
QMovie *spinnerMovie = nullptr;
QTabWidget *tabWidget = nullptr;
RPCLoader loader;  // Create a single instance of Loader

int previousRemaining = 0;
double estimatedTime = -1;  //give impossible value as flag to show it is not set
std::chrono::steady_clock::time_point lastUpdateTime;

void startCoreProcess() {
    if (coreProcess && coreProcess->state() != QProcess::NotRunning) {
        qDebug() << "Core process is already running.";
        return;
    }

    //don't spawn a second daemon if one is already serving the RPC port
    //(two daemons would run two chain analyzers over the same database)
    try {
        Config config("config.cfg");
        QTcpSocket probe;
        //the asset daemon(this project) is reached via rpcassetbind, not rpcbind(which points at
        //the DigiByte Core node).  Probing the wrong host would spawn a duplicate local daemon.
        probe.connectToHost(QString::fromStdString(config.getString("rpcassetbind", "127.0.0.1")),
                            static_cast<quint16>(config.getInteger("rpcassetport", 14024)));
        if (probe.waitForConnected(500)) {
            probe.disconnectFromHost();
            qDebug() << "A DigiAsset Core daemon is already serving the RPC port; not starting another.";
            return;
        }
    } catch (...) {
        //no readable config yet - the daemon's own startup wizard will handle it
    }

    coreProcess = new QProcess();
    QString coreExecutable = QCoreApplication::applicationDirPath() + "/digiasset_core";
    coreProcess->start(coreExecutable);

    QObject::connect(coreProcess, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
        qDebug() << "Core process error:" << error;
    });

    qDebug() << "Core process started: " << coreExecutable;
}

QString estimateTimeRemaining(int sync, int count) {
    if (sync>0) return "Calculating";  //not currently syncing so can't estimate

    using namespace std::chrono;

    //determine time taken between samples
    auto now = steady_clock::now();
    double elapsed = duration_cast<seconds>(now - lastUpdateTime).count();

    //determine rate of catch up
    int remaining = 0 - sync;   //sync is negative number for how many blocks behind it is.
    int catchUpCount = previousRemaining-remaining; //get how much closer we are to being synced then before
    double syncSpeed = catchUpCount / elapsed;
    double currentEstimate = remaining / syncSpeed;

    //update previous samples
    previousRemaining = remaining;
    lastUpdateTime = now;

    if (syncSpeed > 0) {
        if (estimatedTime==-1) estimatedTime=currentEstimate;    //handle first sync time
        estimatedTime = (0.7 * estimatedTime) + (0.3 * currentEstimate);  // Smoothing factor

        int days = static_cast<int>(estimatedTime) / 86400;
        int hours = (static_cast<int>(estimatedTime) % 86400) / 3600;
        int minutes = (static_cast<int>(estimatedTime) % 3600) / 60;

        if (days > 0) {
            return QString("%1 days %2 hrs %3 min")
                    .arg(days).arg(hours).arg(minutes);
        } else if (hours > 0) {
            return QString("%1 hrs %2 min")
                    .arg(hours).arg(minutes);
        } else if (estimatedTime >= 60) {
            return QString("%1 min")
                    .arg(minutes);
        } else {
            return QString("less than a minute");
        }
    } else {
        //falling behind in sync.  Can happen if blocks are densely packed or machine is really slow.
        return "Calculating";
    }
}


void updateLoadingProgress(QTimer *timer, QWidget &splash) {
    DigiByteCore& dgb = loader.getCore();

    try {
        Json::Value args = Json::arrayValue;
        Json::Value result = dgb.sendcommand("syncstate", args);

        int count = result["count"].asInt();
        int sync = result["sync"].asInt();
        int progress = count + sync;
        int percentage = (progress * 100) / count;

        progressBar->setValue(percentage);

        QString syncMessage;
        switch (sync) {
            case 1:
                syncMessage = "Sync Stopped. Something is wrong!";
                progressBar->setVisible(false);
                loadingSpinner->setVisible(true);
                spinnerMovie->start();
                break;
            case 2:
                syncMessage = "Initializing...";
                progressBar->setVisible(false);
                loadingSpinner->setVisible(true);
                spinnerMovie->start();
                break;
            case 3:
                syncMessage = "Rewinding blocks...";
                progressBar->setVisible(false);
                loadingSpinner->setVisible(true);
                spinnerMovie->start();
                break;
            case 4:
                syncMessage = "Optimizing blocks...";
                progressBar->setVisible(false);
                loadingSpinner->setVisible(true);
                spinnerMovie->start();
                break;
            default:
                syncMessage = "Syncing Blocks: " + QString::number(progress) + " / " + QString::number(count);
                progressBar->setVisible(true);
                loadingSpinner->setVisible(false);
                spinnerMovie->stop();

                QString timeEstimate = estimateTimeRemaining(sync, count);
                timeLabel->setText("Estimated time remaining: " + timeEstimate);
                break;
        }
        statusLabel->setText(syncMessage);

        if (sync == 0) {
            timer->stop();
            splash.close();

            tabWidget = new QTabWidget();
            SyncTab *syncTab = new SyncTab();
            tabWidget->addTab(syncTab, "Sync Status");
            tabWidget->addTab(new BalancesTab(), "Balances");
            tabWidget->addTab(new SendAssetTab(), "Send Asset");
            tabWidget->addTab(new CreateAssetTab(), "Create Asset");
            tabWidget->addTab(new ManageAssetTab(), "Burn / Reissue");
            tabWidget->addTab(new HistoryTab(), "History");
            tabWidget->resize(800, 600);
            tabWidget->show();
        }

    } catch (const DigiByteException &e) {
        statusLabel->setText("Error: " + QString::fromStdString(e.getMessage()));
    }
}

void showLoadingScreen(QApplication &app) {
    QWidget splash;
    splash.setFixedSize(400, 250);
    splash.setWindowTitle("Loading...");

    QVBoxLayout *layout = new QVBoxLayout(&splash);
    statusLabel = new QLabel("Starting DigiAsset Core...");
    statusLabel->setAlignment(Qt::AlignCenter);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setVisible(false);

    timeLabel = new QLabel("Estimated time remaining: Calculating");
    timeLabel->setAlignment(Qt::AlignCenter);

    loadingSpinner = new QLabel();
    QString spinnerPath = QCoreApplication::applicationDirPath() + "/images/spinner.gif";
    spinnerMovie = new QMovie(spinnerPath);
    loadingSpinner->setMovie(spinnerMovie);
    loadingSpinner->setAlignment(Qt::AlignCenter);
    loadingSpinner->setVisible(true);
    spinnerMovie->start();

    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
    layout->addWidget(timeLabel);
    layout->addWidget(loadingSpinner);

    splash.show();

    QTimer *timer = new QTimer(&splash);
    QObject::connect(timer, &QTimer::timeout, [&]() {
        updateLoadingProgress(timer, splash);
    });

    lastUpdateTime = std::chrono::steady_clock::now();
    timer->start(15000);
    app.exec();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/images/app_icon.png"));

    startCoreProcess();
    showLoadingScreen(app);

    return 0;
}
