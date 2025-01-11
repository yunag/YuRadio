/**
 * @file
 * @brief TODO: move to separate thread
 */

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(audioStreamRecorderLog, "YuRadio.AudioStreamRecorder")

#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif /* Q_OS_ANDROID */

#include <QFileInfo>

#include "audiostreamrecorder.h"

using namespace Qt::StringLiterals;

namespace {

const QString &removeSpecialCharacters(QString &path) {
  static QRegularExpression re(u"[/\\?%*:|\"<>\\s]+"_s);

  return path.replace(re, "_");
}

QString formatRecordingName(AudioStreamRecorder::RecordingNameFormat format,
                            const QString &stationName,
                            const QString &streamTitle, const QDateTime &date) {
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

}  // namespace

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

void AudioStreamRecorder::processFrame(const ffmpeg::frame &frame,
                                       const QString &streamTitle) {
  if (!m_recording) {
    return;
  }

  if (m_muxer.input_format() != frame.audio_format() && canSaveRecording()) {
    /* Force save when frame format is not the same as in muxer */
    saveRecording();
    m_startTime = QDateTime::currentDateTime();

    /* TODO: Change resampler dynamically in muxer to avoid that behavior */
  }

  if (m_recordingPolicy == SaveRecordingWhenStreamTitleChanges &&
      !streamTitle.isEmpty() && !m_streamTitle.isEmpty() &&
      m_streamTitle != streamTitle && canSaveRecording()) {
    saveRecording();
    m_startTime = QDateTime::currentDateTime();
  }

  if (!m_muxer.opened() && m_errorString.isEmpty()) {
    m_tempFile = std::make_unique<QTemporaryFile>();
    if (m_tempFile->open() &&
        m_tempFile->rename(m_tempFile->fileName() + u".mp3"_s) &&
        m_tempFile->open()) {
      /* OK */
    } else {
      QString errorString =
        QString("Failed to open temporary file for recording: %1")
          .arg(m_tempFile->errorString());

      setError(errorString);
      qCWarning(audioStreamRecorderLog).noquote() << errorString;

      closeMuxer();
      return;
    }

    const QString tempRecordingFilename = m_tempFile->fileName();

    m_muxer.set_input_format(frame.audio_format());

    std::error_code ec =
      m_muxer.open(tempRecordingFilename.toUtf8().constData());
    if (ec) {
      QString errorString =
        QString("Failed to open file for recording: %1 %2")
          .arg(tempRecordingFilename, QString::fromStdString(ec.message()));

      setError(errorString);
      qCWarning(audioStreamRecorderLog).noquote() << errorString;

      closeMuxer();
      return;
    }

    ffmpeg::metadata_map metadata;
    if (!streamTitle.isEmpty()) {
      metadata["StreamTitle"] = streamTitle.toStdString();
    }

    ec = m_muxer.write_header(metadata);
    if (ec) {
      qCWarning(audioStreamRecorderLog)
        << "Failed to write header:" << ec.message();

      closeMuxer();
      return;
    }
  }

  m_streamTitle = streamTitle;

  std::error_code ec = m_muxer.write(frame);
  if (ec) {
    qCWarning(audioStreamRecorderLog)
      << "Failed to write frame to output file:" << ec.message();
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
    return c != '_'_L1 && c != '-'_L1 &&
           (!c.isLetterOrNumber() || !c.isPrint());
  }).mid(0, 240);
}

void AudioStreamRecorder::performCopy(std::unique_ptr<QTemporaryFile> file,
                                      const QString &destination) {
  QThreadPool::globalInstance()->start(
    [this, readFile = std::move(file), destination]() {
    QFile saveFile(destination);

    if (!saveFile.open(QIODeviceBase::ReadWrite)) {
      QString errorString = QString("Failed to open destination file: %1")
                              .arg(saveFile.errorString());

      QMetaObject::invokeMethod(this, [this, errorString]() {
        setError(errorString);
      });

      qCWarning(audioStreamRecorderLog).noquote() << errorString;
      return;
    }

    if (readFile->open()) {
      readFile->seek(0);

      while (!readFile->atEnd() &&
             saveFile.write(readFile->read(16384)) != -1) {
        std::this_thread::yield();
      }
    } else {
      QString errorString = QString("Failed to open temporary file: %1")
                              .arg(readFile->errorString());

      QMetaObject::invokeMethod(this, [this, err = std::move(errorString)]() {
        setError(err);
      });

      qCWarning(audioStreamRecorderLog).noquote() << errorString;
    }
  });
}

void AudioStreamRecorder::saveRecording() {
  qCDebug(audioStreamRecorderLog) << "Save Recording";

  if (canSaveRecording()) {
    const QString suffix = u"mp3"_s;
    const QString finalName = recordingName() + '.' + suffix;

    std::error_code ec = m_muxer.write_trailer();
    if (ec) {
      qCWarning(audioStreamRecorderLog)
        << "Failed to write trailer:" << ec.message();
      closeMuxer();
      return;
    }

    m_muxer.close();

    QString recordingPath = outputLocationToDir().filePath(finalName);
    qCDebug(audioStreamRecorderLog)
      << QString("Rename %1 to %2").arg(m_tempFile->fileName(), recordingPath);

    if (m_tempFile->rename(recordingPath)) {
      /* Successfully renamed file. No remove needed */
      m_tempFile->setAutoRemove(false);
      return;
    }

    if (!m_tempFile->copy(recordingPath)) {
      QString errorString =
        QString("Failed to copy recording file from %1 to %2.")
          .arg(m_tempFile->fileName(), recordingPath);

      qCWarning(audioStreamRecorderLog).noquote() << errorString;

      /* NOTE: Final try. Especially useful for Android */
      performCopy(std::move(m_tempFile), recordingPath);
    }

  } else {
    QString errorString = "Failed to save recording. File is closed";
    setError(errorString);

    qCWarning(audioStreamRecorderLog).noquote() << errorString;
  }
}

void AudioStreamRecorder::reset() {
  m_streamTitle = {};
  m_errorString = {};
  m_startTime = QDateTime::currentDateTime();
  closeMuxer();
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
  return m_recording && m_muxer.opened() && m_tempFile && m_tempFile->isOpen();
}

void AudioStreamRecorder::closeMuxer() {
  m_muxer.close();
  m_tempFile = {};
}
