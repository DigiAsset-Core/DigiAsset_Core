#include "BalancesTab.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

BalancesTab::BalancesTab(QWidget *parent) : QWidget(parent), _dgbCore() {
    _dgbCore.setFileName("config.cfg", true);
    _dgbCore.makeConnection();
    _icons = new AssetIconProvider(this);
    connect(_icons, &AssetIconProvider::iconReady, this, &BalancesTab::applyIcon);

    QVBoxLayout *layout = new QVBoxLayout(this);

    //DigiByte balance + refresh button on one row, with DigiByte's own icon alongside it.
    //DigiByte is asset index 1 in the metadata(see Database.cpp), so its icon is fetched
    //through the same provider as any other asset.
    QHBoxLayout *topRow = new QHBoxLayout();
    _dgbIconLabel = new QLabel();
    topRow->addWidget(_dgbIconLabel);
    _digibyteLabel = new QLabel("DigiByte: Loading...");
    topRow->addWidget(_digibyteLabel);
    topRow->addStretch();
    _refreshButton = new QPushButton("Refresh");
    connect(_refreshButton, &QPushButton::clicked, this, &BalancesTab::updateBalances);
    topRow->addWidget(_refreshButton);
    layout->addLayout(topRow);

    //asset table
    _assetTable = new QTableWidget(0, 5, this);
    _assetTable->setHorizontalHeaderLabels({"Name", "Asset ID", "Index", "Amount", "Decimals"});
    _assetTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    _assetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _assetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _assetTable->setIconSize(QSize(_icons->iconSize(), _icons->iconSize()));
    layout->addWidget(_assetTable);

    _statusLabel = new QLabel("");
    layout->addWidget(_statusLabel);

    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &BalancesTab::updateBalances);
    _timer->start(30000); //update every 30 seconds

    setDgbIcon(); //request DigiByte's icon(index 1); fills in when its download finishes
    updateBalances();
}

///shows DigiByte's icon(asset index 1) next to the balance once the download completes
void BalancesTab::setDgbIcon() {
    QIcon icon = _icons->icon(1);
    if (icon.isNull()) return;
    int s = _icons->iconSize();
    _dgbIconLabel->setPixmap(icon.pixmap(s, s));
}

///sets the cached icon on whichever row currently shows the asset
void BalancesTab::applyIcon(uint64_t assetIndex) {
    if (assetIndex == 1) { //DigiByte native coin - shown next to the balance, not in the table
        setDgbIcon();
        return;
    }
    QIcon icon = _icons->icon(assetIndex);
    if (icon.isNull()) return;
    for (int row = 0; row < _assetTable->rowCount(); row++) {
        QTableWidgetItem *indexItem = _assetTable->item(row, 2);
        if ((indexItem != nullptr) && (indexItem->text().toULongLong() == assetIndex)) {
            QTableWidgetItem *nameItem = _assetTable->item(row, 0);
            if (nameItem != nullptr) nameItem->setIcon(icon);
        }
    }
}

void BalancesTab::updateBalances() {
    try {
        Json::Value args = Json::arrayValue;
        Json::Value result = _dgbCore.sendcommand("getwalletbalances", args);

        _digibyteLabel->setText("DigiByte: " + QString::fromStdString(result["digibyte"]["amount"].asString()) + " DGB");

        const Json::Value &assets = result["assets"];
        _assetTable->setRowCount(assets.size());
        int row = 0;
        for (const auto &asset: assets) {
            uint64_t assetIndex = asset["assetIndex"].asUInt64();
            QString name = _icons->name(assetIndex);
            QTableWidgetItem *nameItem = new QTableWidgetItem(name);
            QIcon icon = _icons->icon(assetIndex);
            if (!icon.isNull()) nameItem->setIcon(icon);
            _assetTable->setItem(row, 0, nameItem);
            _assetTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(asset["assetId"].asString())));
            _assetTable->setItem(row, 2, new QTableWidgetItem(QString::number(assetIndex)));
            _assetTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(asset["amount"].asString())));
            _assetTable->setItem(row, 4, new QTableWidgetItem(QString::number(asset["decimals"].asUInt())));
            row++;
        }
        _statusLabel->setText(assets.empty() ? "No assets in wallet." : "");
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Error fetching balances: " + QString::fromStdString(e.getMessage()));
    }
}
