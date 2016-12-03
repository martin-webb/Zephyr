#include "soundchannel2.h"

#include "../logging.h"

#include <math.h>


// #define LOG_IO_WRITES_TO_NR20
// #define LOG_IO_WRITES_TO_NR21
// #define LOG_IO_WRITES_TO_NR22
// #define LOG_IO_WRITES_TO_NR23
// #define LOG_IO_WRITES_TO_NR24
// #define LOG_IO_WRITES_TO_ALL
// #define LOG_CHANNEL_TRIGGERS
// #define LOG_ZOMBIE


extern int16_t DUTY_CYCLES[][8];


void initSoundChannel2(SoundChannel2* channel, uint8_t* nr52)
{
  soundChannel2Reset(channel);

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

  channel->zombie = false;
}


static void channelOn(SoundChannel2* channel)
{
  channel->on = true;
  (*channel->nr52) |= (1 << 1);
}


static void channelOff(SoundChannel2* channel)
{
  channel->on = false;
  (*channel->nr52) &= ~(1 << 1);
}


static void loadLengthCounter(SoundChannel2* channel)
{
  channel->length = 64 - (channel->nr21 & 63);
  channel->counterSelection = (channel->nr24 & (1 << 6));
}


static uint32_t frequencyToCycles(uint16_t frequency)
{
  uint32_t frequencyInHz = 131072 / (2048 - frequency);
  return (1024 * 1024 * 4) / frequencyInHz;
}


static void loadFrequency(SoundChannel2* channel)
{
  channel->frequency = ((channel->nr24 & 7) << 8) | channel->nr23;
  channel->frequencyCyclesInPeriod = frequencyToCycles(channel->frequency);

  // TODO: What's the correct way to handle the current cycle count? For now, if there are more cycles per period
  // than our current value then simply continue counting from our current position, only reseting the counter
  // if the frequency has increased
  if (channel->frequencyCyclesInPeriod < channel->frequencyCycles) {
    channel->frequencyCycles = channel->frequencyCyclesInPeriod;
  }
}


static void loadVolumeAndEnvelope(SoundChannel2* channel)
{
  channel->volume = (channel->nr22 >> 4);
  channel->envelopeIncrease = (channel->nr22 & (1 << 3));
  channel->envelopeSweepPeriod = (channel->nr22 & 7);
  channel->envelopeSweepRemaining = channel->envelopeSweepPeriod;
  channel->envelopeActive = (channel->envelopeSweepPeriod != 0);
}


static void enableZombie(SoundChannel2* channel)
{
#ifdef LOG_ZOMBIE
  debug("\b[SCH2] Entering zombie mode\n");
#endif

  channel->zombie = true;
}


static void disableZombie(SoundChannel2* channel)
{
#ifdef LOG_ZOMBIE
  debug("\b[SCH2] Exiting zombie mode\n");
#endif

  channel->zombie = false;
}


static bool isZombie(SoundChannel2* channel)
{
  return channel->zombie;
}


static uint8_t readNR20(SoundChannel2* channel)
{
  return 0xFF;
}


static uint8_t readNR21(SoundChannel2* channel)
{
  return channel->nr21 | 0x3F;
}


static uint8_t readNR22(SoundChannel2* channel)
{
  return channel->nr22 | 0x00;
}


static uint8_t readNR23(SoundChannel2* channel)
{
  return channel->nr23 | 0xFF;
}


static uint8_t readNR24(SoundChannel2* channel)
{
  return channel->nr24 | 0xBF;
}


uint8_t soundChannel2ReadByte(SoundChannel2* channel, uint16_t address)
{
  if (address == IO_REG_ADDRESS_NR20) {
    return readNR20(channel);
  } else if (address == IO_REG_ADDRESS_NR21) {
    return readNR21(channel);
  } else if (address == IO_REG_ADDRESS_NR22) {
    return readNR22(channel);
  } else if (address == IO_REG_ADDRESS_NR23) {
    return readNR23(channel);
  } else if (address == IO_REG_ADDRESS_NR24) {
    return readNR24(channel);
  } else {
    return 0x00; // TODO: Warning message
  }
}


static void writeNR20(SoundChannel2* channel, uint8_t value)
{
  // Unused register
#if defined(LOG_IO_WRITES_TO_NR20) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH2][NR20][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR21(SoundChannel2* channel, uint8_t value)
{
  channel->nr21 = value;
  loadLengthCounter(channel); // TODO: Should this only occur for writes to the length counter bits (and not the duty cycle bits too)?

#if defined(LOG_IO_WRITES_TO_NR21) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH2][NR21][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR22(SoundChannel2* channel, uint8_t value)
{
  channel->nr22 = value;

  if (isZombie(channel)) {
    if ((value >> 4) == 0) {
      channelOff(channel);
    }
  }

#if defined(LOG_IO_WRITES_TO_NR22) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH2][NR22][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR23(SoundChannel2* channel, uint8_t value)
{
  uint8_t currentFrequencyBits = channel->nr23;
  channel->nr23 = value;
  if (currentFrequencyBits != value) {
    loadFrequency(channel);
  }

#if defined(LOG_IO_WRITES_TO_NR23) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH2][NR23][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR24(SoundChannel2* channel, uint8_t value)
{
  uint8_t currentFrequencyBits = (channel->nr24 & 7);
  channel->nr24 = value;

#if defined(LOG_IO_WRITES_TO_NR24) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH2][NR24][WRITE] 0x%02X\n", value);
#endif

  if (value & (1 << 7)) {
    soundChannel2Trigger(channel); // Channel trigger will also reload the frequency, so no need to do it twice
  } else if (currentFrequencyBits != (value & 7)) {
    loadFrequency(channel);
  }
}


void soundChannel2WriteByte(SoundChannel2* channel, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_NR20) {
    writeNR20(channel, value);
  } else if (address == IO_REG_ADDRESS_NR21) {
    writeNR21(channel, value);
  } else if (address == IO_REG_ADDRESS_NR22) {
    writeNR22(channel, value);
  } else if (address == IO_REG_ADDRESS_NR23) {
    writeNR23(channel, value);
  } else if (address == IO_REG_ADDRESS_NR24) {
    writeNR24(channel, value);
  } else {
    // TODO: Warning message
  }
}


void soundChannel2Reset(SoundChannel2* channel)
{
  // No change to NR20 or NR23
  channel->nr21 = 0x3F;
  channel->nr22 = 0x00;
  channel->nr24 = 0xBF;
}


void soundChannel2Trigger(SoundChannel2* channel)
{
  channelOn(channel);

  loadLengthCounter(channel);
  loadFrequency(channel);
  loadVolumeAndEnvelope(channel);

  disableZombie(channel);

#ifdef LOG_CHANNEL_TRIGGERS
  debug("\b[SCH2][TRIGGER] NR21=0x%02X NR22=0x%02X NR23=0x%02X NR24=0x%02X\n", channel->nr21, channel->nr22, channel->nr23, channel->nr24);
#endif
}


void soundChannel2Update(SoundChannel2* channel, uint8_t cyclesExecuted)
{
  // uint16_t frequency = ((channel->nr24 & 7) << 8) | channel->nr23;
  // channel->frequencyCyclesInPeriod = frequencyToCycles(frequency);

  channel->frequencyCycles = (channel->frequencyCycles + cyclesExecuted) % channel->frequencyCyclesInPeriod;

}


void soundChannel2ClockLength(SoundChannel2* channel)
{
  if (!channel->on || !channel->counterSelection) {
    return;
  }

  channel->length--;

  if (channel->length == 0) {
    channelOff(channel);
  }
}


void soundChannel2ClockVolume(SoundChannel2* channel)
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


int16_t soundChannel2GetCurrentSample(SoundChannel2* channel)
{
  int16_t sample = 0;

  if (channel->on) {
    uint8_t dutyNumber = (channel->nr21 >> 6);
    uint8_t dutyIndex = channel->frequencyCycles / (channel->frequencyCyclesInPeriod / 8);
    int16_t value = DUTY_CYCLES[dutyNumber][dutyIndex];
    return value * (channel->volume / 15.0);
  }

  return sample;
}
