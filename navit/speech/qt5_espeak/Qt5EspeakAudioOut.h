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
   Qt5EspeakAudioOut(int samplerate);
   ~Qt5EspeakAudioOut();
   void addSamples(short *wav, int numsamples);
public slots:
   void handleStateChanged(QAudio::State newState);
   void resume(int state);
signals:
   void call_resume(int state);
protected:
private:
   QByteArray * data;
   QBuffer * buffer;
   QAudioOutput * audio;
};
#endif
