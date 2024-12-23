#ifndef JSONRESTLISTMODEL_H
#define JSONRESTLISTMODEL_H

#include "abstractrestmodel.h"

class JsonRestListModel : public AbstractRestListModel {
  Q_OBJECT
  Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY
               dataPathChanged FINAL)
  QML_ELEMENT

public:
  explicit JsonRestListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;

  QHash<int, QByteArray> roleNames() const override;
  void reset() override;

  Q_INVOKABLE QVariantMap get(int row);

  QString dataPath() const;
  void setDataPath(const QString &newDataPath);

  void generateRoleNames(const QVariantMap &item);

  static std::optional<QJsonArray> parseJson(const QByteArray &data,
                                             const QString &dataPath = {});

signals:
  void dataPathChanged();
  void fetchMoreRequested();

protected:
  void handleRequestData(const QByteArray &data) override;

private:
  bool tryParseJsonData(const QByteArray &data);

private:
  QList<QVariantMap> m_items;

  QHash<int, QByteArray> m_roleNames;
  int m_roleNameIndex;

  QString m_dataPath;
};

#endif /* !JSONRESTLISTMODEL_H */
