#ifndef Qt5EspeakAudioOut_h
#define Qt5EspeakAudioOut_h
#include <QObject>
#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
class Qt5EspeakAudioOut:public QObject
{
   Q_OBJECT
public:
   /* Instantiate this. Parameters are the sample rate to use,
    * and the category to sort this audio output to. Not all platforms
    * will honour category */
   Qt5EspeakAudioOut(int samplerate, const char * category);
   ~Qt5EspeakAudioOut();
   /* Add new samples to this class. The samples will be played*/
   void addSamples(short *wav, int numsamples);
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
private:
   /* internal buffer */
   QByteArray * data;
   QBuffer * buffer;
   /* audio output class */
   QAudioOutput * audio;
};
#endif
