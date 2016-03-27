#ifndef COREAUDIO_H_
#define COREAUDIO_H_

#include "audiosamplebuffer.h"

#import <AudioToolbox/AudioToolbox.h>

struct GBAudioInfo
{
  AudioUnit outputUnit;
  AudioSampleBuffer* audioSampleBuffer;
};

struct GBAudioInfo* initCoreAudioPlayback(AudioSampleBuffer* audioSampleBuffer);

#endif // COREAUDIO_H_