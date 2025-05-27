#ifndef VOICEOBJECT_H
#define VOICEOBJECT_H

#include <QObject>
#include "item.h"
#include "voice.h"

class VoiceObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    //Q_PROPERTY(struct voice * voice READ voice WRITE setVoice NOTIFY voiceChanged)

public:
    VoiceObject(QObject *parent=0);
    VoiceObject(const QString &name, const bool &active, struct voice *v, QObject *parent=0);

    QString name() const;
    void setName(const QString &name);

    bool active() const;
    void setActive(const bool &active);

    struct voice * voice() const;
    void setVoice(struct voice * voice);

signals:
    void nameChanged();
    void activeChanged();
    void voiceChanged();

private:
    QString m_name;
    bool m_active;
    struct voice *m_voice;
};

#endif // MAPOBJECT_H
