#include <QLoggingCategory>
Q_LOGGING_CATEGORY(audioStreamRecorderLog, "YuRadio.AudioStreamRecorder")

#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif /* Q_OS_ANDROID */

#include <QFileInfo>

#include "audiostreamrecorder.h"

using namespace Qt::StringLiterals;

AudioStreamRecorder::AudioStreamRecorder(QObject *parent)
    : QObject(parent), m_recordingPolicy(NoRecordingPolicy),
      m_recordingNameFormat(StationDateTime), m_recording(false) {
  QString musicLocation =
    QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
    u"/YuRadio"_s;

  /* Ensure music location path exists */
  QDir().mkpath(musicLocation);
  m_outputLocation = QUrl::fromLocalFile(musicLocation);
  qCDebug(audioStreamRecorderLog)
    << "Default music location:" << m_outputLocation;
}

AudioStreamRecorder::~AudioStreamRecorder() {
  stop();
}

QUrl AudioStreamRecorder::outputLocation() const {
  return m_outputLocation;
}

void AudioStreamRecorder::setOutputLocation(const QUrl &outputLocation) {
  if (m_outputLocation != outputLocation) {
    m_outputLocation = outputLocation;
    emit outputLocationChanged();
  }
}

bool AudioStreamRecorder::recording() const {
  return m_recording;
}

void AudioStreamRecorder::record() {
  qCDebug(audioStreamRecorderLog) << "Record";

#ifdef Q_OS_ANDROID
  static std::once_flag checkPermissionFlag;
  std::call_once(checkPermissionFlag, []() {
    QList<QString> permissions = {
      u"android.permission.WRITE_EXTERNAL_STORAGE"_s,
      u"android.permission.READ_EXTERNAL_STORAGE"_s,
    };

    for (const auto &permission : permissions) {
      if (QtAndroidPrivate::checkPermission(permission).result() !=
          QtAndroidPrivate::Authorized) {
        auto result = QtAndroidPrivate::requestPermission(permission).result();
        if (result != QtAndroidPrivate::Authorized) {
          qCWarning(audioStreamRecorderLog)
            << "Permission is not granted:" << permission;
          /* NOTE: I'm not calling return here intentionally. 
           * These permissions might be not required by some devices
           */
        }
      }
    }
  });
#endif /* Q_OS_ANDROID */

  reset();
  setRecording(true);
}

void AudioStreamRecorder::stop() {
  qCDebug(audioStreamRecorderLog) << "Stop";

  if (canSaveRecording()) {
    saveRecording();
  }

  reset();
  setRecording(false);
}

void AudioStreamRecorder::setRecording(bool recording) {
  if (recording != m_recording) {
    m_recording = recording;
    emit recordingChanged();
  }
}

AudioStreamRecorder::RecordingPolicy
AudioStreamRecorder::recordingPolicy() const {
  return m_recordingPolicy;
}

void AudioStreamRecorder::setRecordingPolicy(RecordingPolicy policy) {
  if (m_recordingPolicy != policy) {
    m_recordingPolicy = policy;
    emit recordingPolicyChanged();
  }
}

void AudioStreamRecorder::processBuffer(const QByteArray &buffer,
                                        const QUrl &streamUrl,
                                        const QString &id) {
  if (!m_recording) {
    return;
  }

  if (m_streamUrl.isValid() && m_streamUrl != streamUrl) {
    reset();
  }

  if (m_recordingPolicy == SaveRecordingWhenStreamTitleChanges &&
      !id.isEmpty() && !m_streamTitle.isEmpty() && m_streamUrl.isValid() &&
      m_streamTitle != id && m_streamUrl == streamUrl && canSaveRecording()) {
    saveRecording();
  }

  if (!m_file) {
    m_file = std::make_unique<QTemporaryFile>();

    if (!m_file->open()) {
      QString errorString = QString("Failed to open file for recording: %1 %2")
                              .arg(m_file->fileName(), m_file->errorString());
      setError(errorString);
      stop();

      qCWarning(audioStreamRecorderLog).noquote() << errorString;
      return;
    }
  }

  m_streamTitle = id;
  m_streamUrl = streamUrl;

  m_file->write(buffer);
}

void AudioStreamRecorder::setPreferredSuffix(const QString &preferredSuffix) {
  qCDebug(audioStreamRecorderLog) << "Set preferred suffix:" << preferredSuffix;

  static const QStringList mpegFormats = {
    u"mpga"_s,
    u"mpega"_s,
    u"mp1"_s,
    u"mp2"_s,
  };

  if (mpegFormats.contains(preferredSuffix)) {
    m_preferredSuffix = u"mp3"_s;
  } else {
    m_preferredSuffix = preferredSuffix;
  }
}

static const QString &removeSpecialCharacters(QString &path) {
  static QRegularExpression re(u"[/\\?%*:|\"<>\\s]+"_s);

  return path.replace(re, "_");
}

static QString
formatRecordingName(AudioStreamRecorder::RecordingNameFormat format,
                    const QString &stationName, const QString &streamTitle,
                    const QDateTime &date) {
  QString dateTimePath = date.toString(u"yyyy-MM-dd-hhmmss"_s);

  switch (format) {
    case AudioStreamRecorder::StationDateTime:
      return stationName + '_' + dateTimePath;
    case AudioStreamRecorder::StationTrackNameDateTime:
      return stationName + '_' + streamTitle + '_' + dateTimePath;
    case AudioStreamRecorder::TrackNameDateTime:
      return streamTitle + '_' + dateTimePath;
  }
}

QString AudioStreamRecorder::recordingName() const {
  QString streamTitle = m_streamTitle;
  if (streamTitle.isEmpty() || m_recordingPolicy == NoRecordingPolicy) {
    streamTitle = u"UnknownTrack"_s;
  } else {
    removeSpecialCharacters(streamTitle);
  }

  QString stationName = m_stationName.isEmpty() ? u"YuRadio"_s : m_stationName;
  if (m_stationName.isEmpty()) {
    qCWarning(audioStreamRecorderLog) << "Station name is empty string";
  }

  return formatRecordingName(m_recordingNameFormat, stationName, streamTitle,
                             m_startTime)
    .trimmed()
    .removeIf([](QChar c) {
    return c != QLatin1Char('_') && c != QLatin1Char('-') &&
           (!c.isLetterOrNumber() || !c.isPrint());
  }).mid(0, 240);
}

void AudioStreamRecorder::performCopy(std::unique_ptr<QTemporaryFile> file,
                                      const QString &destination) {
  QThreadPool::globalInstance()->start(
    [this, file = std::move(file), destination]() {
    QFile saveFile(destination);

    if (!saveFile.open(QIODeviceBase::ReadWrite)) {
      QString errorString = QString("Failed to open destination file: %1")
                              .arg(saveFile.errorString());

      QMetaObject::invokeMethod(
        this, [this, errorString]() { setError(errorString); });

      qCWarning(audioStreamRecorderLog).noquote() << errorString;
    }

    if (file->open()) {
      file->seek(0);

      while (!file->atEnd() && saveFile.write(file->read(16384)) != -1) {
        ;
      }
    } else {
      QString errorString =
        QString("Failed to open temporary file: %1").arg(file->errorString());

      QMetaObject::invokeMethod(
        this, [this, errorString]() { setError(errorString); });

      qCWarning(audioStreamRecorderLog).noquote() << errorString;
    }
  });
}

void AudioStreamRecorder::saveRecording() {
  qCDebug(audioStreamRecorderLog) << "Save Recording";

  if (canSaveRecording()) {
    QString suffix = m_preferredSuffix;
    if (suffix.isEmpty()) {
      qCWarning(audioStreamRecorderLog())
        << "Suffix is empty using mp3 as fallback";
      suffix = u"mp3"_s;
    }

    QString finalName = recordingName() + '.' + suffix;

    QString recordingPath = outputLocationToDir().filePath(finalName);

    qCDebug(audioStreamRecorderLog)
      << QString("Rename %1 to %2").arg(m_file->fileName(), recordingPath);

    if (m_file->rename(recordingPath)) {
      /* Successfully renamed file. No need to remove it */
      m_file->setAutoRemove(false);
    } else if (!m_file->copy(recordingPath)) {
      /* NOTE: QTemporaryFile needs to be copied when destination resides in
       * different partition
       */
      QString errorString =
        QString("Failed to copy recording file from %1 to %2. Reason: %3")
          .arg(m_file->fileName(), recordingPath, m_file->errorString());

      qCWarning(audioStreamRecorderLog).noquote() << errorString;

      /* NOTE: Final try. Especially useful for Android */
      performCopy(std::move(m_file), recordingPath);
    }
  } else {
    QString errorString = "Failed to save recording. File is closed";
    setError(errorString);

    qCWarning(audioStreamRecorderLog).noquote() << errorString;
  }

  m_file.reset(nullptr);
  m_startTime = QDateTime::currentDateTime();
}

void AudioStreamRecorder::reset() {
  m_streamTitle = QString();
  m_streamUrl = QUrl();
  m_startTime = QDateTime::currentDateTime();
  m_file.reset(nullptr);
}

AudioStreamRecorder::RecordingNameFormat
AudioStreamRecorder::recordingNameFormat() const {
  return m_recordingNameFormat;
}

void AudioStreamRecorder::setRecordingNameFormat(RecordingNameFormat format) {
  if (m_recordingNameFormat != format) {
    m_recordingNameFormat = format;
    emit recordingNameFormatChanged();
  }
}

void AudioStreamRecorder::setError(const QString &errorString) {
  m_errorString = errorString;
  emit errorOccurred();
}

QString AudioStreamRecorder::errorString() const {
  return m_errorString;
}

QDir AudioStreamRecorder::outputLocationToDir() const {
  QString filePath = m_outputLocation.isLocalFile()
                       ? m_outputLocation.toLocalFile()
                       : m_outputLocation.toString();

  return {filePath};
}

QString AudioStreamRecorder::stationName() const {
  return m_stationName;
}

void AudioStreamRecorder::setStationName(const QString &stationName) {
  if (m_stationName != stationName) {
    m_stationName = stationName;
    removeSpecialCharacters(m_stationName);

    emit stationNameChanged();
  }
}

bool AudioStreamRecorder::canSaveRecording() const {
  return m_recording && m_file && m_file->isOpen();
}
