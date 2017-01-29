#ifndef COREAUDIO_H_
#define COREAUDIO_H_

#include "audiosamplebuffer.h"

#import <AudioToolbox/AudioToolbox.h>


struct GBAudioContext
{
  AudioUnit outputUnit;
  AudioSampleBuffer* audioSampleBuffer;
};


struct GBAudioContext* initCoreAudioPlayback(AudioSampleBuffer* audioSampleBuffer);

#endif // COREAUDIO_H_
