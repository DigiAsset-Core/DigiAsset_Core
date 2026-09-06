#include "SendAssetTab.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QShowEvent>
#include <QVBoxLayout>

SendAssetTab::SendAssetTab(QWidget *parent) : QWidget(parent), _dgbCore() {
    _dgbCore.setFileName("config.cfg", true);
    _dgbCore.makeConnection();
    _icons = new AssetIconProvider(this);
    connect(_icons, &AssetIconProvider::iconReady, this, [this](quint64 assetIndex) {
        AssetIconProvider::applyToCombo(_assetCombo, assetIndex, _icons->icon(assetIndex));
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    _addressEdit = new QLineEdit();
    _addressEdit->setPlaceholderText("dgb1... address or DigiByte domain");
    form->addRow("Pay To:", _addressEdit);

    QHBoxLayout *assetRow = new QHBoxLayout();
    _assetCombo = new QComboBox();
    _assetCombo->setIconSize(QSize(_icons->iconSize(), _icons->iconSize()));
    assetRow->addWidget(_assetCombo, 1);
    _refreshButton = new QPushButton("Refresh");
    connect(_refreshButton, &QPushButton::clicked, this, &SendAssetTab::refreshAssets);
    assetRow->addWidget(_refreshButton);
    form->addRow("Asset:", assetRow);

    _amountEdit = new QLineEdit();
    _amountEdit->setPlaceholderText("Amount in display units(e.g. 1.5)");
    form->addRow("Amount:", _amountEdit);

    layout->addLayout(form);

    _sendButton = new QPushButton("Send Asset");
    connect(_sendButton, &QPushButton::clicked, this, &SendAssetTab::sendAsset);
    layout->addWidget(_sendButton);

    _statusLabel = new QLabel("");
    _statusLabel->setWordWrap(true);
    _statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(_statusLabel);
    layout->addStretch();

    refreshAssets();
}

///refreshes the dropdown whenever the tab is shown so it recovers from an earlier error
///(e.g. the wallet was not loaded yet) without needing a manual Refresh click
void SendAssetTab::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (_assetCombo->count() == 0) refreshAssets();
}

///reloads the asset dropdown from the wallet's holdings
void SendAssetTab::refreshAssets() {
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

void SendAssetTab::sendAsset() {
    QString address = _addressEdit->text().trimmed();
    QString amount = _amountEdit->text().trimmed();
    if (address.isEmpty() || amount.isEmpty() || (_assetCombo->currentIndex() < 0)) {
        _statusLabel->setText("Please fill in address, asset and amount.");
        return;
    }

    //confirm
    QString assetLabel = _assetCombo->currentText();
    if (QMessageBox::question(this, "Confirm Send",
                              QString("Send %1 of\n%2\nto %3?").arg(amount, assetLabel, address)) != QMessageBox::Yes) {
        return;
    }

    try {
        Json::Value args = Json::arrayValue;
        args.append(address.toStdString());
        args.append((Json::UInt64) _assetCombo->currentData().toULongLong());
        args.append(amount.toStdString());
        Json::Value txid = _dgbCore.sendcommand("sendasset", args);
        _statusLabel->setText("Sent!  txid: " + QString::fromStdString(txid.asString()));
        _amountEdit->clear();
        refreshAssets();
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Send failed: " + QString::fromStdString(e.getMessage()));
    }
}
