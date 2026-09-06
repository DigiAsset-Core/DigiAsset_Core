//
// Created by DigiAsset Core on 22/07/26.
//

#ifndef ASSETICONPROVIDER_H
#define ASSETICONPROVIDER_H

#include "DigiByteCore.h"
#include <QIcon>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <cstdint>
#include <map>
#include <set>

class QComboBox;

/**
 * Shared source of DigiAsset icons for the GUI tabs.
 *
 * An asset's icon comes from its metadata(getassetdata) - the data.urls entry named "icon"
 * points to an image on IPFS, which is fetched straight from the IPFS node's HTTP API(the same
 * node the daemon uses).  Results are cached so every tab that shows the same asset shares one
 * download.  icon() returns immediately(possibly with a null icon) and iconReady() fires once a
 * download finishes so callers can refresh whatever widget shows that asset.
 */
class AssetIconProvider : public QObject {
    Q_OBJECT

public:
    explicit AssetIconProvider(QObject *parent = nullptr);

    ///cached icon for the asset(null QIcon if not downloaded yet).  Kicks off the metadata +
    ///icon fetch the first time an asset is requested.
    QIcon icon(uint64_t assetIndex);

    ///display name from the asset metadata(empty if the metadata can't be read yet).  Loads the
    ///metadata on first request so the name is available immediately to callers.
    QString name(uint64_t assetIndex);

    ///icon height in pixels(config: guiasseticonsize, default 16), clamped to a sane range
    int iconSize() const { return _iconSize; }

    ///convenience for the dropdown tabs: set icon on whichever combo item carries assetIndex
    ///as its userData(the value the Send/Manage tabs store with each item)
    static void applyToCombo(QComboBox *combo, uint64_t assetIndex, const QIcon &icon);

signals:
    ///emitted when an asset's icon finishes downloading
    void iconReady(quint64 assetIndex);

private:
    void loadAssetInfo(uint64_t assetIndex);
    void fetchIcon(uint64_t assetIndex, const QString &cid);
    QString cachePath(const QString &cid) const;

    QNetworkAccessManager *_net;
    QString _ipfsApi;   //e.g. http://localhost:5001/api/v0/
    QString _cacheDir;  //on-disk icon cache(<appdir>/cache/icons), keyed by cid
    int _iconSize;
    std::map<uint64_t, QString> _names;  //assetIndex -> asset name(from metadata)
    std::map<uint64_t, QIcon> _icons;    //assetIndex -> fetched icon
    std::set<uint64_t> _infoLoaded;      //assetIndexes whose metadata was fetched
    std::set<uint64_t> _iconInFlight;    //icon downloads in progress
    DigiByteCore _dgbCore;
};

#endif // ASSETICONPROVIDER_H
