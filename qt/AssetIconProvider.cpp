//
// Created by DigiAsset Core on 22/07/26.
//

#include "AssetIconProvider.h"
#include "Config.h"
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>

//kubo(the IPFS node) rejects requests whose User-Agent starts with "Mozilla" as CSRF
//protection, and Qt's default UA is Mozilla-like - so every IPFS request must override it
static const char *IPFS_USER_AGENT = "DigiAssetCore-GUI";

AssetIconProvider::AssetIconProvider(QObject *parent) : QObject(parent), _iconSize(16), _dgbCore() {
    _dgbCore.setFileName("config.cfg", true);
    _dgbCore.makeConnection();
    _net = new QNetworkAccessManager(this);

    //the icon bytes come straight from the IPFS node's API(same one the daemon uses)
    try {
        Config config("config.cfg");
        _ipfsApi = QString::fromStdString(config.getString("ipfspath", "http://localhost:5001/api/v0/"));
        _iconSize = config.getInteger("guiasseticonsize", 16);
    } catch (...) {
        _ipfsApi = "http://localhost:5001/api/v0/";
    }
    if (_iconSize < 8) _iconSize = 8; //keep it visible but sane
    if (_iconSize > 64) _iconSize = 64;

    //icons are cached on disk(keyed by their content-addressed cid) so they survive restarts
    //and don't get re-downloaded from IPFS every launch
    _cacheDir = QCoreApplication::applicationDirPath() + "/cache/icons";
    QDir().mkpath(_cacheDir);
}

QString AssetIconProvider::cachePath(const QString &cid) const {
    return _cacheDir + "/" + cid;
}

QIcon AssetIconProvider::icon(uint64_t assetIndex) {
    loadAssetInfo(assetIndex);
    auto it = _icons.find(assetIndex);
    return (it != _icons.end()) ? it->second : QIcon();
}

QString AssetIconProvider::name(uint64_t assetIndex) {
    loadAssetInfo(assetIndex);
    auto it = _names.find(assetIndex);
    return (it != _names.end()) ? it->second : QString();
}

/**
 * Fetches the asset's metadata once and caches its display name plus starts the icon
 * download if the metadata lists one(data.urls entry named "icon" on ipfs)
 */
void AssetIconProvider::loadAssetInfo(uint64_t assetIndex) {
    if (_infoLoaded.count(assetIndex)) return;
    _infoLoaded.insert(assetIndex);
    try {
        Json::Value args = Json::arrayValue;
        args.append(static_cast<Json::UInt64>(assetIndex));
        Json::Value data = _dgbCore.sendcommand("getassetdata", args);
        const Json::Value &meta = data["ipfs"]["data"];
        if (meta.isMember("assetName") && meta["assetName"].isString()) {
            _names[assetIndex] = QString::fromStdString(meta["assetName"].asString());
        }
        if (meta.isMember("urls") && meta["urls"].isArray()) {
            for (const auto &url: meta["urls"]) {
                if (!url.isObject() || !url.isMember("name") || !url.isMember("url")) continue;
                if (url["name"].asString() != "icon") continue;
                //some test assets point "icon" at non-image data(e.g. application/json) - those
                //can't be shown as an icon, so skip anything not declared as an image
                if (url.isMember("mimeType") && url["mimeType"].isString() &&
                    url["mimeType"].asString().rfind("image/", 0) != 0) {
                    break;
                }
                std::string target = url["url"].asString();
                if (target.rfind("ipfs://", 0) == 0) {
                    fetchIcon(assetIndex, QString::fromStdString(target.substr(7)));
                }
                break;
            }
        }
    } catch (const DigiByteException &) {
        //metadata may not have synced yet - try again on a later request
        _infoLoaded.erase(assetIndex);
    }
}

///provides the icon from the on-disk cache if present, otherwise downloads it from the IPFS
///node(api "cat" call), writes the bytes to the cache and hands back a QIcon
void AssetIconProvider::fetchIcon(uint64_t assetIndex, const QString &cid) {
    if (_iconInFlight.count(assetIndex) || _icons.count(assetIndex)) return;

    //disk cache hit: load straight from the file, no network needed
    QPixmap cached;
    if (cached.load(cachePath(cid))) {
        _icons[assetIndex] = QIcon(cached);
        emit iconReady((quint64) assetIndex);
        return;
    }

    //cache miss: download the bytes from the IPFS node
    _iconInFlight.insert(assetIndex);
    QNetworkRequest request(QUrl(_ipfsApi + "cat?arg=" + cid));
    request.setHeader(QNetworkRequest::UserAgentHeader, IPFS_USER_AGENT);
    QNetworkReply *reply = _net->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, assetIndex, cid, reply]() {
        _iconInFlight.erase(assetIndex);
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QByteArray bytes = reply->readAll();
        QPixmap pixmap;
        if (!pixmap.loadFromData(bytes)) return;
        //write to the disk cache(best effort - a failed write just means we fetch again later)
        QFile file(cachePath(cid));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(bytes);
            file.close();
        }
        _icons[assetIndex] = QIcon(pixmap);
        emit iconReady((quint64) assetIndex);
    });
}

void AssetIconProvider::applyToCombo(QComboBox *combo, uint64_t assetIndex, const QIcon &icon) {
    if ((combo == nullptr) || icon.isNull()) return;
    for (int i = 0; i < combo->count(); i++) {
        if (combo->itemData(i).toULongLong() == assetIndex) combo->setItemIcon(i, icon);
    }
}
