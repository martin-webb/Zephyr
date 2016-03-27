#ifndef SOUND_SOUNDCONTROLLER_H_
#define SOUND_SOUNDCONTROLLER_H_

#include "audiosample.h"
#include "soundchannel1.h"
#include "soundchannel2.h"
#include "soundchannel3.h"
#include "soundchannel4.h"

#include <stdint.h>

#define IO_REG_ADDRESS_NR50 0xFF24
#define IO_REG_ADDRESS_NR51 0xFF25
#define IO_REG_ADDRESS_NR52 0xFF26

typedef struct
{
  uint8_t nr50;
  uint8_t nr51;
  uint8_t nr52;

  uint16_t frameCycles;
  uint8_t frameStep;

  uint8_t frameSequencerStep;
  uint16_t frameSequencerCycles;

  SoundChannel1 channel1;
  SoundChannel2 channel2;
  SoundChannel3 channel3;
  SoundChannel4 channel4;

  // Emulator master channel controls
  bool channel1Master;
  bool channel2Master;
  bool channel3Master;
  bool channel4Master;
} SoundController;

void initSoundController(SoundController* soundController);

void soundControllerReset(SoundController* soundController);
void soundOn(SoundController* soundController);
void soundOff(SoundController* soundController);

uint8_t soundControllerReadByte(SoundController* soundController, uint16_t address);
void soundControllerWriteByte(SoundController* soundController, uint16_t address, uint8_t value);

void soundUpdate(SoundController* soundController, uint8_t cyclesExecuted);
AudioSample soundGetCurrentSample(SoundController* soundController);

#endif // SOUND_SOUNDCONTROLLER_H_