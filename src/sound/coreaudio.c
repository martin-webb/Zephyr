#include "coreaudio.h"

#include "../logging.h"

static void CHECK_ERROR(OSStatus error, const char* operation)
{
  if (error == noErr) {
    return;
  }

  char errorString[20];
  *(UInt32*)(errorString + 1) = CFSwapInt32HostToBig(error);
  if (isprint(errorString[0]) && isprint(errorString[2]) && isprint(errorString[3]) && isprint(errorString[4])) {
    errorString[0] = errorString[5] = '\'';
    errorString[6] = '\0';
  } else {
    sprintf(errorString, "%d", (int)error);
  }

  fprintf(stderr, "Error: %s (%s)\n", operation, errorString);
  exit(1);
}


static OSStatus GBAudioUnitRenderProc(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
                                      const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                                      AudioBufferList* ioData)
{
  struct GBAudioInfo* gbAudio = (struct GBAudioInfo*)inRefCon;
  AudioSampleBuffer* audioSampleBuffer = gbAudio->audioSampleBuffer;

  Float32* buffer0Data = (Float32*)ioData->mBuffers[0].mData;
  Float32* buffer1Data = (Float32*)ioData->mBuffers[1].mData;

  int availableFrames = sampleBufferAvailableSamples(audioSampleBuffer);

  if (availableFrames >= 4096) {
    debug("%s: Too many frames available (%d)\n", __func__, availableFrames);
  }

  if (availableFrames < inNumberFrames) {
    debug("%s: audio thread needs %u samples, only %d available (zeroing buffer)\n", __func__, inNumberFrames, availableFrames);
    for (int i = 0; i < inNumberFrames; i++) {
      *buffer0Data++ = 0;
      *buffer1Data++ = 0;
    }
  } else {
    // debug("%s: audio thread needs %u samples, %d available\n", __func__, inNumberFrames, availableFrames);
    // TODO: Implement emulator "fast mode"
    for (int i = 0; i < inNumberFrames; i++) {
      AudioSample sample = sampleBufferGet(audioSampleBuffer);

      Float32 left = sample.so1 / ((float)SHRT_MAX);
      Float32 right = sample.so2 / ((float)SHRT_MAX);

      *buffer0Data++ = left;
      *buffer1Data++ = right;
    }
  }

  return noErr;
}


struct GBAudioInfo* initCoreAudioPlayback(AudioSampleBuffer* audioSampleBuffer)
{
  struct GBAudioInfo* audioInfo = (struct GBAudioInfo*)malloc(1 * sizeof(struct GBAudioInfo));
  memset(audioInfo, 0, sizeof(struct GBAudioInfo));
  audioInfo->audioSampleBuffer = audioSampleBuffer;

  AudioComponentDescription outputDescription = {0};
  outputDescription.componentType = kAudioUnitType_Output;
  outputDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
  outputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;

  AudioComponent component = AudioComponentFindNext(NULL, &outputDescription);
  if (component == NULL) {
    fprintf(stderr, "Can't get output unit.\n");
    exit(1);
  }

  CHECK_ERROR(AudioComponentInstanceNew(component, &audioInfo->outputUnit), "AudioComponentInstanceNew");

  AURenderCallbackStruct input;
  input.inputProc = GBAudioUnitRenderProc;
  input.inputProcRefCon = &audioInfo->outputUnit;

  CHECK_ERROR(AudioUnitSetProperty(audioInfo->outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &input, sizeof(input)), "AudioUnitSetProperty");
  CHECK_ERROR(AudioUnitInitialize(audioInfo->outputUnit), "AudioUnitInitialize");
  CHECK_ERROR(AudioOutputUnitStart(audioInfo->outputUnit), "AudioOutputUnitStart");

  return audioInfo;
}
