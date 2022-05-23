/**
 * Navit, a modular navigation system.
 * Copyright (C) 2017-2017 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
// style with: clang-format -style=WebKit -i *

#include "Qt5EspeakAudioOut.h"
extern "C" {
#include "debug.h"
}

Qt5EspeakAudioOut::Qt5EspeakAudioOut(int samplerate, const char* category) {
    data = new QByteArray();
    buffer = new QBuffer(data);
    buffer->open(QIODevice::ReadWrite);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(samplerate);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        dbg(lvl_error,
            "Raw audio format not supported by backend, cannot play audio.");
        return;
    }

    audio = new QAudioOutput(format, this);
    /* try to set a rather huge buffer size in order to avoid chopping due to
    * event loop
    * not getting idle. Drawing may take just too long time. This hopefully
    * removes the
    * need to do multi threading with all its problems. May be a problem on
    * systems with
    * really low memory.*/
    audio->setBufferSize((samplerate * 1 /*ch*/ * 2 /*samplezize*/) * 5 /*seconds*/);
    dbg(lvl_debug, "Buffer size is: %d", audio->bufferSize());
    if (category != NULL)
        audio->setCategory(QString(category));

    connect(audio, SIGNAL(stateChanged(QAudio::State)), this,
            SLOT(handleStateChanged(QAudio::State)));
    /* to cope with resume coming from other thread (of libespeak)*/
    connect(this, SIGNAL(call_resume(int)), this, SLOT(resume(int)));
}

Qt5EspeakAudioOut::~Qt5EspeakAudioOut() {
    delete (audio);
    audio = NULL;
    delete (buffer);
    buffer = NULL;
    delete (data);
    data = NULL;
}

void Qt5EspeakAudioOut::handleStateChanged(QAudio::State newState) {
    dbg(lvl_debug, "Enter %d", newState);
    switch (newState) {
    case QAudio::ActiveState:
        break;
    case QAudio::SuspendedState:
        break;
    case QAudio::StoppedState:
        break;
    case QAudio::IdleState:
        /*remove all data that was already read*/
        data->remove(0, buffer->pos());
        buffer->seek(0);
        dbg(lvl_debug, "Size %d", data->size());
        break;
// Sailfish's QT version doesn't have this. Doesn't do anything either.
//    case QAudio::InterruptedState:
    default:
        break;
    }
}

void Qt5EspeakAudioOut::resume(int state) {
    dbg(lvl_debug, "Enter %d", state);
    if (audio->state() != QAudio::ActiveState)
        audio->start(buffer);
}

void Qt5EspeakAudioOut::addSamples(short* wav, int numsamples) {
    dbg(lvl_debug, "Enter (%d samples)", numsamples);

    /*remove all data that was already read (if any)*/
    data->remove(0, buffer->pos());
    buffer->seek(0);

    if (numsamples > 0) {
        data->append((const char*)wav, numsamples * sizeof(short));
        dbg(lvl_debug, "%ld samples in buffer",
            (long int)(buffer->size() / sizeof(short)));
        emit call_resume(numsamples);
    }
}
