//
// Created by DigiAsset Core on 19/07/26.
//

#ifndef MANAGEASSETTAB_H
#define MANAGEASSETTAB_H

#include "AssetIconProvider.h"
#include "DigiByteCore.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

/**
 * Burn assets or reissue more of an unlocked asset.  Uses the burnasset and
 * reissueasset RPC methods(both confirmed with a dryrun priced dialog first).
 */
class ManageAssetTab : public QWidget {
    Q_OBJECT

public:
    explicit ManageAssetTab(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshAssets();
    void burnAsset();
    void reissueAsset();

private:
    QComboBox * _assetCombo;
    QPushButton * _refreshButton;
    QLineEdit * _burnAmountEdit;
    QPushButton * _burnButton;
    QLineEdit * _reissueAmountEdit;
    QLineEdit * _reissueToEdit;
    QPushButton * _reissueButton;
    QLabel * _statusLabel;
    AssetIconProvider * _icons;
    DigiByteCore _dgbCore;
};

#endif // MANAGEASSETTAB_H
