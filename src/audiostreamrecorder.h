#ifndef AUDIOSTREAMRECORDER_H
#define AUDIOSTREAMRECORDER_H

#include <QObject>
#include <QtQml>

#include "ffmpeg/frame.h"
#include "ffmpeg/muxer.h"

class RecorderSink : public QObject {
public:
  RecorderSink() : QObject(nullptr) {}

  virtual void send(const ffmpeg::frame &frame, const QString &streamTitle) = 0;
};

class AudioRecorderSinkWorker;

class AudioStreamRecorder : public QObject {
  Q_OBJECT
  Q_PROPERTY(QUrl outputLocation READ outputLocation WRITE setOutputLocation
               NOTIFY outputLocationChanged)
  Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
  Q_PROPERTY(RecordingPolicy recordingPolicy READ recordingPolicy WRITE
               setRecordingPolicy NOTIFY recordingPolicyChanged)
  Q_PROPERTY(RecordingNameFormat recordingNameFormat READ recordingNameFormat
               WRITE setRecordingNameFormat NOTIFY recordingNameFormatChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorOccurred)
  Q_PROPERTY(QString stationName READ stationName WRITE setStationName NOTIFY
               stationNameChanged)
  QML_ELEMENT

public:
  explicit AudioStreamRecorder(QObject *parent = nullptr);
  ~AudioStreamRecorder() override;

  enum RecordingNameFormat {
    StationDateTime = 0,
    StationTrackNameDateTime,
    TrackNameDateTime,
  };
  Q_ENUM(RecordingNameFormat)

  enum RecordingPolicy {
    NoRecordingPolicy = 0,
    SaveRecordingWhenStreamTitleChanges,
  };
  Q_ENUM(RecordingPolicy)

  QUrl outputLocation() const;
  void setOutputLocation(const QUrl &outputLocation);
  bool recording() const;

  RecordingPolicy recordingPolicy() const;
  void setRecordingPolicy(RecordingPolicy policy);

  RecordingNameFormat recordingNameFormat() const;
  void setRecordingNameFormat(RecordingNameFormat format);

  QString errorString() const;

  QString stationName() const;
  void setStationName(const QString &stationName);
  RecorderSink *recorderSink() const;

  Q_INVOKABLE void record();
  Q_INVOKABLE void stop();
  Q_INVOKABLE bool canSaveRecording() const;
  Q_INVOKABLE void saveRecording();

signals:
  void outputLocationChanged();
  void recordingChanged();
  void recordingPolicyChanged();
  void recordingNameFormatChanged();
  void stationNameChanged();
  void errorOccurred();

protected:
  void reset();
  void setError(const QString &errorString);
  void setRecording(bool recording);
  QString recordingName() const;
  QString dateTimePath() const;

private:
  QUrl m_outputLocation;
  QString m_errorString;
  QString m_stationName;
  RecordingPolicy m_recordingPolicy;
  RecordingNameFormat m_recordingNameFormat;

  std::unique_ptr<AudioRecorderSinkWorker> m_recorderWorker;
  QThread m_recorderThread;

  bool m_recording;
};

class AudioRecorderSinkWorker : public RecorderSink {
  Q_OBJECT

public:
  explicit AudioRecorderSinkWorker() = default;
  ~AudioRecorderSinkWorker() override;

  enum State {
    IdleState = 0,
    PreparingState,
    RecordingState,
  };
  Q_ENUM(State)

  void send(const ffmpeg::frame &frame, const QString &streamTitle) override;
  void reset();
  State state() const;

  bool canSave() const;
  void save();

  AudioStreamRecorder::RecordingNameFormat recordingNameFormat() const;
  void setRecordingNameFormat(AudioStreamRecorder::RecordingNameFormat format);

  void setRecordingPolicy(AudioStreamRecorder::RecordingPolicy policy);
  AudioStreamRecorder::RecordingPolicy recordingPolicy() const;

  QString stationName() const;
  void setStationName(const QString &stationName);

  QUrl outputLocation() const;
  void setOutputLocation(const QUrl &outputLocation);

signals:
  void errorOccurred(const QString &errorString);

private:
  bool beginRecording(const ffmpeg::metadata_map &metadata);
  void performCopy(std::unique_ptr<QTemporaryFile> file,
                   const QString &destination);
  void setState(State state);
  void resetInternal();
  QString recordingName() const;

private:
  ffmpeg::muxer m_muxer;
  mutable std::mutex m_mutex;
  std::unique_ptr<QTemporaryFile> m_tempFile;
  State m_state;

  QDateTime m_startTime;

  QString m_outputFileName;
  QString m_streamTitle;
  QString m_stationName;

  QUrl m_outputLocation;

  AudioStreamRecorder::RecordingPolicy m_recordingPolicy;
  AudioStreamRecorder::RecordingNameFormat m_recordingNameFormat;
};

#endif /* !AUDIOSTREAMRECORDER_H */
