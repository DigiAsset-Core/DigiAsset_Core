//
// Created by DigiAsset Core on 14/07/26.
//

#ifndef CREATEASSETTAB_H
#define CREATEASSETTAB_H

#include "DigiByteCore.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <vector>

/**
 * Form for creating(issuing) a new DigiAsset.  Uses the issueasset RPC method.
 */
class CreateAssetTab : public QWidget {
    Q_OBJECT

public:
    explicit CreateAssetTab(QWidget *parent = nullptr);

private slots:
    void createAsset();
    void chooseImage(QLineEdit* pathEdit, const QString& title);

private:
    ///uploads a local file to the IPFS node and returns its ipfs:// url + mime type.  Returns
    ///false and fills errorOut on failure.  Blocks until the upload finishes.
    bool uploadImage(const QString& filePath, QString& ipfsUrlOut, QString& mimeTypeOut, QString& errorOut);

    QLineEdit * _nameEdit;
    QLineEdit * _amountEdit;
    QSpinBox * _decimalsSpin;
    QCheckBox * _lockedCheck;
    QComboBox * _aggregationCombo;
    QTextEdit * _descriptionEdit;
    QLineEdit * _iconPathEdit;                  //local path of the icon image(shown in wallet lists)
    QLineEdit * _imagePathEdit;                 //local path of the larger cover image
    QCheckBox * _royaltyCheck;
    QLineEdit * _royaltyAddressEdit;
    QLineEdit * _royaltyAmountEdit;
    QLineEdit * _toAddressEdit;
    QPushButton * _createButton;
    QLabel * _statusLabel;
    std::vector<QCheckBox*> _pspChecks;         //one per storage pool, same order as _pspIndexes
    std::vector<unsigned int> _pspIndexes;      //pool indexes for the psp param of issueasset
    QNetworkAccessManager * _net;               //used to upload icon/image bytes to the IPFS node
    QString _ipfsApi;                           //e.g. http://localhost:5001/api/v0/
    unsigned int _defaultTimeoutMs = 50000;     //rpctimeout from config.cfg, restored after issueasset
    DigiByteCore _dgbCore;
};

#endif // CREATEASSETTAB_H
