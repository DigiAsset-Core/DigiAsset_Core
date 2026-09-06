#include "CreateAssetTab.h"
#include "Config.h"
#include <QApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVBoxLayout>

CreateAssetTab::CreateAssetTab(QWidget *parent) : QWidget(parent), _dgbCore() {
    _dgbCore.setFileName("config.cfg", true);
    _dgbCore.makeConnection();
    _net = new QNetworkAccessManager(this);

    //icons/images are uploaded to the same IPFS node the daemon uses before issuing
    try {
        Config config("config.cfg");
        _ipfsApi = QString::fromStdString(config.getString("ipfspath", "http://localhost:5001/api/v0/"));
        _defaultTimeoutMs = static_cast<unsigned int>(config.getInteger("rpctimeout", 50000));
    } catch (...) {
        _ipfsApi = "http://localhost:5001/api/v0/";
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();
    //some styles(eg macOS) default to leaving fields at their size hint instead of growing them
    //with the window, which makes every field look tiny on a wide window
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::DontWrapRows);

    _nameEdit = new QLineEdit();
    _nameEdit->setPlaceholderText("My Asset");
    form->addRow("Name:", _nameEdit);

    _amountEdit = new QLineEdit();
    _amountEdit->setPlaceholderText("Number of assets to create(e.g. 1000)");
    form->addRow("Amount:", _amountEdit);

    _decimalsSpin = new QSpinBox();
    _decimalsSpin->setRange(0, 7);
    _decimalsSpin->setValue(0);
    form->addRow("Decimals:", _decimalsSpin);

    _lockedCheck = new QCheckBox("No more can ever be issued");
    _lockedCheck->setChecked(true);
    form->addRow("Locked:", _lockedCheck);

    _aggregationCombo = new QComboBox();
    _aggregationCombo->addItem("aggregable");
    _aggregationCombo->addItem("hybrid");
    _aggregationCombo->addItem("dispersed");
    form->addRow("Aggregation:", _aggregationCombo);

    //permanent storage pools.  The metadata must be stored somewhere permanent or the
    //asset won't be recognised by most of the ecosystem, so at least one is required
    QVBoxLayout *pspLayout = new QVBoxLayout();
    for (unsigned int i = 0;; i++) {
        Json::Value args = Json::arrayValue;
        args.append(i);
        Json::Value pool;
        try {
            pool = _dgbCore.sendcommand("getpsp", args);
        } catch (const DigiByteException &) {
            break; //no more pools
        }
        //pool 0 is "Local Storage"(this node only) - metadata stored only there is lost if this
        //node goes offline, so it's hidden from the default GUI to avoid accidental use.
        //Delete the next line to offer it again as an option.
        if (i == 0) continue;

        QCheckBox *check = new QCheckBox(QString::fromStdString(pool["name"].asString()));
        check->setToolTip(QString::fromStdString(pool["description"].asString()));
        check->setChecked(i == 1); //public pool on by default
        _pspChecks.push_back(check);
        _pspIndexes.push_back(i);
        pspLayout->addWidget(check);
    }
    form->addRow("Store Metadata In:", pspLayout);

    _descriptionEdit = new QTextEdit();
    _descriptionEdit->setPlaceholderText("Optional description stored in the asset's metadata");
    _descriptionEdit->setMaximumHeight(80);
    form->addRow("Description:", _descriptionEdit);

    //icon + cover image.  Standard DigiAsset metadata stores these as data.urls entries
    //named "icon"(the small thumbnail shown in wallets/lists) and "image"(a larger picture).
    //The chosen file is uploaded to the IPFS node when the asset is created.
    _iconPathEdit = new QLineEdit();
    _iconPathEdit->setReadOnly(true);
    _iconPathEdit->setPlaceholderText("Optional - small icon shown in wallet lists (PNG/JPG/GIF/SVG)");
    QPushButton *iconBrowse = new QPushButton("Browse...");
    connect(iconBrowse, &QPushButton::clicked, this, [this]() { chooseImage(_iconPathEdit, "Choose Icon"); });
    QHBoxLayout *iconRow = new QHBoxLayout();
    iconRow->addWidget(_iconPathEdit, 1);
    iconRow->addWidget(iconBrowse);
    form->addRow("Icon:", iconRow);

    _imagePathEdit = new QLineEdit();
    _imagePathEdit->setReadOnly(true);
    _imagePathEdit->setPlaceholderText("Optional - larger cover image (PNG/JPG/GIF/SVG)");
    QPushButton *imageBrowse = new QPushButton("Browse...");
    connect(imageBrowse, &QPushButton::clicked, this, [this]() { chooseImage(_imagePathEdit, "Choose Cover Image"); });
    QHBoxLayout *imageRow = new QHBoxLayout();
    imageRow->addWidget(_imagePathEdit, 1);
    imageRow->addWidget(imageBrowse);
    form->addRow("Cover Image:", imageRow);

    //royalty rule: every transfer of the asset must pay this address.  More rule types
    //exist on the RPC side(rules param of issueasset) - the GUI covers the common one
    _royaltyCheck = new QCheckBox("Every transfer must pay a royalty");
    _royaltyAddressEdit = new QLineEdit();
    _royaltyAddressEdit->setPlaceholderText("Optional - defaults to a new wallet address");
    _royaltyAddressEdit->setEnabled(false);
    _royaltyAmountEdit = new QLineEdit();
    _royaltyAmountEdit->setPlaceholderText("DGB per transfer(min 0.0001)");
    _royaltyAmountEdit->setEnabled(false);
    connect(_royaltyCheck, &QCheckBox::toggled, this, [this](bool on) {
        _royaltyAddressEdit->setEnabled(on);
        _royaltyAmountEdit->setEnabled(on);
    });
    QVBoxLayout *royaltyLayout = new QVBoxLayout();
    royaltyLayout->addWidget(_royaltyCheck);
    royaltyLayout->addWidget(_royaltyAddressEdit);
    royaltyLayout->addWidget(_royaltyAmountEdit);
    form->addRow("Royalty:", royaltyLayout);

    _toAddressEdit = new QLineEdit();
    _toAddressEdit->setPlaceholderText("Optional - defaults to a new wallet address");
    form->addRow("Send To:", _toAddressEdit);

    layout->addLayout(form);

    _createButton = new QPushButton("Create Asset");
    connect(_createButton, &QPushButton::clicked, this, &CreateAssetTab::createAsset);
    layout->addWidget(_createButton);

    _statusLabel = new QLabel("");
    _statusLabel->setWordWrap(true);
    _statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(_statusLabel);
    layout->addStretch();
}

void CreateAssetTab::createAsset() {
    QString name = _nameEdit->text().trimmed();
    QString amount = _amountEdit->text().trimmed();
    if (name.isEmpty() || amount.isEmpty()) {
        _statusLabel->setText("Please fill in name and amount.");
        return;
    }

    //at least one storage pool is required - without permanent storage the metadata can be
    //lost and the asset won't be recognised by most of the ecosystem
    Json::Value pspArray = Json::arrayValue;
    for (size_t i = 0; i < _pspChecks.size(); i++) {
        if (_pspChecks[i]->isChecked()) pspArray.append(_pspIndexes[i]);
    }
    if (pspArray.empty()) {
        _statusLabel->setText("Select at least one storage pool under \"Store Metadata In\".");
        return;
    }

    try {
        Json::Value config = Json::objectValue;
        config["name"] = name.toStdString();
        config["amount"] = amount.toStdString();
        config["decimals"] = _decimalsSpin->value();
        config["locked"] = _lockedCheck->isChecked();
        config["aggregation"] = _aggregationCombo->currentText().toStdString();
        QString description = _descriptionEdit->toPlainText().trimmed();
        if (!description.isEmpty()) config["description"] = description.toStdString();

        //upload the icon/cover image to IPFS and reference them in data.urls.  Done before the
        //dryrun so the estimated cost includes the storage-pool fee for the image bytes.
        Json::Value urls = Json::arrayValue;
        struct { QLineEdit *edit; const char *urlName; } images[] = {
                {_iconPathEdit, "icon"},
                {_imagePathEdit, "image"},
        };
        for (const auto &image: images) {
            QString path = image.edit->text().trimmed();
            if (path.isEmpty()) continue;
            _statusLabel->setText(QString("Uploading %1 to IPFS...").arg(image.urlName));
            _statusLabel->repaint();
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            QString ipfsUrl, mimeType, uploadError;
            if (!uploadImage(path, ipfsUrl, mimeType, uploadError)) {
                _statusLabel->setText(QString("Could not upload %1: %2").arg(image.urlName, uploadError));
                return;
            }
            Json::Value entry = Json::objectValue;
            entry["name"] = image.urlName;
            entry["url"] = ipfsUrl.toStdString();
            entry["mimeType"] = mimeType.toStdString();
            urls.append(entry);
        }
        if (!urls.empty()) config["urls"] = urls;

        QString toAddress = _toAddressEdit->text().trimmed();
        if (!toAddress.isEmpty()) config["toAddress"] = toAddress.toStdString();
        config["psp"] = pspArray;

        //royalty rule
        if (_royaltyCheck->isChecked()) {
            QString royaltyAddress = _royaltyAddressEdit->text().trimmed();
            bool amountOk = false;
            double royaltyDgb = _royaltyAmountEdit->text().trimmed().toDouble(&amountOk);
            if (!amountOk || (royaltyDgb < 0.0001)) {
                _statusLabel->setText("Royalty needs a DGB amount of at least 0.0001.");
                return;
            }
            //left blank - default to a new wallet address, same as the "Send To" field
            if (royaltyAddress.isEmpty()) {
                try {
                    Json::Value result = _dgbCore.sendcommand("getnewaddress", Json::arrayValue);
                    royaltyAddress = QString::fromStdString(result.asString());
                    _royaltyAddressEdit->setText(royaltyAddress);
                } catch (const DigiByteException& e) {
                    _statusLabel->setText("Could not get a wallet address for the royalty: " + QString::fromStdString(e.getMessage()));
                    return;
                }
            }
            Json::Value addresses = Json::objectValue;
            addresses[royaltyAddress.toStdString()] =
                    static_cast<Json::UInt64>(royaltyDgb * 100000000.0 + 0.5); //DGB -> sats
            Json::Value rules = Json::objectValue;
            rules["royalty"] = Json::objectValue;
            rules["royalty"]["addresses"] = addresses;
            config["rules"] = rules;
        }

        //price the issuance first so the user confirms real numbers.  Replaces any leftover
        //"Uploading ... to IPFS" text so the status line doesn't look stuck on a finished step
        _statusLabel->setText("Estimating cost...");
        _statusLabel->repaint();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QString costText = "Costs could not be estimated.";
        try {
            Json::Value dryConfig = config;
            dryConfig["dryrun"] = true;
            Json::Value dryArgs = Json::arrayValue;
            dryArgs.append(dryConfig);
            Json::Value costs = _dgbCore.sendcommand("issueasset", dryArgs);
            costText = QString("Storage pool fee: %1 DGB\nEstimated total cost: %2 DGB")
                               .arg(QString::fromStdString(costs["pspFee"].asString()),
                                    QString::fromStdString(costs["estimatedTotal"].asString()));
        } catch (const DigiByteException&) {} //fall back to generic text

        //confirm - issuance can not be undone.  Built as an instance(rather than the static
        //QMessageBox::question helper) so it can be raised/activated - on some platforms a
        //dialog opened right after a long blocking call can otherwise appear behind the
        //main window, making it look like nothing happened
        QString lockedText = _lockedCheck->isChecked() ? "locked(supply can never change)" : "unlocked";
        QMessageBox confirmBox(QMessageBox::Question, "Confirm Create Asset",
                               QString("Create %1 of \"%2\" (%3)?\n%4\nThis writes to the blockchain and can not be undone.")
                                       .arg(amount, name, lockedText, costText),
                               QMessageBox::Yes | QMessageBox::No, this);
        confirmBox.raise();
        confirmBox.activateWindow();
        if (confirmBox.exec() != QMessageBox::Yes) {
            return;
        }

        //funding can legitimately retry for tens of seconds(eg waiting for a just-broadcast
        //change output to confirm) - use a longer timeout than normal calls so the GUI doesn't
        //give up before the daemon does, and show that we're still working rather than
        //appearing to freeze.  processEvents() here matters: without it, closing the dialog
        //and this status update just sit in the queue while the blocking RPC call below
        //freezes the event loop, so the old dialog area never repaints and looks stuck
        _statusLabel->setText("Creating asset... this can take up to a couple of minutes.");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        _statusLabel->repaint();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        _dgbCore.setTimeout(std::max(_defaultTimeoutMs, 180000u));
        Json::Value args = Json::arrayValue;
        args.append(config);
        Json::Value result;
        try {
            result = _dgbCore.sendcommand("issueasset", args);
        } catch (...) {
            _dgbCore.setTimeout(_defaultTimeoutMs);
            QApplication::restoreOverrideCursor();
            throw;
        }
        _dgbCore.setTimeout(_defaultTimeoutMs);
        QApplication::restoreOverrideCursor();
        _statusLabel->setText("Asset created!\ntxid: " + QString::fromStdString(result["txid"].asString()) +
                              "\nassetId: " + QString::fromStdString(result["assetId"].asString()) +
                              "\nmetadata cid: " + QString::fromStdString(result["cid"].asString()));
    } catch (const DigiByteException &e) {
        _statusLabel->setText("Create failed: " + QString::fromStdString(e.getMessage()));
    }
}

///opens a file dialog and stores the chosen image path in pathEdit
void CreateAssetTab::chooseImage(QLineEdit *pathEdit, const QString &title) {
    QString path = QFileDialog::getOpenFileName(this, title, QString(),
                                                "Images (*.png *.jpg *.jpeg *.gif *.svg *.webp);;All files (*)");
    if (!path.isEmpty()) pathEdit->setText(path);
}

/**
 * Uploads a local image to the IPFS node and returns its ipfs:// url and mime type.
 * Uses the same add parameters the daemon uses(raw single block, CIDv1) so the resulting
 * cid matches what the storage pool expects.  Blocks on a local event loop until done.
 */
bool CreateAssetTab::uploadImage(const QString &filePath, QString &ipfsUrlOut, QString &mimeTypeOut,
                                 QString &errorOut) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorOut = "could not open file";
        return false;
    }
    QByteArray bytes = file.readAll();
    file.close();
    //the daemon's raw single-block add mode caps content at ~2MB - keep icons/images under it
    if (bytes.size() > 2096896) {
        errorOut = "file is larger than 2MB";
        return false;
    }

    QString mimeType = QMimeDatabase().mimeTypeForFile(filePath).name();
    if (mimeType.isEmpty()) mimeType = "application/octet-stream";

    //multipart form upload, matching IPFS::addFile(raw leaves, CIDv1, single block, pinned)
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"file\""));
    filePart.setBody(bytes);
    multiPart->append(filePart);

    QUrl url(_ipfsApi + "add?raw-leaves=true&cid-version=1&hash=sha2-256&chunker=size-2096896&pin=true");
    QNetworkRequest request(url);
    //kubo rejects browser-like(Mozilla) User-Agents as CSRF protection; Qt's default is one
    request.setHeader(QNetworkRequest::UserAgentHeader, "DigiAssetCore-GUI");
    QNetworkReply *reply = _net->post(request, multiPart);
    multiPart->setParent(reply); //deleted with the reply

    //block until the upload completes so createAsset() can stay synchronous
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray response = reply->readAll();
    bool ok = (reply->error() == QNetworkReply::NoError);
    QString netError = reply->errorString();
    reply->deleteLater();

    if (!ok) {
        errorOut = netError.isEmpty() ? "network error" : netError;
        return false;
    }
    QJsonObject obj = QJsonDocument::fromJson(response).object();
    QString hash = obj.value("Hash").toString();
    if (hash.isEmpty()) {
        errorOut = "unexpected response from IPFS node";
        return false;
    }
    ipfsUrlOut = "ipfs://" + hash;
    mimeTypeOut = mimeType;
    return true;
}
