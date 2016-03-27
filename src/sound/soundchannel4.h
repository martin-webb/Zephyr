#ifndef SOUND_SOUNDCHANNEL4_H_
#define SOUND_SOUNDCHANNEL4_H_

#define IO_REG_ADDRESS_NR40 0xFF1F // Unused
#define IO_REG_ADDRESS_NR41 0xFF20
#define IO_REG_ADDRESS_NR42 0xFF21
#define IO_REG_ADDRESS_NR43 0xFF22
#define IO_REG_ADDRESS_NR44 0xFF23

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint8_t nr41;
  uint8_t nr42;
  uint8_t nr43;
  uint8_t nr44;

  uint8_t* nr52;

  bool on;

  uint32_t frequencyCycles;
  uint32_t frequencyCyclesInPeriod;

  uint8_t length;
  bool counterSelection;

  uint8_t volume;
  bool envelopeActive;
  bool envelopeIncrease;
  uint8_t envelopeSweepPeriod;
  uint8_t envelopeSweepRemaining;

  uint16_t lfsr;

  bool zombie;
} SoundChannel4;

void initSoundChannel4(SoundChannel4* channel, uint8_t* nr52);

uint8_t soundChannel4ReadByte(SoundChannel4* channel, uint16_t address);
void soundChannel4WriteByte(SoundChannel4* channel, uint16_t address, uint8_t value);

void soundChannel4Reset(SoundChannel4* channel);
void soundChannel4Trigger(SoundChannel4* channel);
void soundChannel4Update(SoundChannel4* channel, uint8_t cyclesExecuted);

void soundChannel4ClockLength(SoundChannel4* channel);
void soundChannel4ClockVolume(SoundChannel4* channel);

int16_t soundChannel4GetCurrentSample(SoundChannel4* channel);

#endif // SOUND_SOUNDCHANNEL4_H_