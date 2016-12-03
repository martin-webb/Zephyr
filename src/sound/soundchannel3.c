#include "soundchannel3.h"

#include "../logging.h"

#define WAVE_PATTERN_RAM_NUM_SAMPLES 32

// #define LOG_IO_WRITES_TO_NR30
// #define LOG_IO_WRITES_TO_NR31
// #define LOG_IO_WRITES_TO_NR32
// #define LOG_IO_WRITES_TO_NR33
// #define LOG_IO_WRITES_TO_NR34
// #define LOG_IO_WRITES_TO_ALL
// #define LOG_CHANNEL_TRIGGERS

void initSoundChannel3(SoundChannel3* channel, uint8_t* nr52)
{
  soundChannel3Reset(channel);

  channel->nr52 = nr52;

  for (int i = 0; i < 16; i++) {
    channel->wavePatternRAM[i] = 0;
  }
  channel->frequencyCycles = 0;
  channel->frequencyCyclesInPeriod = 1;

  channel->length = 0;
  channel->counterSelection = false;
}


static void channelOn(SoundChannel3* channel)
{
  (*channel->nr52) |= (1 << 2);
}


static void channelOff(SoundChannel3* channel)
{
  (*channel->nr52) &= ~(1 << 2);
}


static void loadLengthCounter(SoundChannel3* channel)
{
  channel->length = 256 - channel->nr31;
  channel->counterSelection = (channel->nr34 & (1 << 6));
}


static uint32_t frequencyToCycles(uint16_t frequency)
{
  uint32_t frequencyInHz = 65536 / (2048 - frequency);
  return (1024 * 1024 * 4) / frequencyInHz;
}


static void loadFrequency(SoundChannel3* channel)
{
  uint16_t frequency = ((channel->nr34 & 7) << 8) | channel->nr33;
  channel->frequencyCyclesInPeriod = frequencyToCycles(frequency);

  // TODO: What's the correct way to handle the current cycle count? For now, if there are more cycles per period
  // than our current value then simply continue counting from our current position, only reseting the counter
  // if the frequency has increased
  if (channel->frequencyCyclesInPeriod < channel->frequencyCycles) {
    channel->frequencyCycles = channel->frequencyCyclesInPeriod;
  }
}


static uint8_t readNR30(SoundChannel3* channel)
{
  return channel->nr30 | 0x7F;
}


static uint8_t readNR31(SoundChannel3* channel)
{
  return channel->nr31 | 0xFF;
}


static uint8_t readNR32(SoundChannel3* channel)
{
  return channel->nr32 | 0x9F;
}


static uint8_t readNR33(SoundChannel3* channel)
{
  return channel->nr33 | 0xFF;
}


static uint8_t readNR34(SoundChannel3* channel)
{
  return channel->nr34 | 0xBF;
}


static uint8_t readWaveRAM(SoundChannel3* channel, uint16_t address)
{
  return channel->wavePatternRAM[address - IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN];
}


uint8_t soundChannel3ReadByte(SoundChannel3* channel, uint16_t address)
{
  if (address == IO_REG_ADDRESS_NR30) {
    return readNR30(channel);
  } else if (address == IO_REG_ADDRESS_NR31) {
    return readNR31(channel);
  } else if (address == IO_REG_ADDRESS_NR32) {
    return readNR32(channel);
  } else if (address == IO_REG_ADDRESS_NR33) {
    return readNR33(channel);
  } else if (address == IO_REG_ADDRESS_NR34) {
    return readNR34(channel);
  } else if (address >= IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN && address <= IO_REG_ADDRESS_WAVE_PATTERN_RAM_END) {
    return readWaveRAM(channel, address);
  } else {
    return 0; // TODO: Warning message
  }
}


static void writeNR30(SoundChannel3* channel, uint8_t value)
{
  channel->nr30 = value;

#if defined(LOG_IO_WRITES_TO_NR30) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH3][NR30][WRITE] 0x%02X\n", value);
#endif

  if (value & (1 << 7)) {
    channelOn(channel);
    soundChannel3Trigger(channel);
  } else {
    channelOff(channel);
  }
}


static void writeNR31(SoundChannel3* channel, uint8_t value)
{
  channel->nr31 = value;
  loadLengthCounter(channel);

#if defined(LOG_IO_WRITES_TO_NR31) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH3][NR31][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR32(SoundChannel3* channel, uint8_t value)
{
  channel->nr32 = value;

#if defined(LOG_IO_WRITES_TO_NR32) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH3][NR32][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR33(SoundChannel3* channel, uint8_t value)
{
  channel->nr33 = value;
  loadFrequency(channel);

#if defined(LOG_IO_WRITES_TO_NR33) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH3][NR33][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR34(SoundChannel3* channel, uint8_t value)
{
  channel->nr34 = value;
  loadFrequency(channel);

#if defined(LOG_IO_WRITES_TO_NR34) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH3][NR34][WRITE] 0x%02X\n", value);
#endif
}


static void writeWaveRAM(SoundChannel3* channel, uint16_t address, uint8_t value)
{
  channel->wavePatternRAM[address - IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN] = value;
}


void soundChannel3WriteByte(SoundChannel3* channel, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_NR30) {
    writeNR30(channel, value);
  } else if (address == IO_REG_ADDRESS_NR31) {
    writeNR31(channel, value);
  } else if (address == IO_REG_ADDRESS_NR32) {
    writeNR32(channel, value);
  } else if (address == IO_REG_ADDRESS_NR33) {
    writeNR33(channel, value);
  } else if (address == IO_REG_ADDRESS_NR34) {
    writeNR34(channel, value);
  } else if (address >= IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN && address <= IO_REG_ADDRESS_WAVE_PATTERN_RAM_END) {
    writeWaveRAM(channel, address, value);
  } else {
    // TODO: Warning message
  }
}


void soundChannel3Reset(SoundChannel3* channel)
{
  // No change to NR33
  channel->nr30 = 0x7F;
  channel->nr31 = 0xFF;
  channel->nr32 = 0x9F;
  channel->nr34 = 0xBF;
}


static bool soundChannel3On(SoundChannel3* channel)
{
  return (channel->nr30 & (1 << 7));
}


void soundChannel3Trigger(SoundChannel3* channel)
{
  // Enable channel
  // TODO: Does the playback bit need to be set here? Or is this only for channels 1, 2 and 4?

  loadLengthCounter(channel);
  loadFrequency(channel);

#ifdef LOG_CHANNEL_TRIGGERS
  debug("\b[SCH3][TRIGGER] NR30=0x%02X NR31=0x%02X NR32=0x%02X NR33=0x%02X\n", channel->nr30, channel->nr31, channel->nr32, channel->nr33);
#endif
}


void soundChannel3Update(SoundChannel3* channel, uint8_t cyclesExecuted)
{
  channel->frequencyCycles = (channel->frequencyCycles + cyclesExecuted) % channel->frequencyCyclesInPeriod;
  // TODO: Store wave table position to support reading samples from the previous wave table sample on channel trigger
}


void soundChannel3ClockLength(SoundChannel3* channel)
{
  if (!soundChannel3On(channel) || !channel->counterSelection) {
    return;
  }

  channel->length--;

  if (channel->length == 0) {
    channel->nr30 = 0;
    channelOff(channel);
  }
}


int16_t soundChannel3GetCurrentSample(SoundChannel3* channel)
{
  int16_t sample = 0;

  if (soundChannel3On(channel)) {
    // TODO: When to do this initially?
    // uint16_t frequency = ((channel->nr34 & 7) << 8) | channel->nr33;
    // channel->frequencyCyclesInPeriod = frequencyToCycles(frequency);

    uint8_t wavePatternIndex = channel->frequencyCycles / (channel->frequencyCyclesInPeriod / 32);
    uint8_t ramIndex = wavePatternIndex / 2;
    uint8_t sampleBits = 0;
    if (wavePatternIndex % 2 == 0) {
      sampleBits = (channel->wavePatternRAM[ramIndex] >> 4);
    } else {
      sampleBits = (channel->wavePatternRAM[ramIndex] & 15);
    }

    uint8_t shift = ((channel->nr32 >> 5) & 3);
    if (shift == 0) {
      sample = 0;
    } else {
      sampleBits >>= (shift - 1);

      sample = (int16_t)(((sampleBits / 15.0) * UINT16_MAX) - 1 - INT16_MAX);
    }
  }

  return sample;
}
