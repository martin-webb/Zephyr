#ifndef SOUND_AUDIOSAMPLEBUFFER_H_
#define SOUND_AUDIOSAMPLEBUFFER_H_

#include "audiosample.h"

#include <stdbool.h>

typedef struct
{
  AudioSample* data;
  AudioSample last;
  int size;
  int read;
  int write;
  bool overflowed;
} AudioSampleBuffer;

void sampleBufferInitialise(AudioSampleBuffer* buffer, int size);
void sampleBufferFinalise(AudioSampleBuffer* buffer);

AudioSample sampleBufferGet(AudioSampleBuffer* buffer);
void sampleBufferPut(AudioSampleBuffer* buffer, AudioSample sample);

int sampleBufferAvailableSamples(AudioSampleBuffer* buffer);

#endif // SOUND_AUDIOSAMPLEBUFFER_H_
