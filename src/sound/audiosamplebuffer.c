#include "audiosamplebuffer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void sampleBufferInitialise(AudioSampleBuffer* buffer, int size)
{
  buffer->data = (AudioSample*)malloc(size * sizeof(AudioSample));
  assert(buffer->data);

  memset(buffer->data, 0, size * sizeof(AudioSample));

  buffer->last.so1 = 0;
  buffer->last.so2 = 0;
  buffer->size = size;
  buffer->write = 0;
  buffer->read = 0;
  buffer->overflowed = false;
}


void sampleBufferFinalise(AudioSampleBuffer* buffer)
{
  free(buffer->data);
}


AudioSample sampleBufferGet(AudioSampleBuffer* buffer)
{
  AudioSample sample = buffer->data[buffer->read];
  buffer->read = (buffer->read + 1) % buffer->size;
  if (buffer->read == 0 && buffer->overflowed) {
    buffer->overflowed = false;
  }
  buffer->last = sample;
  return sample;
}


void sampleBufferPut(AudioSampleBuffer* buffer, AudioSample sample)
{
  buffer->data[buffer->write] = sample;
  buffer->write = (buffer->write + 1) % buffer->size;
  if (buffer->write == 0) {
    buffer->overflowed = true;
  }
}


int sampleBufferAvailableSamples(AudioSampleBuffer* buffer)
{
  if (buffer->overflowed) {
    return (buffer->size - buffer->read) + buffer->write;
  } else {
    return buffer->write - buffer->read;
  }
}
