#include "androidmediaplayerservice.h"
#include "androidmediasessionimageprovider.h"

#include <QtCore/private/qandroidextras_p.h>

static const char radioPlayerClassName[] = "org/yuradio/RadioPlayer";

static QPointer<AndroidMediaPlayerService> attachedMediaPlayerService = nullptr;
static QPointer<RadioPlayer> attachedRadioPlayer = nullptr;

AndroidMediaPlayerService::AndroidMediaPlayerService(RadioPlayer *player)
    : MediaPlayerService(player),
      imageProvider(new AndroidMediaSessionImageProvider(this)) {
  static std::once_flag registerNativeMethodsFlag;
  std::call_once(registerNativeMethodsFlag, []() {
    registerNativeMethods();
  });

  QJniObject context(QNativeInterface::QAndroidApplication::context());
  context.callMethod<void>("registerMediaPlayerService");

  attachedMediaPlayerService = this;
  attachedRadioPlayer = player;
}

AndroidMediaPlayerService::~AndroidMediaPlayerService() {
  QJniObject context(QNativeInterface::QAndroidApplication::context());

  attachedRadioPlayer = nullptr;
  attachedMediaPlayerService = nullptr;

  context.callMethod<void>("unregisterMediaPlayerService");
};

static void setVolumeNative(JNIEnv * /*env*/, jobject /*thiz*/, jfloat volume) {
  if (attachedRadioPlayer) {
    QMetaObject::invokeMethod(attachedRadioPlayer, [volume]() {
      attachedRadioPlayer->setVolume(static_cast<double>(volume));
    });
  }
}

Q_DECLARE_JNI_NATIVE_METHOD(setVolumeNative);

static void playNative(JNIEnv * /*env*/, jobject /*thiz*/) {
  if (attachedRadioPlayer) {
    QMetaObject::invokeMethod(attachedRadioPlayer, &RadioPlayer::play);
  }
}

Q_DECLARE_JNI_NATIVE_METHOD(playNative);

static void pauseNative(JNIEnv * /*env*/, jobject /*thiz*/) {
  if (attachedRadioPlayer) {
    QMetaObject::invokeMethod(attachedRadioPlayer, &RadioPlayer::pause);
  }
}

Q_DECLARE_JNI_NATIVE_METHOD(pauseNative);

static void stopNative(JNIEnv * /*env*/, jobject /*thiz*/) {
  if (attachedRadioPlayer) {
    QMetaObject::invokeMethod(attachedRadioPlayer, &RadioPlayer::stop);
  }
}

Q_DECLARE_JNI_NATIVE_METHOD(stopNative);

static void radioPlayerHandlerNative(JNIEnv * /*env*/, jobject /*thiz*/,
                                     jobject radioPlayer) {
  if (!attachedMediaPlayerService || !attachedRadioPlayer) {
    return;
  }

  attachedMediaPlayerService->radioPlayer = radioPlayer;
  attachedRadioPlayer->disconnect(attachedMediaPlayerService);

  QObject::connect(attachedRadioPlayer, &RadioPlayer::streamTitleChanged,
                   attachedMediaPlayerService, []() {
    QString newStreamTitle = attachedRadioPlayer->streamTitle();

    QJniObject authorString = QJniObject::fromString(newStreamTitle);
    attachedMediaPlayerService->radioPlayer.callMethod<void>(
      "setAuthorMetadata", authorString.object<jstring>());
  });

  QObject::connect(attachedRadioPlayer, &RadioPlayer::mediaItemChanged,
                   attachedMediaPlayerService, []() {
    QUrl newSource = attachedRadioPlayer->mediaItem().source;
    QUrl newArtworkUri = attachedRadioPlayer->mediaItem().artworkUri;
    QString newTitle = attachedRadioPlayer->mediaItem().author;

    attachedMediaPlayerService->imageProvider->setImageSource(newArtworkUri);
    QUrl proxyArtworkUri =
      attachedMediaPlayerService->imageProvider->imageUrl();

    QJniObject artworkUriString =
      QJniObject::fromString(proxyArtworkUri.toString());
    QJniObject titleString = QJniObject::fromString(newTitle);
    QJniObject sourceString = QJniObject::fromString(newSource.toString());

    attachedMediaPlayerService->radioPlayer.callMethod<void>(
      "setMediaItemData", sourceString.object<jstring>(),
      titleString.object<jstring>(), artworkUriString.object<jstring>());
  });

  QObject::connect(attachedRadioPlayer, &RadioPlayer::mediaStatusChanged,
                   attachedMediaPlayerService, []() {
    attachedMediaPlayerService->radioPlayer.callMethod<void>(
      "mediaStateChanged", attachedRadioPlayer->mediaStatus());
  });

  QObject::connect(attachedRadioPlayer, &RadioPlayer::playbackStateChanged,
                   attachedMediaPlayerService, []() {
    attachedMediaPlayerService->radioPlayer.callMethod<void>(
      "playbackStateChanged", attachedRadioPlayer->playbackState());
  });
}

void AndroidMediaPlayerService::registerNativeMethods() {
  std::initializer_list<JNINativeMethod> nativeRadioPlayerMethods = {
    Q_JNI_NATIVE_METHOD(setVolumeNative),
    Q_JNI_NATIVE_METHOD(playNative),
    Q_JNI_NATIVE_METHOD(pauseNative),
    Q_JNI_NATIVE_METHOD(stopNative),
    {"radioPlayerHandlerNative", "(Lorg/yuradio/RadioPlayer;)V",
     reinterpret_cast<void *>(radioPlayerHandlerNative)}};

  QJniEnvironment env;
  env.registerNativeMethods(radioPlayerClassName, nativeRadioPlayerMethods);
}
