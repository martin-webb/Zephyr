#include "soundchannel4.h"

#include "../logging.h"

#include <math.h>

// #define LOG_IO_WRITES_TO_NR40
// #define LOG_IO_WRITES_TO_NR41
// #define LOG_IO_WRITES_TO_NR42
// #define LOG_IO_WRITES_TO_NR43
// #define LOG_IO_WRITES_TO_NR44
// #define LOG_IO_WRITES_TO_ALL
// #define LOG_CHANNEL_TRIGGERS
// #define LOG_ZOMBIE

void initSoundChannel4(SoundChannel4* channel, uint8_t* nr52)
{
  soundChannel4Reset(channel);

  channel->nr52 = nr52;

  channel->on = false;

  channel->frequencyCycles = 0;
  channel->frequencyCyclesInPeriod = 0;

  channel->length = 0;
  channel->counterSelection = false;

  channel->volume = 0;
  channel->envelopeActive = false;
  channel->envelopeIncrease = false;
  channel->envelopeSweepPeriod = 0;
  channel->envelopeSweepRemaining = 0;
  channel->lfsr = 0x7FFF;

  channel->zombie = false;
}


static void channelOn(SoundChannel4* channel)
{
  channel->on = true;
  (*channel->nr52) |= (1 << 4);
}


static void channelOff(SoundChannel4* channel)
{
  channel->on = false;
  (*channel->nr52) &= ~(1 << 4);
}


static void loadLengthCounter(SoundChannel4* channel)
{
  channel->length = 64 - (channel->nr41 & 63);
  channel->counterSelection = (channel->nr44 & (1 << 6));
}


static void loadFrequency(SoundChannel4* channel)
{
  uint8_t s = (channel->nr43 >> 4);
  uint8_t r = (channel->nr43 & 7);
  uint32_t frequencyInHz = (uint32_t)((float)524288 / ((r == 0) ? 0.5 : r) / pow(2, (s + 1)));
  channel->frequencyCyclesInPeriod = (1024 * 1024 * 4) / frequencyInHz;

  // TODO: What's the correct way to handle the current cycle count? For now, if there are more cycles per period
  // than our current value then simply continue counting from our current position, only reseting the counter
  // if the frequency has increased
  if (channel->frequencyCyclesInPeriod < channel->frequencyCycles) {
    channel->frequencyCycles = channel->frequencyCyclesInPeriod;
  }
}


static void loadVolumeAndEnvelope(SoundChannel4* channel)
{
  channel->volume = (channel->nr42 >> 4);
  channel->envelopeIncrease = (channel->nr42 & (1 << 3));
  channel->envelopeSweepPeriod = (channel->nr42 & 7);
  channel->envelopeSweepRemaining = channel->envelopeSweepPeriod;
  channel->envelopeActive = (channel->envelopeSweepPeriod != 0);
}


static void resetLFSR(SoundChannel4* channel)
{
  channel->lfsr = 0x7FFF;
}


static void stepLFSR(SoundChannel4* channel)
{
  uint8_t bit = ((channel->lfsr >> 0) ^ (channel->lfsr >> 1)) & 1;
  channel->lfsr >>= 1;
  channel->lfsr |= (bit << 14); // 15 bit mode - after the shift the leftmost bit will be 0, so we can simply XOR it into place
  if (channel->nr43 & (1 << 3)) { // 7 bit mode
    channel->lfsr = (channel->lfsr & 0x7F80) | (bit << 6) | (channel->lfsr & 0x3F); // Assuming we want to keep all the bits in the register, we can't just XOR it now...
  }
}


static void enableZombie(SoundChannel4* channel)
{
#ifdef LOG_ZOMBIE
  debug("\b[SCH4] Entering zombie mode\n");
#endif

  channel->zombie = true;
}


static void disableZombie(SoundChannel4* channel)
{
#ifdef LOG_ZOMBIE
  debug("\b[SCH4] Exiting zombie mode\n");
#endif

  channel->zombie = false;
}


static bool isZombie(SoundChannel4* channel)
{
  return channel->zombie;
}


static uint8_t readNR40(SoundChannel4* channel)
{
  return 0xFF;
}


static uint8_t readNR41(SoundChannel4* channel)
{
  return channel->nr41 | 0xFF;
}


static uint8_t readNR42(SoundChannel4* channel)
{
  return channel->nr42 | 0x00;
}


static uint8_t readNR43(SoundChannel4* channel)
{
  return channel->nr43 | 0x00;
}


static uint8_t readNR44(SoundChannel4* channel)
{
  return channel->nr44 | 0xBF;
}


uint8_t soundChannel4ReadByte(SoundChannel4* channel, uint16_t address)
{
  if (address == IO_REG_ADDRESS_NR40) {
    return readNR40(channel);
  } else if (address == IO_REG_ADDRESS_NR41) {
    return readNR41(channel);
  } else if (address == IO_REG_ADDRESS_NR42) {
    return readNR42(channel);
  } else if (address == IO_REG_ADDRESS_NR43) {
    return readNR43(channel);
  } else if (address == IO_REG_ADDRESS_NR44) {
    return readNR44(channel);
  } else {
    return 0x00; // TODO: Warning message
  }
}


static void writeNR40(SoundChannel4* channel, uint8_t value)
{
  // Unused register
#if defined(LOG_IO_WRITES_TO_NR40) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH4][NR40][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR41(SoundChannel4* channel, uint8_t value)
{
  channel->nr41 = value;
  loadLengthCounter(channel);

#if defined(LOG_IO_WRITES_TO_NR41) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH4][NR41][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR42(SoundChannel4* channel, uint8_t value)
{
  // if ((value & 0xF0) != 0) {
  //   channelOn(channel);
  // }

  channel->nr42 = value;

  if (isZombie(channel)) {
    if ((value >> 4) == 0) {
      channelOff(channel);
    }
  }

#if defined(LOG_IO_WRITES_TO_NR42) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH4][NR42][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR43(SoundChannel4* channel, uint8_t value)
{
  channel->nr43 = value;
  loadFrequency(channel);

#if defined(LOG_IO_WRITES_TO_NR43) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH4][NR43][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR44(SoundChannel4* channel, uint8_t value)
{
  channel->nr44 = value;

#if defined(LOG_IO_WRITES_TO_NR44) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH4][NR44][WRITE] 0x%02X\n", value);
#endif

  if (value & (1 << 7)) {
    soundChannel4Trigger(channel);
  } else {
    loadLengthCounter(channel); // TODO: Really?
  }
}


void soundChannel4WriteByte(SoundChannel4* channel, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_NR40) {
    writeNR40(channel, value);
  } else if (address == IO_REG_ADDRESS_NR41) {
    writeNR41(channel, value);
  } else if (address == IO_REG_ADDRESS_NR42) {
    writeNR42(channel, value);
  } else if (address == IO_REG_ADDRESS_NR43) {
    writeNR43(channel, value);
  } else if (address == IO_REG_ADDRESS_NR44) {
    writeNR44(channel, value);
  } else {
    // TODO: Warning
  }
}


void soundChannel4Reset(SoundChannel4* channel)
{
  channel->nr41 = 0x00;
  channel->nr41 = 0xFF;
  channel->nr42 = 0x00;
  channel->nr43 = 0x00;
  channel->nr44 = 0xBF;
}


void soundChannel4Trigger(SoundChannel4* channel)
{
  channelOn(channel);

  loadLengthCounter(channel);
  loadFrequency(channel);
  loadVolumeAndEnvelope(channel);

  resetLFSR(channel);

  disableZombie(channel);

#ifdef LOG_CHANNEL_TRIGGERS
  debug("\b[SCH4][TRIGGER] NR41=0x%02X NR42=0x%02X NR43=0x%02X NR44=0x%02X length=%d counterSel=%d volume=%d env=%d envDir=%d envPeriod=%d clockShift=%d mode=%d divisor=%d\n",
    channel->nr41,
    channel->nr42,
    channel->nr43,
    channel->nr44,
    channel->length,
    channel->counterSelection,
    channel->volume,
    channel->envelopeActive,
    channel->envelopeIncrease,
    channel->envelopeSweepPeriod,
    (channel->nr43 >> 4),
    (channel->nr43 & (1 << 3)) ? 7 : 15,
    (channel->nr43 & 7)
  );
#endif
}


void soundChannel4Update(SoundChannel4* channel, uint8_t cyclesExecuted)
{

  if (channel->frequencyCycles + cyclesExecuted >= channel->frequencyCyclesInPeriod) {
    stepLFSR(channel);
  }
  channel->frequencyCycles = (channel->frequencyCycles + cyclesExecuted) % channel->frequencyCyclesInPeriod;
}


void soundChannel4ClockLength(SoundChannel4* channel)
{
  if (!channel->on || !channel->counterSelection) {
    return;
  }

  channel->length--;

  if (channel->length == 0) {
    channelOff(channel);
  }
}


void soundChannel4ClockVolume(SoundChannel4* channel)
{
  if (!channel->envelopeActive) {
    return;
  }

  channel->envelopeSweepRemaining--;

  if (channel->envelopeSweepRemaining > 0) {
    return;
  }

  if (channel->envelopeIncrease) {
    if (channel->volume < 15) {
      channel->volume++;
    }
    if (channel->volume == 15) {
      channel->envelopeActive = false;
      enableZombie(channel);
    }
  } else {
    if (channel->volume > 0) {
      channel->volume--;
    }
    if (channel->volume == 0) {
      channel->envelopeActive = false;
      enableZombie(channel);
    }
  }

  channel->envelopeSweepRemaining = channel->envelopeSweepPeriod;
}


int16_t soundChannel4GetCurrentSample(SoundChannel4* channel)
{
  int16_t sample = 0;

  if (channel->on) {
    sample = (((channel->lfsr & 1) == 0) ? INT16_MAX : INT16_MIN) * (channel->volume / 15.0);
  }

  return sample;
}