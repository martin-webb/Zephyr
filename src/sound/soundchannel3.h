#ifndef SOUND_SOUNDCHANNEL3_H_
#define SOUND_SOUNDCHANNEL3_H_

#define IO_REG_ADDRESS_NR30 0xFF1A
#define IO_REG_ADDRESS_NR31 0xFF1B
#define IO_REG_ADDRESS_NR32 0xFF1C
#define IO_REG_ADDRESS_NR33 0xFF1D
#define IO_REG_ADDRESS_NR34 0xFF1E

#define IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN 0xFF30
#define IO_REG_ADDRESS_WAVE_PATTERN_RAM_END 0xFF3F

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint8_t nr30;
  uint8_t nr31;
  uint8_t nr32;
  uint8_t nr33;
  uint8_t nr34;

  uint8_t* nr52;

  uint8_t wavePatternRAM[16];

  uint32_t frequencyCycles;
  uint32_t frequencyCyclesInPeriod;

  uint16_t length;
  bool counterSelection;
} SoundChannel3;

void initSoundChannel3(SoundChannel3* channel, uint8_t* nr52);

uint8_t soundChannel3ReadByte(SoundChannel3* channel, uint16_t address);
void soundChannel3WriteByte(SoundChannel3* channel, uint16_t address, uint8_t value);

void soundChannel3Reset(SoundChannel3* channel);
void soundChannel3Trigger(SoundChannel3* channel);
void soundChannel3Update(SoundChannel3* channel, uint8_t cyclesExecuted);

void soundChannel3ClockLength(SoundChannel3* channel);

int16_t soundChannel3GetCurrentSample(SoundChannel3* channel);

#endif // SOUND_SOUNDCHANNEL3_H_
