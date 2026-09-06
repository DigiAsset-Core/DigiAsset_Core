//
// Created by DigiAsset Core on 19/07/26.
//

#ifndef HISTORYTAB_H
#define HISTORYTAB_H

#include "AssetIconProvider.h"
#include "DigiByteCore.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
#include <cstdint>
#include <map>
#include <string>

/**
 * Shows the wallet's recent transactions(newest first) with a text filter and Older/Newer
 * paging.  The transaction list comes from the wallet's listtransactions call(forwarded by the
 * daemon); each transaction is also decoded with getrawtransaction so DigiAsset movements are
 * labelled and shown(asset name, amount and icon) instead of just their tiny DGB carrier amount.
 */
class HistoryTab : public QWidget {
    Q_OBJECT

public:
    explicit HistoryTab(QWidget *parent = nullptr);

private slots:
    void updateHistory();
    void applyFilter();
    void olderPage();
    void newerPage();
    void applyIcon(quint64 assetIndex);

private:
    ///the asset moved to a particular output of a transaction
    struct AssetMove {
        uint64_t assetIndex = 0;
        uint64_t count = 0;   //amount in the smallest unit
        unsigned int decimals = 0;
    };
    ///decodes a transaction and returns, per receiving address, the asset moved to it.  Only
    ///cached once the transaction has a confirmation - an unconfirmed transaction has no
    ///blockhash yet, so the daemon can't decode its assets and looks identical to a plain
    ///transaction; caching that would permanently mislabel it even after it confirms
    std::map<std::string, AssetMove> txAssets(const std::string &txid, int confirmations);

    QLabel * _statusLabel;
    QLabel * _pageLabel;
    QLineEdit * _filterEdit;
    QPushButton * _refreshButton;
    QPushButton * _olderButton;
    QPushButton * _newerButton;
    QTableWidget * _txTable;
    QTimer * _timer;
    AssetIconProvider * _icons;
    std::map<std::string, std::map<std::string, AssetMove>> _txAssetCache; //txid -> address -> asset
    unsigned int _page = 0;      //0 = most recent transactions
    unsigned int _lastCount = 0; //rows the last update fetched(< PAGE_SIZE means last page)
    DigiByteCore _dgbCore;

    static const unsigned int PAGE_SIZE = 100;

    //table columns
    enum Column { COL_TIME = 0, COL_TYPE, COL_DGB, COL_ASSET, COL_CONF, COL_ADDRESS, COL_TXID, COL_COUNT };
};

#endif // HISTORYTAB_H
