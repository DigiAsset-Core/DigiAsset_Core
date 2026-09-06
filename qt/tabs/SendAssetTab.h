//
// Created by DigiAsset Core on 14/07/26.
//

#ifndef SENDASSETTAB_H
#define SENDASSETTAB_H

#include "AssetIconProvider.h"
#include "DigiByteCore.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

/**
 * Form for sending DigiAssets from the wallet.  Uses the sendasset RPC method.
 */
class SendAssetTab : public QWidget {
    Q_OBJECT

public:
    explicit SendAssetTab(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshAssets();
    void sendAsset();

private:
    QLineEdit * _addressEdit;
    QComboBox * _assetCombo;
    QLineEdit * _amountEdit;
    QPushButton * _sendButton;
    QPushButton * _refreshButton;
    QLabel * _statusLabel;
    AssetIconProvider * _icons;
    DigiByteCore _dgbCore;
};

#endif // SENDASSETTAB_H
