#include "Qt5EspeakAudioOut.h"
extern "C" {
#include "debug.h"
}

Qt5EspeakAudioOut::Qt5EspeakAudioOut(int samplerate)
{
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
   if (!info.isFormatSupported(format))
   {
      dbg(lvl_error,"Raw audio format not supported by backend, cannot play audio.\n");
      return;
   }
   audio = new QAudioOutput(format, this);
   connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
   connect(this, SIGNAL(call_resume(int)), this, SLOT(resume(int)));
}

Qt5EspeakAudioOut::~Qt5EspeakAudioOut()
{
   delete(audio);
   audio = NULL;
   delete(buffer);
   buffer = NULL;
   delete(data);
   data = NULL;
}

void Qt5EspeakAudioOut::handleStateChanged(QAudio::State newState)
{
   dbg(lvl_debug, "Enter %d\n", newState);
   switch(newState)
   {
      case QAudio::ActiveState:
      break;
      case QAudio::SuspendedState:
      break;
      case QAudio::StoppedState:
      break;
      case QAudio::IdleState:
         dbg(lvl_debug,"Size %d\n",data->size());
      break;
   }
}

void Qt5EspeakAudioOut::resume(int state)
{
   dbg(lvl_debug,"Enter %d\n", state);
   if(audio->state() != QAudio::ActiveState)
      audio->start(buffer);
}

void Qt5EspeakAudioOut::addSamples(short *wav, int numsamples)
{
   dbg(lvl_debug, "Enter (%d samples)\n", numsamples);
   if(numsamples > 0)
   {
      data->append((const char *) wav, numsamples * sizeof(short));
      dbg(lvl_debug, "%ld samples in buffer\n",(long int)(buffer->size()/sizeof(short)));
      emit call_resume(numsamples);
   }
}
