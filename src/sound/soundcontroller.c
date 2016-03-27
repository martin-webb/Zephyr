#include "soundcontroller.h"

void initSoundController(SoundController* soundController)
{
  soundControllerReset(soundController);

  soundController->frameSequencerStep = 0;
  soundController->frameSequencerCycles = 0;

  // Pass down a pointer to the NR52 register to allow each channel to update its status bit
  initSoundChannel1(&soundController->channel1, &soundController->nr52);
  initSoundChannel2(&soundController->channel2, &soundController->nr52);
  initSoundChannel3(&soundController->channel3, &soundController->nr52);
  initSoundChannel4(&soundController->channel4, &soundController->nr52);

  soundController->channel1Master = true;
  soundController->channel2Master = true;
  soundController->channel3Master = true;
  soundController->channel4Master = true;
}


void soundControllerReset(SoundController* soundController)
{
  soundController->nr50 = 0x77;
  soundController->nr51 = 0xF3;
  soundController->nr52 = 0xFF; // TODO: What should this be?
}


void soundOn(SoundController* soundController)
{
  soundController->nr50 |= (1 << 7);
}


void soundOff(SoundController* soundController)
{
  soundController->nr50 &= ~(1 << 7);
}


static void frameSequencerUpdate(SoundController* soundController, uint8_t cyclesExecuted)
{
  if (soundController->frameSequencerCycles + cyclesExecuted >= 8192) {
    soundController->frameSequencerStep = (soundController->frameSequencerStep + 1) % 8;
    switch (soundController->frameSequencerStep) {
      case 0: // Length counter clock
        soundChannel1ClockLength(&soundController->channel1);
        soundChannel2ClockLength(&soundController->channel2);
        soundChannel3ClockLength(&soundController->channel3);
        soundChannel4ClockLength(&soundController->channel4);
        break;
      case 2: // Length counter clock and sweep clock
        soundChannel1ClockLength(&soundController->channel1);
        soundChannel2ClockLength(&soundController->channel2);
        soundChannel3ClockLength(&soundController->channel3);
        soundChannel4ClockLength(&soundController->channel4);
        soundChannel1ClockSweep(&soundController->channel1);
        break;
      case 4: // Length counter clock
        soundChannel1ClockLength(&soundController->channel1);
        soundChannel2ClockLength(&soundController->channel2);
        soundChannel3ClockLength(&soundController->channel3);
        soundChannel4ClockLength(&soundController->channel4);
        break;
      case 6: // Length counter clock and sweep clock
        soundChannel1ClockLength(&soundController->channel1);
        soundChannel2ClockLength(&soundController->channel2);
        soundChannel3ClockLength(&soundController->channel3);
        soundChannel4ClockLength(&soundController->channel4);
        soundChannel1ClockSweep(&soundController->channel1);
        break;
      case 7: // Volume envelope clock
        soundChannel1ClockVolume(&soundController->channel1);
        soundChannel2ClockVolume(&soundController->channel2);
        soundChannel4ClockVolume(&soundController->channel4);
        break;
    }
  }
  soundController->frameSequencerCycles = (soundController->frameSequencerCycles + cyclesExecuted) % 8192;
}


static void updateChannel1Status(SoundController* soundController)
{
  if (soundController->channel1.on) {
    soundController->nr52 |= (1 << 0);
  } else {
    soundController->nr52 &= ~(1 << 0);
  }
}


static void updateChannel2Status(SoundController* soundController)
{
  if (soundController->channel2.on) {
    soundController->nr52 |= (1 << 1);
  } else {
    soundController->nr52 &= ~(1 << 1);
  }
}


static void updateChannel3Status(SoundController* soundController)
{
  if (soundController->channel3.nr30 & (1 << 7)) {
    soundController->nr52 |= (1 << 2);
  } else {
    soundController->nr52 &= ~(1 << 2);
  }
}


static void updateChannel4Status(SoundController* soundController)
{
  if (soundController->channel4.on) {
    soundController->nr52 |= (1 << 4);
  } else {
    soundController->nr52 &= ~(1 << 4);
  }
}


static uint8_t readNR50(SoundController* soundController)
{
  return soundController->nr50 | 0x00;
}


static uint8_t readNR51(SoundController* soundController)
{
  return soundController->nr51 | 0x00;
}


static uint8_t readNR52(SoundController* soundController)
{
  return soundController->nr52 | 0x70;
}


uint8_t soundControllerReadByte(SoundController* soundController, uint16_t address)
{
  if ((soundController->nr52 & (1 << 7)) == 0 && address != IO_REG_ADDRESS_NR52) {
    return 0x00;
  }

  if (address >= IO_REG_ADDRESS_NR10 && address <= IO_REG_ADDRESS_NR14) {
    return soundChannel1ReadByte(&soundController->channel1, address);
  } else if (address >= IO_REG_ADDRESS_NR20 && address <= IO_REG_ADDRESS_NR24) {
    return soundChannel2ReadByte(&soundController->channel2, address);
  } else if ((address >= IO_REG_ADDRESS_NR30 && address <= IO_REG_ADDRESS_NR34) ||
             (address >= IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN && address <= IO_REG_ADDRESS_WAVE_PATTERN_RAM_END)) {
    return soundChannel3ReadByte(&soundController->channel3, address);
  } else if (address >= IO_REG_ADDRESS_NR41 && address <= IO_REG_ADDRESS_NR44) {
    return soundChannel4ReadByte(&soundController->channel4, address);
  } else if (address >= 0xFF27 && address <= 0xFF2F) {
    return 0xFF;
  } else if (address == IO_REG_ADDRESS_NR50) {
    return readNR50(soundController);
  } else if (address == IO_REG_ADDRESS_NR51) {
    return readNR51(soundController);
  } else if (address == IO_REG_ADDRESS_NR52) {
    return readNR52(soundController);
  } else {
    // warning("Read from unhandled I/O register address 0x%04X (in sound controller range 0x%04X-0x%04X)\n", value, address, IO_REG_ADDRESS_NR10, IO_REG_ADDRESS_NR52);
    return 0x00;
  }
}


static void writeNR50(SoundController* soundController, uint8_t value)
{
  soundController->nr50 = value;
}


static void writeNR51(SoundController* soundController, uint8_t value)
{
  soundController->nr51 = value;
}


static void writeNR52(SoundController* soundController, uint8_t value)
{
  soundController->nr52 = (value & 128) | (soundController->nr52 & 127);
  if (value & (1 << 7)) {
    soundOn(soundController);
  } else {
    soundOff(soundController);
    // TODO: Reset all sound channel registers on a full power off
  }
}


void soundControllerWriteByte(SoundController* soundController, uint16_t address, uint8_t value)
{
  if ((soundController->nr52 & (1 << 7)) == 0 && address != IO_REG_ADDRESS_NR52) {
    return;
  }

  if (address >= IO_REG_ADDRESS_NR10 && address <= IO_REG_ADDRESS_NR14) {
    soundChannel1WriteByte(&soundController->channel1, address, value);
  } else if (address >= IO_REG_ADDRESS_NR20 && address <= IO_REG_ADDRESS_NR24) {
    soundChannel2WriteByte(&soundController->channel2, address, value);
  } else if ((address >= IO_REG_ADDRESS_NR30 && address <= IO_REG_ADDRESS_NR34) ||
             (address >= IO_REG_ADDRESS_WAVE_PATTERN_RAM_BEGIN && address <= IO_REG_ADDRESS_WAVE_PATTERN_RAM_END)) {
    soundChannel3WriteByte(&soundController->channel3, address, value);
  } else if (address >= IO_REG_ADDRESS_NR40 && address <= IO_REG_ADDRESS_NR44) {
    soundChannel4WriteByte(&soundController->channel4, address, value);
  } else if (address >= 0xFF27 && address <= 0xFF2F) {
    // Nothing happens in this address range // TODO: verify this
  } else if (address == IO_REG_ADDRESS_NR50) {
    writeNR50(soundController, value);
  } else if (address == IO_REG_ADDRESS_NR51) {
    writeNR51(soundController, value);
  } else if (address == IO_REG_ADDRESS_NR52) {
    writeNR52(soundController, value);
  } else {
    // warning("Write of value 0x%02X to unhandled I/O register address 0x%04X (in sound controller range 0x%04X-0x%04X)\n", value, address, IO_REG_ADDRESS_NR10, IO_REG_ADDRESS_NR52);
  }
}


void soundUpdate(SoundController* soundController, uint8_t cyclesExecuted)
{
  frameSequencerUpdate(soundController, cyclesExecuted);

  soundChannel1Update(&soundController->channel1, cyclesExecuted);
  soundChannel2Update(&soundController->channel2, cyclesExecuted);
  soundChannel3Update(&soundController->channel3, cyclesExecuted);
  soundChannel4Update(&soundController->channel4, cyclesExecuted);

  // TODO: Improve how the sound channels communicate their status back to the sound controller
  updateChannel1Status(soundController);
  updateChannel2Status(soundController);
  updateChannel3Status(soundController);
  updateChannel4Status(soundController);
}


AudioSample soundGetCurrentSample(SoundController* soundController)
{
  AudioSample sample = {.so1 = 0, .so2 = 0};

  if (soundController->nr52 & (1 << 7)) {
    int16_t ch1 = (soundController->channel1Master ? soundChannel1GetCurrentSample(&soundController->channel1) / 4 : 0);
    int16_t ch2 = (soundController->channel2Master ? soundChannel2GetCurrentSample(&soundController->channel2) / 4 : 0);
    int16_t ch3 = (soundController->channel3Master ? soundChannel3GetCurrentSample(&soundController->channel3) / 4 : 0);
    int16_t ch4 = (soundController->channel4Master ? soundChannel4GetCurrentSample(&soundController->channel4) / 4 : 0);

    float ch1F = ch1 / ((float)INT16_MAX);
    float ch2F = ch2 / ((float)INT16_MAX);
    float ch3F = ch3 / ((float)INT16_MAX);
    float ch4F = ch4 / ((float)INT16_MAX);

    ch1F *= 0.008;
    ch2F *= 0.008;
    ch3F *= 0.008;
    ch4F *= 0.008;

    const uint8_t nr51 = soundController->nr51;

    float so1 = (nr51 & (1 << 0) ? ch1F : 0) +
                (nr51 & (1 << 1) ? ch2F : 0) +
                (nr51 & (1 << 2) ? ch3F : 0) +
                (nr51 & (1 << 3) ? ch4F : 0);

    float so2 = (nr51 & (1 << 4) ? ch1F : 0) +
                (nr51 & (1 << 5) ? ch2F : 0) +
                (nr51 & (1 << 6) ? ch3F : 0) +
                (nr51 & (1 << 7) ? ch4F : 0);

    // Main output volume control
    so1 *= (((soundController->nr50 >> 0) & 7) / 7.0);
    so2 *= (((soundController->nr50 >> 4) & 7) / 7.0);

    // Hard limiting
    so1 *= 0.2;
    so2 *= 0.2;

    // Clipping
    if (so1 > 1.0f) so1 = 1.0f;
    if (so2 > 1.0f) so2 = 1.0f;
    if (so1 < -1.0f) so1 = -1.0f;
    if (so2 < -1.0f) so2 = -1.0f;

    sample.so1 = (int16_t)(so1 * INT16_MAX);
    sample.so2 = (int16_t)(so2 * INT16_MAX);
  }

  return sample;
}