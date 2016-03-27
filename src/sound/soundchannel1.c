#include "soundchannel1.h"

#include "../logging.h"

#include <math.h>

// #define LOG_IO_WRITES_TO_NR10
// #define LOG_IO_WRITES_TO_NR11
// #define LOG_IO_WRITES_TO_NR12
// #define LOG_IO_WRITES_TO_NR13
// #define LOG_IO_WRITES_TO_NR14
// #define LOG_IO_WRITES_TO_ALL
// #define LOG_CHANNEL_TRIGGERS
// #define LOG_ZOMBIE

extern int16_t DUTY_CYCLES[][8];

void initSoundChannel1(SoundChannel1* channel, uint8_t* nr52)
{
  soundChannel1Reset(channel);

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

  channel->frequencyShadow = 0;
  channel->sweepActive = false;
  channel->sweepIncrease = false;
  channel->sweepPeriod = 0;
  channel->sweepRemaining = 0;

  channel->zombie = false;
}


static void channelOn(SoundChannel1* channel)
{
  channel->on = true;
  (*channel->nr52) |= (1 << 0);
}


static void channelOff(SoundChannel1* channel)
{
  channel->on = false;
  (*channel->nr52) &= ~(1 << 1);
}


static void loadLengthCounter(SoundChannel1* channel)
{
  channel->length = 64 - (channel->nr11 & 63);
  channel->counterSelection = (channel->nr14 & (1 << 6));
}


static uint32_t frequencyToCycles(uint16_t frequency)
{
  uint32_t frequencyInHz = 131072 / (2048 - frequency);
  return (1024 * 1024 * 4) / frequencyInHz;
}


static void loadFrequency(SoundChannel1* channel)
{
  channel->frequency = ((channel->nr14 & 7) << 8) | channel->nr13;
  channel->frequencyCyclesInPeriod = frequencyToCycles(channel->frequency);

  // TODO: What's the correct way to handle the current cycle count? For now, if there are more cycles per period
  // than our current value then simply continue counting from our current position, only reseting the counter
  // if the frequency has increased
  if (channel->frequencyCyclesInPeriod < channel->frequencyCycles) {
    channel->frequencyCycles = channel->frequencyCyclesInPeriod;
  }
}


static void loadVolumeAndEnvelope(SoundChannel1* channel)
{
  channel->volume = (channel->nr12 >> 4);
  channel->envelopeIncrease = (channel->nr12 & (1 << 3));
  channel->envelopeSweepPeriod = (channel->nr12 & 7);
  channel->envelopeSweepRemaining = channel->envelopeSweepPeriod;
  channel->envelopeActive = (channel->envelopeSweepPeriod != 0);
}


static void loadFrequencySweep(SoundChannel1* channel)
{
  channel->frequencyShadow = channel->frequency;
  channel->sweepIncrease = ((channel->nr10 & (1 << 3)) == 0);
  channel->sweepPeriod = (channel->nr10 >> 4);
  channel->sweepRemaining = channel->sweepPeriod;
  channel->sweepShift = (channel->nr10 & 7);
  channel->sweepActive = (channel->sweepPeriod != 0 || channel->sweepShift != 0);
}


static void enableZombie(SoundChannel1* channel)
{
#ifdef LOG_ZOMBIE
  debug("\b[SCH1] Entering zombie mode\n");
#endif

  channel->zombie = true;
}


static void disableZombie(SoundChannel1* channel)
{
#ifdef LOG_ZOMBIE
  debug("\b[SCH1] Exiting zombie mode\n");
#endif

  channel->zombie = false;
}


static bool isZombie(SoundChannel1* channel)
{
  return channel->zombie;
}


static uint8_t readNR10(SoundChannel1* channel)
{
  return channel->nr10 | 0x80;
}


static uint8_t readNR11(SoundChannel1* channel)
{
  return channel->nr11 | 0x3F;
}


static uint8_t readNR12(SoundChannel1* channel)
{
  return channel->nr12 | 0x00;
}


static uint8_t readNR13(SoundChannel1* channel)
{
  return channel->nr13 | 0xFF;
}


static uint8_t readNR14(SoundChannel1* channel)
{
  return channel->nr14 | 0xBF;
}


uint8_t soundChannel1ReadByte(SoundChannel1* channel, uint16_t address)
{
  if (address == IO_REG_ADDRESS_NR10) {
    return readNR10(channel);
  } else if (address == IO_REG_ADDRESS_NR11) {
    return readNR11(channel);
  } else if (address == IO_REG_ADDRESS_NR12) {
    return readNR12(channel);
  } else if (address == IO_REG_ADDRESS_NR13) {
    return readNR13(channel);
  } else if (address == IO_REG_ADDRESS_NR14) {
    return readNR14(channel);
  } else {
    return 0x00; // TODO: Warning message
  }
}


static void writeNR10(SoundChannel1* channel, uint8_t value)
{
  channel->nr10 = value;

#if defined(LOG_IO_WRITES_TO_NR10) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH1][NR10][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR11(SoundChannel1* channel, uint8_t value)
{
  channel->nr11 = value;
  loadLengthCounter(channel); // TODO: Should this only occur for writes to the length counter bits (and not the duty cycle bits too)?

#if defined(LOG_IO_WRITES_TO_NR11) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH1][NR11][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR12(SoundChannel1* channel, uint8_t value)
{
  channel->nr12 = value;

  if (isZombie(channel)) {
    if ((value >> 4) == 0) {
      channelOff(channel);
    }
  }

#if defined(LOG_IO_WRITES_TO_NR12) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH1][NR12][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR13(SoundChannel1* channel, uint8_t value)
{
  uint8_t currentFrequencyBits = channel->nr13;
  channel->nr13 = value;
  if (currentFrequencyBits != value) {
    loadFrequency(channel);
  }

#if defined(LOG_IO_WRITES_TO_NR13) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH1][NR13][WRITE] 0x%02X\n", value);
#endif
}


static void writeNR14(SoundChannel1* channel, uint8_t value)
{
  uint8_t currentFrequencyBits = (channel->nr14 & 7);

  channel->nr14 = value;

  if (value & (1 << 7)) {
    soundChannel1Trigger(channel); // Channel trigger will also reload the frequency, so no need to do it twice
  } else if (currentFrequencyBits != (value & 7)) {
    loadFrequency(channel);
  }

#if defined(LOG_IO_WRITES_TO_NR14) || defined(LOG_IO_WRITES_TO_ALL)
  debug("\b[SCH1][NR14][WRITE] 0x%02X\n", value);
#endif
}


void soundChannel1WriteByte(SoundChannel1* channel, uint16_t address, uint8_t value)
{
  if (address == IO_REG_ADDRESS_NR10) {
    writeNR10(channel, value);
  } else if (address == IO_REG_ADDRESS_NR11) {
    writeNR11(channel, value);
  } else if (address == IO_REG_ADDRESS_NR12) {
    writeNR12(channel, value);
  } else if (address == IO_REG_ADDRESS_NR13) {
    writeNR13(channel, value);
  } else if (address == IO_REG_ADDRESS_NR14) {
    writeNR14(channel, value);
  } else {
    // TODO: Warning message
  }
}


void soundChannel1Reset(SoundChannel1* channel)
{
  // No change to NR13
  channel->nr10 = 0x80;
  channel->nr11 = 0xBF;
  channel->nr12 = 0xF3;
  channel->nr14 = 0xBF;
}


static uint16_t soundChannel1UpdatedFrequency(SoundChannel1* channel)
{
  int16_t change = channel->frequencyShadow >> channel->sweepShift;
  if (!channel->sweepIncrease) {
    change = -change;
  }
  return channel->frequencyShadow + change;
}


void soundChannel1Trigger(SoundChannel1* channel)
{
  channelOn(channel);

  loadLengthCounter(channel);
  loadFrequency(channel);
  loadVolumeAndEnvelope(channel);

  loadFrequencySweep(channel);

  if (channel->sweepActive) {
    uint16_t newFrequency = soundChannel1UpdatedFrequency(channel);
    if (newFrequency > 2047 || channel->sweepShift == 0) {
      channelOff(channel);
    }
  }

  disableZombie(channel);

#ifdef LOG_CHANNEL_TRIGGERS
  debug("\b[SCH1][TRIGGER] NR10=0x%02X NR11=0x%02X NR12=0x%02X NR13=0x%02X NR14=0x%02X\n", channel->nr10, channel->nr11, channel->nr12, channel->nr13, channel->nr14);
#endif
}


void soundChannel1Update(SoundChannel1* channel, uint8_t cyclesExecuted)
{
  // uint16_t frequency = ((channel->nr14 & 7) << 8) | channel->nr13;
  // channel->frequencyCyclesInPeriod = frequencyToCycles(frequency);

  channel->frequencyCycles = (channel->frequencyCycles + cyclesExecuted) % channel->frequencyCyclesInPeriod;
}


void soundChannel1ClockLength(SoundChannel1* channel)
{
  if (!channel->on || !channel->counterSelection) {
    return;
  }

  channel->length--;

  if (channel->length == 0) {
    channelOff(channel);
  }
}


void soundChannel1ClockVolume(SoundChannel1* channel)
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


void soundChannel1ClockSweep(SoundChannel1* channel)
{
  if (!channel->sweepActive || channel->sweepPeriod == 0) {
    return;
  }

  channel->sweepRemaining--;

  if (channel->sweepRemaining > 0) {
    return;
  }

  uint16_t newFrequency = soundChannel1UpdatedFrequency(channel);
  if (newFrequency <= 2047 && channel->sweepShift != 0) {
    channel->nr13 = (newFrequency & 255);
    channel->nr14 = (channel->nr14 & 192) | ((newFrequency & 1792) >> 8); // Update lowest 3 bits of NR14
    channel->frequencyShadow = newFrequency;
    channel->frequencyCyclesInPeriod = frequencyToCycles(newFrequency);

    uint16_t newNewFrequency = soundChannel1UpdatedFrequency(channel);
    if (newNewFrequency > 2047) {
      channelOff(channel);
    }
  } else {
    channelOff(channel);
  }

  channel->sweepRemaining = channel->sweepPeriod;
}


int16_t soundChannel1GetCurrentSample(SoundChannel1* channel)
{
  int16_t sample = 0;

  if (channel->on) {
    uint8_t dutyNumber = (channel->nr11 >> 6);
    uint8_t dutyIndex = channel->frequencyCycles / (channel->frequencyCyclesInPeriod / 8);
    int16_t value = DUTY_CYCLES[dutyNumber][dutyIndex];
    sample = value * (channel->volume / 15.0);
  }

  return sample;
}