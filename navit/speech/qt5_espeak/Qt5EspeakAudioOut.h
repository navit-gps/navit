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

#ifndef Qt5EspeakAudioOut_h
#define Qt5EspeakAudioOut_h
#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
#include <QObject>
class Qt5EspeakAudioOut : public QObject {
    Q_OBJECT

public:
    /* Instantiate this. Parameters are the sample rate to use,
   * and the category to sort this audio output to. Not all platforms
   * will honour category */
    Qt5EspeakAudioOut(int samplerate, const char* category);
    ~Qt5EspeakAudioOut();
    /* Add new samples to this class. The samples will be played*/
    void addSamples(short* wav, int numsamples);
public slots:
    /* Deal with QAudioOutput status changes */
    void handleStateChanged(QAudio::State newState);
    /* Cause QAusioOutput to resume playing (after samples were added)*/
    void resume(int state);
signals:
    /* Cause QAusioOutput to resume playing. Emit this from different thread
   * as this is not threadsafe*/
    void call_resume(int state);

protected:
    /* None */

private:
    /* internal buffer */
    QByteArray* data;
    QBuffer* buffer;
    /* audio output class */
    QAudioOutput* audio;
};
#endif
