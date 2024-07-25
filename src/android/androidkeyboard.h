#ifndef ANDROIDKEYBOARD_H
#define ANDROIDKEYBOARD_H

#include <QObject>
#include <QReadWriteLock>

#include <QJniEnvironment>
#include <QJniObject>

/**
 * @class AndroidKeyboard
 * @brief Wrapper class for VirtualKeyboard.java
 *
 * @ref https://falsinsoft.blogspot.com/2017/04/qml-resize-controls-when-android.html
 *
 */
class AndroidKeyboard : public QObject {
  Q_OBJECT
  Q_PROPERTY(int height READ height NOTIFY heightChanged FINAL)

public:
  static AndroidKeyboard *instance();

  int height() const;
  static void registerNativeMethods();

signals:
  void heightChanged();

private:
  explicit AndroidKeyboard(QObject *parent = nullptr);

  static void virtualKeyboardStateChanged(JNIEnv *env, jobject thiz,
                                          jint VirtualKeyboardHeight);
  void setHeight(int newHeight);

private:
  int m_height;
  mutable QReadWriteLock m_rwLock;
};

#endif  // ANDROIDKEYBOARD_H