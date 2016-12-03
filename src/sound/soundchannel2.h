#ifndef SOUND_SOUNDCHANNEL2_H_
#define SOUND_SOUNDCHANNEL2_H_

#define IO_REG_ADDRESS_NR20 0xFF15 // Unused
#define IO_REG_ADDRESS_NR21 0xFF16
#define IO_REG_ADDRESS_NR22 0xFF17
#define IO_REG_ADDRESS_NR23 0xFF18
#define IO_REG_ADDRESS_NR24 0xFF19


#include <stdbool.h>
#include <stdint.h>


typedef struct
{
  uint8_t nr21;
  uint8_t nr22;
  uint8_t nr23;
  uint8_t nr24;

  uint8_t* nr52;

  bool on;

  uint16_t frequency;
  uint32_t frequencyCycles;
  uint32_t frequencyCyclesInPeriod;

  uint8_t length;
  bool counterSelection;

  uint8_t volume;
  bool envelopeActive;
  bool envelopeIncrease;
  uint8_t envelopeSweepPeriod;
  uint8_t envelopeSweepRemaining;

  bool zombie;
} SoundChannel2;


void initSoundChannel2(SoundChannel2* channel, uint8_t* nr52);

uint8_t soundChannel2ReadByte(SoundChannel2* channel, uint16_t address);
void soundChannel2WriteByte(SoundChannel2* channel, uint16_t address, uint8_t value);

void soundChannel2Reset(SoundChannel2* channel);
void soundChannel2Trigger(SoundChannel2* channel);
void soundChannel2Update(SoundChannel2* channel, uint8_t cyclesExecuted);

void soundChannel2ClockLength(SoundChannel2* channel);
void soundChannel2ClockVolume(SoundChannel2* channel);

int16_t soundChannel2GetCurrentSample(SoundChannel2* channel);

#endif // SOUND_SOUNDCHANNEL2_H_
