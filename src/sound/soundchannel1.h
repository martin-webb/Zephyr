#ifndef SOUND_SOUNDCHANNEL1_H_
#define SOUND_SOUNDCHANNEL1_H_

#define IO_REG_ADDRESS_NR10 0xFF10
#define IO_REG_ADDRESS_NR11 0xFF11
#define IO_REG_ADDRESS_NR12 0xFF12
#define IO_REG_ADDRESS_NR13 0xFF13
#define IO_REG_ADDRESS_NR14 0xFF14

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint8_t nr10;
  uint8_t nr11;
  uint8_t nr12;
  uint8_t nr13;
  uint8_t nr14;

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

  uint16_t frequencyShadow;
  bool sweepActive;
  bool sweepIncrease;
  uint8_t sweepPeriod;
  uint8_t sweepRemaining;
  uint8_t sweepShift;

  bool zombie;
} SoundChannel1;

void initSoundChannel1(SoundChannel1* channel, uint8_t* nr52);

uint8_t soundChannel1ReadByte(SoundChannel1* channel, uint16_t address);
void soundChannel1WriteByte(SoundChannel1* channel, uint16_t address, uint8_t value);

void soundChannel1Reset(SoundChannel1* channel);
void soundChannel1Trigger(SoundChannel1* channel);
void soundChannel1Update(SoundChannel1* channel, uint8_t cyclesExecuted);

void soundChannel1ClockLength(SoundChannel1* channel);
void soundChannel1ClockVolume(SoundChannel1* channel);
void soundChannel1ClockSweep(SoundChannel1* channel);

int16_t soundChannel1GetCurrentSample(SoundChannel1* channel);

#endif // SOUND_SOUNDCHANNEL1_H_