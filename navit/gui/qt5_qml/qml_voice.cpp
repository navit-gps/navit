#include "qml_voice.h"

VoiceObject::VoiceObject(QObject *parent)
    : QObject(parent) {
}

VoiceObject::VoiceObject(const QString &name, const bool &active, struct voice *v, QObject *parent)
    : QObject(parent), m_name(name), m_active(active), m_voice(v) {
}

QString VoiceObject::name() const {
    return m_name;
}

void VoiceObject::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

bool VoiceObject::active() const {
    return m_active;
}

void VoiceObject::setActive(const bool &active) {
    if (active != m_active) {
        m_active = active;
        emit activeChanged();
    }
}

struct voice * VoiceObject::voice() const {
    return m_voice;
}

void VoiceObject::setVoice(struct voice * voice) {
    if (voice != m_voice) {
        m_voice = voice;
        emit voiceChanged();
    }
}


