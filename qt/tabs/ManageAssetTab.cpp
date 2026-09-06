#include "ManageAssetTab.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QShowEvent>
#include <QVBoxLayout>

ManageAssetTab::ManageAssetTab(QWidget *parent) : QWidget(parent), _dgbCore() {
    _dgbCore.setFileName("config.cfg", true);
    _dgbCore.makeConnection();
    _icons = new AssetIconProvider(this);
    connect(_icons, &AssetIconProvider::iconReady, this, [this](quint64 assetIndex) {
        AssetIconProvider::applyToCombo(_assetCombo, assetIndex, _icons->icon(assetIndex));
    });

    QVBoxLayout *layout = new QVBoxLayout(this);

    //shared asset picker
    QHBoxLayout *assetRow = new QHBoxLayout();
    assetRow->addWidget(new QLabel("Asset:"));
    _assetCombo = new QComboBox();
    _assetCombo->setIconSize(QSize(_icons->iconSize(), _icons->iconSize()));
    assetRow->addWidget(_assetCombo, 1);
    _refreshButton = new QPushButton("Refresh");
    connect(_refreshButton, &QPushButton::clicked, this, &ManageAssetTab::refreshAssets);
    assetRow->addWidget(_refreshButton);
    layout->addLayout(assetRow);

    //burn
    QGroupBox *burnGroup = new QGroupBox("Burn(permanently destroy - lowers total supply)");
    QFormLayout *burnForm = new QFormLayout(burnGroup);
    _burnAmountEdit = new QLineEdit();
    _burnAmountEdit->setPlaceholderText("Amount in display units(e.g. 1.5)");
    burnForm->addRow("Amount:", _burnAmountEdit);
    _burnButton = new QPushButton("Burn Asset");
    connect(_burnButton, &QPushButton::clicked, this, &ManageAssetTab::burnAsset);
    burnForm->addRow(_burnButton);
    layout->addWidget(burnGroup);

    //reissue
    QGroupBox *reissueGroup = new QGroupBox("Reissue(create more of an UNLOCKED asset you issued)");
    QFormLayout *reissueForm = new QFormLayout(reissueGroup);
    _reissueAmountEdit = new QLineEdit();
    _reissueAmountEdit->setPlaceholderText("Additional amount to create");
    reissueForm->addRow("Amount:", _reissueAmountEdit);
    _reissueToEdit = new QLineEdit();
    _reissueToEdit->setPlaceholderText("Optional - defaults to a new wallet address");
    reissueForm->addRow("Send To:", _reissueToEdit);
    _reissueButton = new QPushButton("Reissue Asset");
    connect(_reissueButton, &QPushButton::clicked, this, &ManageAssetTab::reissueAsset);
    reissueForm->addRow(_reissueButton);
    layout->addWidget(reissueGroup);

    _statusLabel = new QLabel("");
    _statusLabel->setWordWrap(true);
    _statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(_statusLabel);
    layout->addStretch();

    refreshAssets();
}

///refreshes the dropdown whenever the tab is shown so it recovers from an earlier error
///(e.g. the wallet was not loaded yet) without needing a manual Refresh click
void ManageAssetTab::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (_assetCombo->count() == 0) refreshAssets();
}

///reloads the asset dropdown from the wallet's holdings
void ManageAssetTab::refreshAssets() {
    try {
        Json::Value args = Json::arrayValue;
        Json::Value result = _dgbCore.sendcommand("getwalletbalances", args);
        _statusLabel->clear(); //a successful fetch clears any earlier error message
        _assetCombo->clear();
        for (const auto &asset: result["assets"]) {
            uint64_t assetIndex = asset["assetIndex"].asUInt64();
            QString name = _icons->name(assetIndex);
            QString label = (name.isEmpty() ? QString() : name + " - ") +
                            QString::fromStdString(asset["assetId"].asString()) +
                            " (index " + QString::number(assetIndex) +
                            ", balance " + QString::fromStdString(asset["amount"].asString()) + ")";
            _assetCombo->addItem(label, QVariant::fromValue((qulonglong) assetIndex));
            QIcon icon = _icons->icon(assetIndex);
            if (!icon.isNull()) _assetCombo->setItemIcon(_assetCombo->count() - 1, icon);
        }
        if (_assetCombo->count() == 0) {
            _statusLabel->setText("No assets in wallet.");
        }
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Error loading wallet assets: " + QString::fromStdString(e.getMessage()));
    }
}

void ManageAssetTab::burnAsset() {
    QString amount = _burnAmountEdit->text().trimmed();
    if (amount.isEmpty() || (_assetCombo->currentIndex() < 0)) {
        _statusLabel->setText("Pick an asset and fill in the burn amount.");
        return;
    }
    Json::UInt64 assetIndex = (Json::UInt64) _assetCombo->currentData().toULongLong();

    //price it first so the confirmation shows real numbers
    QString costText = "Cost could not be estimated.";
    try {
        Json::Value options = Json::objectValue;
        options["dryrun"] = true;
        Json::Value dryArgs = Json::arrayValue;
        dryArgs.append(assetIndex);
        dryArgs.append(amount.toStdString());
        dryArgs.append(options);
        Json::Value costs = _dgbCore.sendcommand("burnasset", dryArgs);
        costText = "Estimated cost: " + QString::fromStdString(costs["estimatedTotal"].asString()) + " DGB";
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Burn failed: " + QString::fromStdString(e.getMessage()));
        return;
    }

    if (QMessageBox::question(this, "Confirm Burn",
                              QString("PERMANENTLY DESTROY %1 of\n%2?\n%3\nThis can not be undone.")
                                      .arg(amount, _assetCombo->currentText(), costText)) != QMessageBox::Yes) {
        return;
    }

    try {
        Json::Value args = Json::arrayValue;
        args.append(assetIndex);
        args.append(amount.toStdString());
        Json::Value txid = _dgbCore.sendcommand("burnasset", args);
        _statusLabel->setText("Burned!  txid: " + QString::fromStdString(txid.asString()));
        _burnAmountEdit->clear();
        refreshAssets();
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Burn failed: " + QString::fromStdString(e.getMessage()));
    }
}

void ManageAssetTab::reissueAsset() {
    QString amount = _reissueAmountEdit->text().trimmed();
    if (amount.isEmpty() || (_assetCombo->currentIndex() < 0)) {
        _statusLabel->setText("Pick an asset and fill in the reissue amount.");
        return;
    }
    Json::UInt64 assetIndex = (Json::UInt64) _assetCombo->currentData().toULongLong();
    QString toAddress = _reissueToEdit->text().trimmed();

    //price it first - this also surfaces "asset is locked" and "issuer address unfunded"
    //errors before the confirmation dialog
    QString costText;
    try {
        Json::Value options = Json::objectValue;
        options["dryrun"] = true;
        if (!toAddress.isEmpty()) options["toAddress"] = toAddress.toStdString();
        Json::Value dryArgs = Json::arrayValue;
        dryArgs.append(assetIndex);
        dryArgs.append(amount.toStdString());
        dryArgs.append(options);
        Json::Value costs = _dgbCore.sendcommand("reissueasset", dryArgs);
        costText = "Estimated cost: " + QString::fromStdString(costs["estimatedTotal"].asString()) + " DGB";
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Reissue failed: " + QString::fromStdString(e.getMessage()));
        return;
    }

    if (QMessageBox::question(this, "Confirm Reissue",
                              QString("Create %1 more of\n%2?\n%3\nThis writes to the blockchain and can not be undone.")
                                      .arg(amount, _assetCombo->currentText(), costText)) != QMessageBox::Yes) {
        return;
    }

    try {
        Json::Value options = Json::objectValue;
        if (!toAddress.isEmpty()) options["toAddress"] = toAddress.toStdString();
        Json::Value args = Json::arrayValue;
        args.append(assetIndex);
        args.append(amount.toStdString());
        if (!toAddress.isEmpty()) args.append(options);
        Json::Value result = _dgbCore.sendcommand("reissueasset", args);
        _statusLabel->setText("Reissued!  txid: " + QString::fromStdString(result["txid"].asString()));
        _reissueAmountEdit->clear();
        refreshAssets();
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Reissue failed: " + QString::fromStdString(e.getMessage()));
    }
}
