/**
 * @file
 * @brief TODO: move to separate thread
 */

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(audioStreamRecorderLog, "YuRadio.AudioStreamRecorder")

#include "audiostreamrecorder.h"

#include "ffmpeg/muxer.h"

#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif /* Q_OS_ANDROID */

#include <QFileInfo>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

QDir outputLocationToDir(const QUrl &outputLocation) {
  QString filePath = outputLocation.isLocalFile() ? outputLocation.toLocalFile()
                                                  : outputLocation.toString();

  return {filePath};
}

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
    case AudioStreamRecorder::DateTime:
      return dateTimePath;
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

  qCDebug(audioStreamRecorderLog) << "Default music location:" << musicLocation;

  m_recorderWorker = std::make_unique<AudioRecorderSinkWorker>();
  connect(m_recorderWorker.get(), &AudioRecorderSinkWorker::errorOccurred, this,
          [this](const QString &errorString) {
    stop();
    setError(errorString);
  });

  m_recorderWorker->moveToThread(&m_recorderThread);
  m_recorderThread.start();

  setOutputLocation(QUrl::fromLocalFile(musicLocation));
}

AudioStreamRecorder::~AudioStreamRecorder() {
  stop();

  m_recorderThread.quit();
  if (!m_recorderThread.wait(QDeadlineTimer(3s))) {
    qCWarning(audioStreamRecorderLog) << "Failed to quit recorder thread!";
  }
}

QUrl AudioStreamRecorder::outputLocation() const {
  return m_outputLocation;
}

void AudioStreamRecorder::setOutputLocation(const QUrl &outputLocation) {
  if (m_outputLocation != outputLocation) {
    m_outputLocation = outputLocation;
    m_recorderWorker->setOutputLocation(m_outputLocation);

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
    m_recorderWorker->setRecordingPolicy(policy);

    emit recordingPolicyChanged();
  }
}

void AudioStreamRecorder::reset() {
  m_errorString = {};
  m_recorderWorker->reset();
}

AudioStreamRecorder::RecordingNameFormat
AudioStreamRecorder::recordingNameFormat() const {
  return m_recordingNameFormat;
}

void AudioStreamRecorder::setRecordingNameFormat(RecordingNameFormat format) {
  if (m_recordingNameFormat != format) {
    m_recordingNameFormat = format;
    m_recorderWorker->setRecordingNameFormat(m_recordingNameFormat);

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

QString AudioStreamRecorder::stationName() const {
  return m_stationName;
}

void AudioStreamRecorder::setStationName(const QString &stationName) {
  if (m_stationName != stationName) {
    m_stationName = stationName;
    removeSpecialCharacters(m_stationName);
    m_recorderWorker->setStationName(m_stationName);

    emit stationNameChanged();
  }
}

bool AudioStreamRecorder::canSaveRecording() const {
  return m_recorderWorker->canSave();
}

void AudioStreamRecorder::saveRecording() {
  if (canSaveRecording()) {
    m_recorderWorker->save();
  }
}

RecorderSink *AudioStreamRecorder::recorderSink() const {
  return m_recorderWorker.get();
}

AudioRecorderSinkWorker::~AudioRecorderSinkWorker() {
  reset();
}

void AudioRecorderSinkWorker::reset() {
  std::unique_lock locker(m_mutex);
  resetInternal();
}

void AudioRecorderSinkWorker::resetInternal() {
  qCDebug(audioStreamRecorderLog) << "Reset internal state";

  m_muxer.close();
  m_tempFile = {};
  m_streamTitle = {};
  m_startTime = QDateTime::currentDateTime();
  m_state = IdleState;
}

QString AudioRecorderSinkWorker::recordingName() const {
  QString streamTitle = m_streamTitle;

  if (m_streamTitle.isEmpty() ||
      m_recordingPolicy == AudioStreamRecorder::NoRecordingPolicy) {
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

void AudioRecorderSinkWorker::performCopy(std::unique_ptr<QTemporaryFile> file,
                                          const QString &destination) {
  QThreadPool::globalInstance()->start(
    [this, readFile = std::move(file), destination]() {
    QFile saveFile(destination);

    if (!saveFile.open(QIODeviceBase::ReadWrite)) {
      QString errorString = QString("Failed to open destination file: %1")
                              .arg(saveFile.errorString());

      emit errorOccurred(errorString);

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

      emit errorOccurred(errorString);

      qCWarning(audioStreamRecorderLog).noquote() << errorString;
    }
  });
}

bool AudioRecorderSinkWorker::beginRecording(
  const ffmpeg::metadata_map &metadata) {
  qCDebug(audioStreamRecorderLog) << "Prepare recording";
  m_state = PreparingState;

  m_tempFile = std::make_unique<QTemporaryFile>();
  if (m_tempFile->open() &&
      m_tempFile->rename(m_tempFile->fileName() + u".mp3"_s) &&
      m_tempFile->open()) {
    /* OK */
  } else {
    QString errorString =
      QString("Failed to open temporary file for recording: %1")
        .arg(m_tempFile->errorString());

    emit errorOccurred(errorString);
    qCWarning(audioStreamRecorderLog).noquote() << errorString;
    return false;
  }

  const QString tempRecordingFilename = m_tempFile->fileName();

  std::error_code ec = m_muxer.open(tempRecordingFilename.toUtf8().constData());
  if (ec) {
    QString errorString =
      QString("Failed to open file for recording: %1 %2")
        .arg(tempRecordingFilename, QString::fromStdString(ec.message()));

    emit errorOccurred(errorString);
    qCWarning(audioStreamRecorderLog).noquote() << errorString;
    return false;
  }

  qCDebug(audioStreamRecorderLog) << "Write file header";

  ec = m_muxer.write_header(metadata);
  if (ec) {
    qCWarning(audioStreamRecorderLog)
      << "Failed to write header:" << ec.message();
    return false;
  }

  m_state = RecordingState;
  qCDebug(audioStreamRecorderLog) << "Begin recording";
  return true;
}

void AudioRecorderSinkWorker::saveInternal() {
  QString outputFileName =
    outputLocationToDir(m_outputLocation).filePath(recordingName() + u".mp3"_s);

  qCDebug(audioStreamRecorderLog)
    << "Save recording at location:" << outputFileName;

  if (m_state == RecordingState) {
    std::error_code ec = m_muxer.write_trailer();
    if (ec) {
      qCWarning(audioStreamRecorderLog)
        << "Failed to write trailer:" << ec.message();
      resetInternal();
      return;
    }

    qCDebug(audioStreamRecorderLog)
      << QString("Rename %1 to %2").arg(m_tempFile->fileName(), outputFileName);

    if (m_tempFile->rename(outputFileName)) {
      /* Successfully renamed file. No remove needed */
      m_tempFile->setAutoRemove(false);
    } else if (!m_tempFile->copy(outputFileName)) {
      QString errorString =
        QString("Failed to copy recording file from %1 to %2.")
          .arg(m_tempFile->fileName(), outputFileName);

      qCWarning(audioStreamRecorderLog).noquote() << errorString;

      /* NOTE: Final try. Especially useful for Android */
      performCopy(std::move(m_tempFile), outputFileName);
    }

    resetInternal();
  }
}

void AudioRecorderSinkWorker::save() {
  const std::unique_lock locker(m_mutex);
  saveInternal();
}

void AudioRecorderSinkWorker::send(const ffmpeg::frame &frame,
                                   const QString &streamTitle) {
  /* Worker thread should do the job not others */
  assert(QThread::currentThread() == thread());

  std::unique_lock locker(m_mutex);

  if (m_recordingPolicy ==
        AudioStreamRecorder::SaveRecordingWhenStreamTitleChanges &&
      !streamTitle.isEmpty() && !m_streamTitle.isEmpty() &&
      m_streamTitle != streamTitle) {
    saveInternal();
  }

  if (m_state != RecordingState) {
    ffmpeg::metadata_map metadata;

    if (!streamTitle.isEmpty()) {
      metadata["StreamTitle"] = streamTitle.toStdString();
    }

    if (!beginRecording(metadata)) {
      resetInternal();
      return;
    }
  }

  std::error_code ec = m_muxer.write(frame);
  if (ec) {
    qCWarning(audioStreamRecorderLog)
      << "Failed to write frame to output file:" << ec.message();
  }

  m_streamTitle = streamTitle;
}

bool AudioRecorderSinkWorker::canSave() const {
  const std::unique_lock locker(m_mutex);
  return m_state == RecordingState;
}

void AudioRecorderSinkWorker::setRecordingPolicy(
  AudioStreamRecorder::RecordingPolicy policy) {
  const std::unique_lock locker(m_mutex);
  m_recordingPolicy = policy;
}

AudioStreamRecorder::RecordingPolicy
AudioRecorderSinkWorker::recordingPolicy() const {
  const std::unique_lock locker(m_mutex);
  return m_recordingPolicy;
}

AudioStreamRecorder::RecordingNameFormat
AudioRecorderSinkWorker::recordingNameFormat() const {
  const std::unique_lock locker(m_mutex);
  return m_recordingNameFormat;
}

void AudioRecorderSinkWorker::setRecordingNameFormat(
  AudioStreamRecorder::RecordingNameFormat format) {
  const std::unique_lock locker(m_mutex);
  m_recordingNameFormat = format;
}

QString AudioRecorderSinkWorker::stationName() const {
  const std::unique_lock locker(m_mutex);
  return m_stationName;
}

void AudioRecorderSinkWorker::setStationName(const QString &stationName) {
  const std::unique_lock locker(m_mutex);
  m_stationName = stationName;
}

AudioRecorderSinkWorker::State AudioRecorderSinkWorker::state() const {
  const std::unique_lock locker(m_mutex);
  return m_state;
};

void AudioRecorderSinkWorker::setState(State state) {
  const std::unique_lock locker(m_mutex);
  m_state = state;
}

QUrl AudioRecorderSinkWorker::outputLocation() const {
  const std::unique_lock locker(m_mutex);
  return m_outputLocation;
}

void AudioRecorderSinkWorker::setOutputLocation(const QUrl &outputLocation) {
  const std::unique_lock locker(m_mutex);
  m_outputLocation = outputLocation;
}
