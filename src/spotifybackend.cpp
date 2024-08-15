#include <QLoggingCategory>
Q_LOGGING_CATEGORY(spotifyBackendLog, "YuRadio.SpotifyBackend");

#include <QDesktopServices>
#include <QOAuthHttpServerReplyHandler>

#include <QNetworkReply>
#include <QSettings>
#include <QUrlQuery>

#include "applicationconfig.h"
#include "network/json.h"
#include "network/networkmanager.h"
#include "obf.h"
#include "spotifybackend.h"

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

SpotifyBackend::SpotifyBackend(QObject *parent)
    : MusicInfoProviderBackend(parent) {
  auto *replyHandler = new QOAuthHttpServerReplyHandler(1337, this);
  replyHandler->setCallbackPath(u"/callback"_s);

  m_oauth2.setReplyHandler(replyHandler);
  m_oauth2.setAuthorizationUrl(
    QUrl(u"https://accounts.spotify.com/authorize"_s));
  m_oauth2.setAccessTokenUrl(QUrl(u"https://accounts.spotify.com/api/token"_s));

  std::string clientId = SPOTIFY_CLIENTID;
  deobfuscate_str(clientId.data(), SPOTIFY_CLIENTID_HASH);

  std::string secret = SPOTIFY_SECRET;
  deobfuscate_str(secret.data(), SPOTIFY_SECRET_HASH);

  m_oauth2.setClientIdentifier(QString::fromStdString(clientId));
  m_oauth2.setClientIdentifierSharedKey(QString::fromStdString(secret));
  m_oauth2.setUserAgent(NetworkManager::applicationUserAgent());

  m_refreshTokenTimer.setSingleShot(true);

  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, this,
          [](const QString &refreshToken) {
    QSettings settings;
    settings.setValue("SpotifyRefreshToken"_L1, refreshToken);
  });
  connect(
    &m_oauth2, &QOAuth2AuthorizationCodeFlow::error, this,
    [](const QString &error, const QString &errorDescription, const QUrl &uri) {
    qCDebug(spotifyBackendLog) << "Error:" << error << errorDescription << uri;
  });
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::expirationAtChanged, this,
          [this](const QDateTime &expiration) {
    qCDebug(spotifyBackendLog) << "Token Expiration:" << expiration;
    QDateTime currentDate = QDateTime::currentDateTime();
    auto refreshInterval =
      std::chrono::milliseconds(currentDate.msecsTo(expiration)) - 2min;
    Q_ASSERT(refreshInterval > 0ms);

    m_refreshTokenTimer.setInterval(refreshInterval);
    m_refreshTokenTimer.start();
  });
  connect(&m_refreshTokenTimer, &QTimer::timeout, &m_oauth2,
          &QOAuth2AuthorizationCodeFlow::refreshAccessToken);
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, this,
          &SpotifyBackend::handleStatusChange);
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::error, this,
          &SpotifyBackend::authenticationError);
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::granted, this,
          &SpotifyBackend::authenticated);
  connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this,
          &QDesktopServices::openUrl);
}

void SpotifyBackend::grant() {
  QSettings settings;
  QVariant maybeRefreshToken = settings.value("SpotifyRefreshToken"_L1);

  if (maybeRefreshToken.isValid()) {
    m_oauth2.setRefreshToken(maybeRefreshToken.toString());
    m_oauth2.refreshAccessToken();
  } else {
    m_oauth2.grant();
  }
}

void SpotifyBackend::handleStatusChange(
  QOAuth2AuthorizationCodeFlow::Status status) {
  switch (status) {
    case QAbstractOAuth::Status::NotAuthenticated:
      qCDebug(spotifyBackendLog) << "NotAuthenticated";
      break;
    case QAbstractOAuth::Status::TemporaryCredentialsReceived:
      qCDebug(spotifyBackendLog) << "TemporaryCredentialsReceived";
      break;
    case QAbstractOAuth::Status::Granted:
      qCDebug(spotifyBackendLog) << "Granted";
      break;
    case QAbstractOAuth::Status::RefreshingToken:
      qCDebug(spotifyBackendLog) << "RefreshingToken";
      break;
  }
}

void SpotifyBackend::requestMusicInfo(const QString &searchString) {
  if (m_oauth2.status() != QAbstractOAuth::Status::Granted) {
    qCDebug(spotifyBackendLog) << "Access is not granted";
    emit errorOccurred();
    return;
  }

  QUrl searchUrl(u"https://api.spotify.com/v1/search"_s);

  QUrlQuery query;
  query.addQueryItem(u"type"_s, u"track"_s);
  query.addQueryItem(u"q"_s, searchString);
  searchUrl.setQuery(query);

  QNetworkReply *reply = m_oauth2.get(searchUrl);

  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { handleMusicInfoReply(reply); });
}

void SpotifyBackend::handleMusicInfoReply(QNetworkReply *reply) {
  reply->deleteLater();
  if (reply->error()) {
    emit errorOccurred();
    return;
  }

  QByteArray data = reply->readAll();
  auto document = json::byteArrayToJson(data);
  if (!document) {
    emit errorOccurred();
    return;
  }

  QJsonObject rootObject = document->object();
  QJsonObject tracksObject = rootObject["tracks"_L1].toObject();
  QJsonArray itemsArray = tracksObject["items"_L1].toArray();
  if (!itemsArray.size()) {
    emit errorOccurred();
    return;
  }

  QJsonObject itemObject = itemsArray[0].toObject();
  QJsonObject albumObject = itemObject["album"_L1].toObject();

  MusicInfoDetails info;

  for (auto artistObjectRef : itemObject["artists"_L1].toArray()) {
    QJsonObject artistObject = artistObjectRef.toObject();
    info.artistNames.append(artistObject["name"_L1].toString());
  }

  for (auto imageObjectRef : albumObject["images"_L1].toArray()) {
    QJsonObject imageObject = imageObjectRef.toObject();
    info.coverUrls.append(imageObject["url"_L1].toString());
  }

  info.songName = itemObject["name"_L1].toString();
  info.albumName = albumObject["name"_L1].toString();

  QString releaseDateString = albumObject["release_date"_L1].toString();
  info.releaseDate =
    QDateTime::fromString(releaseDateString, Qt::ISODate).date();
  info.trackUrl = itemObject["external_urls"_L1]["spotify"_L1].toString();

  qCInfo(spotifyBackendLog) << "Album Name" << info.albumName;
  qCInfo(spotifyBackendLog) << "Song Name" << info.songName;
  qCInfo(spotifyBackendLog) << "Artist Name" << info.artistNames[0];
  qCInfo(spotifyBackendLog) << "Image Url" << info.coverUrls[0];
  qCInfo(spotifyBackendLog) << "Track Url" << info.trackUrl;

  emit musicInformation(info);
}

bool SpotifyBackend::refreshAuthenticationSupported() {
  QSettings settings;
  return settings.contains("SpotifyRefreshToken"_L1);
}

QString SpotifyBackend::backendName() const {
  return u"spotify"_s;
}
