#include "HistoryTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <cmath>

HistoryTab::HistoryTab(QWidget *parent) : QWidget(parent), _dgbCore() {
    _dgbCore.setFileName("config.cfg", true);
    _dgbCore.makeConnection();
    _icons = new AssetIconProvider(this);
    connect(_icons, &AssetIconProvider::iconReady, this, &HistoryTab::applyIcon);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Recent wallet transactions"));
    _filterEdit = new QLineEdit();
    _filterEdit->setPlaceholderText("Filter(txid, type, address, amount...)");
    _filterEdit->setClearButtonEnabled(true);
    connect(_filterEdit, &QLineEdit::textChanged, this, &HistoryTab::applyFilter);
    topRow->addWidget(_filterEdit, 1);
    _refreshButton = new QPushButton("Refresh");
    connect(_refreshButton, &QPushButton::clicked, this, &HistoryTab::updateHistory);
    topRow->addWidget(_refreshButton);
    layout->addLayout(topRow);

    _txTable = new QTableWidget(0, COL_COUNT, this);
    _txTable->setHorizontalHeaderLabels(
            {"Time", "Type", "Amount (DGB)", "Asset", "Confirmations", "Address", "Transaction ID"});
    _txTable->horizontalHeader()->setSectionResizeMode(COL_TXID, QHeaderView::Stretch);
    _txTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _txTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _txTable->setIconSize(QSize(_icons->iconSize(), _icons->iconSize()));
    layout->addWidget(_txTable);

    QHBoxLayout *pageRow = new QHBoxLayout();
    _newerButton = new QPushButton("< Newer");
    connect(_newerButton, &QPushButton::clicked, this, &HistoryTab::newerPage);
    pageRow->addWidget(_newerButton);
    _pageLabel = new QLabel("");
    pageRow->addWidget(_pageLabel);
    _olderButton = new QPushButton("Older >");
    connect(_olderButton, &QPushButton::clicked, this, &HistoryTab::olderPage);
    pageRow->addWidget(_olderButton);
    pageRow->addStretch();
    layout->addLayout(pageRow);

    _statusLabel = new QLabel("Select a row and copy the transaction id to inspect it with getrawtransaction.");
    _statusLabel->setWordWrap(true);
    layout->addWidget(_statusLabel);

    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, [this]() {
        if (_page == 0) updateHistory(); //don't yank the user off an older page
    });
    _timer->start(30000);

    updateHistory();
}

void HistoryTab::updateHistory() {
    try {
        Json::Value args = Json::arrayValue;
        args.append("*");
        args.append(PAGE_SIZE);
        args.append(_page * PAGE_SIZE); //skip
        Json::Value result = _dgbCore.sendcommand("listtransactions", args);
        _lastCount = result.size();

        //newest first
        _txTable->setRowCount(result.size());
        int row = 0;
        for (int i = static_cast<int>(result.size()) - 1; i >= 0; i--) {
            const Json::Value &tx = result[i];
            std::string txid = tx["txid"].asString();
            std::string address = tx["address"].asString();
            QString category = QString::fromStdString(tx["category"].asString());

            //decode the transaction to see if a DigiAsset moved to this row's output
            QString assetText;
            int confirmations = tx["confirmations"].asInt();
            const std::map<std::string, AssetMove> byAddress = txAssets(txid, confirmations);
            auto it = byAddress.find(address);
            bool isAsset = (it != byAddress.end());
            if (isAsset) {
                const AssetMove &move = it->second;
                double amount = static_cast<double>(move.count) / std::pow(10.0, move.decimals);
                QString name = _icons->name(move.assetIndex);
                if (name.isEmpty()) name = "asset " + QString::number(move.assetIndex);
                assetText = name + "  " + QString::number(amount, 'f', move.decimals);
                category = "asset " + category; //e.g. "asset receive", "asset send"
            }

            QString when = QDateTime::fromSecsSinceEpoch(tx["time"].asInt64()).toString("yyyy-MM-dd hh:mm");
            _txTable->setItem(row, COL_TIME, new QTableWidgetItem(when));
            _txTable->setItem(row, COL_TYPE, new QTableWidgetItem(category));
            _txTable->setItem(row, COL_DGB, new QTableWidgetItem(QString::number(tx["amount"].asDouble(), 'f', 8)));

            QTableWidgetItem *assetItem = new QTableWidgetItem(assetText);
            if (isAsset) {
                assetItem->setData(Qt::UserRole, (qulonglong) it->second.assetIndex); //for applyIcon()
                QIcon icon = _icons->icon(it->second.assetIndex);
                if (!icon.isNull()) assetItem->setIcon(icon);
            }
            _txTable->setItem(row, COL_ASSET, assetItem);

            _txTable->setItem(row, COL_CONF, new QTableWidgetItem(QString::number(tx["confirmations"].asInt())));
            _txTable->setItem(row, COL_ADDRESS, new QTableWidgetItem(QString::fromStdString(address)));
            _txTable->setItem(row, COL_TXID, new QTableWidgetItem(QString::fromStdString(txid)));
            row++;
        }
        //a successful fetch clears any earlier error message
        if (result.empty() && (_page == 0)) {
            _statusLabel->setText("No wallet transactions yet.");
        } else {
            _statusLabel->setText("Select a row and copy the transaction id to inspect it with getrawtransaction.");
        }

        _pageLabel->setText(QString("page %1").arg(_page + 1));
        _newerButton->setEnabled(_page > 0);
        _olderButton->setEnabled(_lastCount == PAGE_SIZE); //a short page is the oldest one

        applyFilter();
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Error fetching history: " + QString::fromStdString(e.getMessage()));
    }
}

///hides rows that don't contain the filter text in any column(case insensitive)
void HistoryTab::applyFilter() {
    QString needle = _filterEdit->text().trimmed();
    for (int row = 0; row < _txTable->rowCount(); row++) {
        bool match = needle.isEmpty();
        for (int col = 0; !match && (col < _txTable->columnCount()); col++) {
            QTableWidgetItem *item = _txTable->item(row, col);
            if ((item != nullptr) && item->text().contains(needle, Qt::CaseInsensitive)) match = true;
        }
        _txTable->setRowHidden(row, !match);
    }
}

/**
 * Decodes a transaction(getrawtransaction verbose) once and caches, keyed by receiving address,
 * the DigiAsset that moved to each output.  Lets updateHistory() label a wallet transaction as an
 * asset movement and show which asset and how much moved to the wallet's address.
 */
std::map<std::string, HistoryTab::AssetMove> HistoryTab::txAssets(const std::string &txid, int confirmations) {
    auto cached = _txAssetCache.find(txid);
    if (cached != _txAssetCache.end()) return cached->second;

    std::map<std::string, AssetMove> byAddress;
    bool decodeFailed = false;
    try {
        Json::Value args = Json::arrayValue;
        args.append(txid);
        args.append(true); //verbose - includes DigiAssets data
        Json::Value tx = _dgbCore.sendcommand("getrawtransaction", args);
        for (const auto &vout: tx["vout"]) {
            if (!vout.isMember("assets") || !vout["assets"].isArray() || vout["assets"].empty()) continue;
            const Json::Value &spk = vout["scriptPubKey"];
            //take the first asset on the output(outputs carry a single asset in practice)
            const Json::Value &asset = vout["assets"][0];
            AssetMove move;
            move.assetIndex = asset["assetIndex"].asUInt64();
            move.count = asset["count"].asUInt64();
            move.decimals = asset["decimals"].asUInt();
            //index by every address on the output so it matches the listtransactions row address
            if (spk.isMember("address")) {
                byAddress[spk["address"].asString()] = move;
            }
            if (spk.isMember("addresses") && spk["addresses"].isArray()) {
                for (const auto &a: spk["addresses"]) byAddress[a.asString()] = move;
            }
        }
    } catch (const DigiByteException &) {
        //an unconfirmed transaction has no blockhash yet, so the daemon can't decode its
        //assets and this call fails - treat as a plain DGB transaction for now, but(below)
        //don't cache that verdict so it gets re-checked once the transaction confirms
        decodeFailed = true;
    }

    //only cache once confirmed: a 0-conf tx that decoded as "no assets" may just be too new
    //for the daemon's chain-indexed asset lookup to see yet, not actually a plain transaction
    if ((confirmations > 0) && !decodeFailed) {
        _txAssetCache.emplace(txid, byAddress);
    }
    return byAddress;
}

///sets the icon on the Asset cell of every row showing the asset once its download finishes
void HistoryTab::applyIcon(quint64 assetIndex) {
    QIcon icon = _icons->icon(assetIndex);
    if (icon.isNull()) return;
    for (int row = 0; row < _txTable->rowCount(); row++) {
        QTableWidgetItem *item = _txTable->item(row, COL_ASSET);
        if ((item != nullptr) && (item->data(Qt::UserRole).toULongLong() == assetIndex)) {
            item->setIcon(icon);
        }
    }
}

void HistoryTab::olderPage() {
    if (_lastCount < PAGE_SIZE) return; //already on the oldest page
    _page++;
    updateHistory();
}

void HistoryTab::newerPage() {
    if (_page == 0) return;
    _page--;
    updateHistory();
}
