#include "mbc3.h"

#include "../battery.h"
#include "../logging.h"
#include "../memory.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_HOUR (60 * SECONDS_IN_MINUTE)
#define SECONDS_IN_DAY (24 * SECONDS_IN_HOUR)

#define SIZEOF_MEMBER(TYPE, MEMBER) sizeof(((TYPE*)0)->MEMBER)

#define DAY_HIGH_HALT_BIT_SELECT (1 << 6)

// The number of CPU clock cycles that need to occur to trigger an increment to the seconds value
// Yes, this is NOT 32,768, the frequency of the oscillator in the real MBC3 RTC
// We're using CPU cycles to increment the RTC which are clocked at a much higher frequency
#define RTC_TICK_FREQUENCY (4 * 1024 * 1024)

typedef struct {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t dayLow;
  uint8_t dayHigh;
} RTC;

typedef struct {
  uint8_t* externalRAM;
  uint32_t externalRAMSize;
  bool timer;
  bool ramAndTimerEnabled;

  uint8_t romBank;
  uint8_t ramBankOrRTCRegister;
  uint8_t latch;

  RTC rtc;
  RTC _rtc;

  uint32_t cycles;
  time_t lastSaveTime;

  FILE* batteryFile;
} MBC3;

#define RTC_SAVE_SIZE \
  SIZEOF_MEMBER(MBC3, rtc) + \
  SIZEOF_MEMBER(MBC3, _rtc) + \
  SIZEOF_MEMBER(MBC3, cycles) + \
  sizeof(uint64_t)

static void mbc3SaveBufferRead(MBC3* mbc3, uint8_t* saveBuffer)
{
  int i = 0;

  if (mbc3->externalRAM != NULL) {
    for (; i < mbc3->externalRAMSize; i++) {
      mbc3->externalRAM[i] = saveBuffer[i];
    }
  }

  if (mbc3->timer) {
    mbc3->rtc.seconds  = saveBuffer[i++];
    mbc3->rtc.minutes  = saveBuffer[i++];
    mbc3->rtc.hours    = saveBuffer[i++];
    mbc3->rtc.dayLow   = saveBuffer[i++];
    mbc3->rtc.dayHigh  = saveBuffer[i++];
    mbc3->_rtc.seconds = saveBuffer[i++];
    mbc3->_rtc.minutes = saveBuffer[i++];
    mbc3->_rtc.hours   = saveBuffer[i++];
    mbc3->_rtc.dayLow  = saveBuffer[i++];
    mbc3->_rtc.dayHigh = saveBuffer[i++];

    mbc3->cycles = 0; // This MUST be zeroed because the following loop relies on constly updating the existing value which on the first iteration could be anything
    for (int byte = 0; byte < sizeof(mbc3->cycles); byte++) {
      mbc3->cycles = (saveBuffer[i++] << (byte * 8)) | mbc3->cycles;
    }

    mbc3->lastSaveTime = (time_t)0; // This MUST be zeroed because the following loop relies on constly updating the existing value which on the first iteration could be anything
    for (int byte = 0; byte < sizeof(uint64_t); byte++) {
      uint8_t saveByte = saveBuffer[i++];
      mbc3->lastSaveTime = (time_t)((saveByte << (byte * 8)) | ((uint64_t)mbc3->lastSaveTime));
    }
  }
}

static void mbc3SaveBufferWrite(MBC3* mbc3, uint8_t* saveBuffer)
{
  mbc3->lastSaveTime = time(NULL);

  int i = 0;

  if (mbc3->externalRAM != NULL) {
    for (; i < mbc3->externalRAMSize; i++) {
      saveBuffer[i] = mbc3->externalRAM[i];
    }
  }

  if (mbc3->timer) {
    saveBuffer[i++] = mbc3->rtc.seconds;
    saveBuffer[i++] = mbc3->rtc.minutes;
    saveBuffer[i++] = mbc3->rtc.hours;
    saveBuffer[i++] = mbc3->rtc.dayLow;
    saveBuffer[i++] = mbc3->rtc.dayHigh;
    saveBuffer[i++] = mbc3->_rtc.seconds;
    saveBuffer[i++] = mbc3->_rtc.minutes;
    saveBuffer[i++] = mbc3->_rtc.hours;
    saveBuffer[i++] = mbc3->_rtc.dayLow;
    saveBuffer[i++] = mbc3->_rtc.dayHigh;

    for (int byte = 0; byte < sizeof(mbc3->cycles); byte++) {
      saveBuffer[i++] = (mbc3->cycles >> (byte * 8)) & 0xFF;
    }

    for (int byte = 0; byte < sizeof(uint64_t); byte++) {
      saveBuffer[i++] = (((uint64_t)mbc3->lastSaveTime) >> (byte * 8)) & 0xFF;
    }
  }
}

static void mbc3UpdateLastSaveTime(MBC3* mbc3)
{
  // The internal RTC values are updated based on clock cycles and NOT real time so that emulation
  // speed increases also apply to the RTC, however we need to use time() when we're updating
  // the internal value for "last update time".
  // Why? If we run the emulator at 4x speed for 1 minute then we're now 3 minutes ahead of real time.
  // If we instantaneously reload a save then there's a negative time difference of 3 minutes to
  // "forward" fast the internal clock by, which means what? Do we rewind the internal clock?
  // Probably not, because the next latch could reverse a game clock - a massive WTF.
  // Alternatively, we could ignore the negative time difference and assume that no time has passed.
  // For a larger time value though, for example 15 minutes of real time for an hour of game
  // time, this puts us 45 minutes ahead. If the save is reloaded 30 minutes later, we're still
  // 15 minutes behind. If we ignore the time difference then 30 minutes of real time have passed
  // but the game time won't have advanced (and if we keep the new time then we're now permanently 15 minutes out)
  // Really, 30 minutes of real time out-of-game should translate to 30 minutes of time difference
  // in-game, as if the clock was ticking and the emulator was running at 1x speed.
  // So, we store the value of time() and always use this to forward-fast the RTC.
  mbc3->lastSaveTime = time(NULL);
}

static void mbc3SaveRTC(MBC3* mbc3)
{
  uint16_t saveAddress = mbc3->externalRAMSize;

  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->rtc.seconds);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->rtc.minutes);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->rtc.hours);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->rtc.dayLow);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->rtc.dayHigh);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->_rtc.seconds);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->_rtc.minutes);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->_rtc.hours);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->_rtc.dayLow);
  batteryFileWriteByte(mbc3->batteryFile, saveAddress++, mbc3->_rtc.dayHigh);

  for (int byte = 0; byte < sizeof(mbc3->cycles); byte++) {
    batteryFileWriteByte(mbc3->batteryFile, saveAddress++, (mbc3->cycles >> (byte * 8)) & 0xFF);
  }

  for (int byte = 0; byte < sizeof(uint64_t); byte++) {
    batteryFileWriteByte(mbc3->batteryFile, saveAddress++, (((uint64_t)mbc3->lastSaveTime) >> (byte * 8)) & 0xFF);
  }
}

uint8_t mbc3ReadByte(MemoryController* memoryController, uint16_t address)
{
  MBC3* mbc3 = (MBC3*)memoryController->mbc;

  if (address <= 0x3FFF) { // Read from ROM Bank 0
    return memoryController->cartridge[address];
  } else if (address >= 0x4000 && address <= 0x7FFF) { // Read from ROM Banks 01-7F
    uint32_t romAddress = (mbc3->romBank * 16 * 1024) + (address - 0x4000);
    return memoryController->cartridge[romAddress];
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Read from external cartridge RAM/RTC registers
    if (mbc3->ramAndTimerEnabled) {
      if (mbc3->ramBankOrRTCRegister <= 0x03) {
        uint16_t ramAddress = (mbc3->ramBankOrRTCRegister * 8 * 1024) + (address - 0xA000);
        assert(ramAddress < mbc3->externalRAMSize);
        return mbc3->externalRAM[ramAddress];
      } else if (mbc3->ramBankOrRTCRegister >= 0x08 && mbc3->ramBankOrRTCRegister <= 0x0C) {
        switch (mbc3->ramBankOrRTCRegister) {
          case 0x08:
            return mbc3->rtc.seconds;
            break;
          case 0x09:
            return mbc3->rtc.minutes;
            break;
          case 0x0A:
            return mbc3->rtc.hours;
            break;
          case 0x0B:
            return mbc3->rtc.dayLow;
            break;
          case 0x0C:
            return mbc3->rtc.dayHigh;
            break;
          default:
            warning("MBC3: Unhandled value 0x%02X for RTC register selection\n", mbc3->ramBankOrRTCRegister);
            break;
        }
        return 0;
      } else {
        warning("MBC3: Unhandled value 0x%02X for RAM bank/RTC register selection\n", mbc3->ramBankOrRTCRegister);
        return 0; // TODO: What value should be returned here?
      }
    } else {
      warning("MBC3: Read from external RAM at address 0x%04X failed because RAM is DISABLED.\n", address);
      return 0; // TODO: What value should be returned here?
    }
  } else {
    return commonReadByte(memoryController, address);
  }
}

void mbc3WriteByte(MemoryController* memoryController, uint16_t address, uint8_t value)
{
  MBC3* mbc3 = (MBC3*)memoryController->mbc;

  if (address <= 0x1FFF) { // External RAM Enable/Disable
    if ((value & 0xF) == 0xA) {
      if (!mbc3->ramAndTimerEnabled) {
        // info("External RAM and RTC was ENABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc3->ramAndTimerEnabled = true;
    } else {
      if (mbc3->ramAndTimerEnabled) {
        // info("External RAM and RTC was DISABLED by value 0x%02X written to address 0x%04X\n", value, address);
      }
      mbc3->ramAndTimerEnabled = false;
    }
  } else if (address >= 0x2000 && address <= 0x3FFF) { // ROM Bank Number
    if (value == 0) {
      value = 1;
    }
    mbc3->romBank = value & 0x7F; // Select the lower 7 bits of the value
  } else if (address >= 0x4000 && address <= 0x5FFF) { // RAM Bank Number or Upper Bits of ROM Bank Number
    mbc3->ramBankOrRTCRegister = value;
  } else if (address >= 0x6000 && address <= 0x7FFF) { // Latch Clock Data
    if (mbc3->latch == 0 && value == 1) {
      mbc3->rtc = mbc3->_rtc;
    }
    mbc3->latch = value;
  } else if (address >= 0xA000 && address <= 0xBFFF) { // Write to external cartridge RAM/RTC registers
    if (mbc3->ramAndTimerEnabled) {
      if (mbc3->ramAndTimerEnabled) {
        if (mbc3->ramBankOrRTCRegister <= 0x03) {
          uint16_t ramAddress = (mbc3->ramBankOrRTCRegister * 8 * 1024) + (address - 0xA000);
          assert(ramAddress < mbc3->externalRAMSize);
          mbc3->externalRAM[ramAddress] = value;
          if (mbc3->batteryFile != NULL) {
            batteryFileWriteByte(mbc3->batteryFile, ramAddress, value);
          }
        } else if (mbc3->ramBankOrRTCRegister >= 0x08 && mbc3->ramBankOrRTCRegister <= 0x0C) {
          switch (mbc3->ramBankOrRTCRegister) {
            case 0x08:
              if (value < 60) {
                mbc3->rtc.seconds = value;
                mbc3->_rtc.seconds = value;
              }
              break;
            case 0x09:
              if (value < 60) {
                mbc3->rtc.minutes = value;
                mbc3->_rtc.minutes = value;
              }
              break;
            case 0x0A:
              if (value < 24) {
                mbc3->rtc.hours = value;
                mbc3->_rtc.hours = value;
              }
              break;
            case 0x0B:
              mbc3->rtc.dayLow = value;
              mbc3->_rtc.dayLow = value;
              break;
            case 0x0C:
              mbc3->rtc.dayHigh = value;
              mbc3->_rtc.dayHigh = value;
              break;
            default:
              warning("MBC3: Unhandled value 0x%02X for RTC register selection\n", mbc3->ramBankOrRTCRegister);
              break;
          }
          mbc3UpdateLastSaveTime(mbc3);
          mbc3SaveRTC(mbc3);
        } else {
          warning("MBC3: Unhandled value 0x%02X for RAM bank/RTC register selection\n", mbc3->ramBankOrRTCRegister);
        }
      } else {
        warning("MBC3: Read from external RAM at address 0x%04X failed because RAM is DISABLED.\n", address);
      }
    } else {
      warning("MBC3: Write of value 0x%02X to external RAM at address 0x%04X failed because RAM is DISABLED.\n", value, address);
    }
  } else {
    commonWriteByte(memoryController, address, value);
  }
}

static void mbc3IncrementDays(MBC3* mbc3)
{
  uint16_t dayCounterBefore = ((mbc3->_rtc.dayHigh & 1) << 8) | mbc3->_rtc.dayLow;
  uint16_t dayCounterAfter = (dayCounterBefore + 1) % 512;

  uint8_t dayCounterMostSignificantBit = (dayCounterAfter & 0x100) >> 8;
  uint8_t dayCounterCarry = (dayCounterAfter == 0) << 7;

  mbc3->_rtc.dayLow = (dayCounterAfter & 0xFF);
  mbc3->_rtc.dayHigh = mbc3->_rtc.dayHigh | dayCounterCarry | dayCounterMostSignificantBit;
}

static void mbc3IncrementHours(MBC3* mbc3)
{
  mbc3->_rtc.hours = (mbc3->_rtc.hours + 1) % 24;
  if (mbc3->_rtc.hours == 0) {
    mbc3IncrementDays(mbc3);
  }
}

static void mbc3IncrementMinutes(MBC3* mbc3)
{
  mbc3->_rtc.minutes = (mbc3->_rtc.minutes + 1) % 60;
  if (mbc3->_rtc.minutes == 0) {
    mbc3IncrementHours(mbc3);
  }
}

static void mbc3IncrementSeconds(MBC3* mbc3)
{
  mbc3->_rtc.seconds = (mbc3->_rtc.seconds + 1) % 60;
  if (mbc3->_rtc.seconds == 0) {
    mbc3IncrementMinutes(mbc3);
  }
}

static void mbc3CartridgeUpdate(MemoryController* memoryController, uint32_t cyclesExecuted)
{
  MBC3* mbc3 = (MBC3*)memoryController->mbc;

  const int cyclesPerSecond = RTC_TICK_FREQUENCY;

  if (mbc3->timer) {
    if (((mbc3->rtc.dayHigh & DAY_HIGH_HALT_BIT_SELECT) == 0) && ((mbc3->cycles + cyclesExecuted) >= cyclesPerSecond)) {
      mbc3IncrementSeconds(mbc3);

      if (mbc3->batteryFile != NULL) {
        mbc3UpdateLastSaveTime(mbc3);
        mbc3SaveRTC(mbc3);
      }
    }

    mbc3->cycles = (mbc3->cycles + cyclesExecuted) % cyclesPerSecond;
    // It would be good to save the RTC state at this point so that we could serialise the updated
    // value of the internal clock cycles counter, however with the frequency that this method is
    // called this seriously affects the emulation speed, and as long as mbc3FinaliseMemoryController()
    // is called we still have a point where we serialise the most up-to-date value of the entire timer state.
  }
}

static void mbc3FastForwardRTC(MemoryController* memoryController, MBC3* mbc3, time_t now)
{
  // RTC would only have been ticking if enabled
  if ((mbc3->rtc.dayHigh & DAY_HIGH_HALT_BIT_SELECT) == 0) {
    uint64_t remainingSeconds = (uint64_t)floor(difftime(now, mbc3->lastSaveTime));

    debug("\b[MBC3] Fast-forwarding MBC3 RTC by %llu seconds\n", remainingSeconds);

    // Update days
    int days = (remainingSeconds / SECONDS_IN_DAY);
    for (int i = 0; i < days; i++) {
      mbc3IncrementDays(mbc3);
    }
    remainingSeconds %= SECONDS_IN_DAY;

    // Update hours
    int hours = (remainingSeconds / SECONDS_IN_HOUR);
    for (int i = 0; i < hours; i++) {
      mbc3IncrementHours(mbc3);
    }
    remainingSeconds %= SECONDS_IN_HOUR;

    // Update minutes
    int minutes = (remainingSeconds / SECONDS_IN_MINUTE);
    for (int i = 0; i < minutes; i++) {
      mbc3IncrementMinutes(mbc3);
    }
    remainingSeconds %= SECONDS_IN_MINUTE;

    // Update seconds
    for (int i = 0; i < remainingSeconds; i++) {
      mbc3IncrementSeconds(mbc3);
    }
  }

  // We don't need to check for a battery here because there's no reason to be fast-forwarding RTC values if there isn't one
  mbc3UpdateLastSaveTime(mbc3);
  mbc3SaveRTC(mbc3);
}

void mbc3InitialiseMemoryController(
  MemoryController* memoryController,
  uint32_t externalRAMSizeBytes,
  const char* romFilename,
  bool ram,
  bool timer,
  bool battery
)
{
  MBC3* mbc3 = (MBC3*)malloc(sizeof(MBC3));
  assert(mbc3);

  mbc3->externalRAM = NULL;
  mbc3->externalRAMSize = externalRAMSizeBytes;
  mbc3->timer = timer;
  mbc3->ramAndTimerEnabled = false;
  mbc3->romBank = 1; // Initialise to 1 because writes of 0 are translated to a 1
  mbc3->ramBankOrRTCRegister = 0;
  mbc3->latch = 1; // Start on a non-zero value to ensure that the 00h->01h write pattern is required even the first time

  mbc3->rtc.seconds = 0;
  mbc3->rtc.minutes = 0;
  mbc3->rtc.hours = 0;
  mbc3->rtc.dayLow = 0;
  mbc3->rtc.dayHigh = 0;

  mbc3->_rtc.seconds = 0;
  mbc3->_rtc.minutes = 0;
  mbc3->_rtc.hours = 0;
  mbc3->_rtc.dayLow = 0;
  mbc3->_rtc.dayHigh = 0;

  mbc3->cycles = 0;
  mbc3->batteryFile = NULL;

  time_t now = time(NULL);
  mbc3->lastSaveTime = now;

  if (ram) {
    mbc3->externalRAM = (uint8_t*)malloc(externalRAMSizeBytes * sizeof(uint8_t));
    assert(mbc3->externalRAM);
  }

  if (!ram && externalRAMSizeBytes > 0) {
    warning("\b[MBC3] Cartridge declared %u bytes of external RAM but cartridge type did not indicate RAM.\n", externalRAMSizeBytes);
  } else if (ram && externalRAMSizeBytes == 0) {
    warning("\b[MBC3] Cartridge type indicated RAM but cartridge declared %u bytes of external RAM.\n", externalRAMSizeBytes);
  }

  if (battery) {
    // To do the initial load/save of external RAM to the battery file we need to prepare a single
    // buffer that can fit external RAM

    // MBC3 can support battery-backed persistence for either RAM, RTC, or both, so determine the correct save buffer size
    uint32_t saveBufferSize = (externalRAMSizeBytes * sizeof(uint8_t)) + ((timer) ? RTC_SAVE_SIZE : 0);

    uint8_t* saveBuffer = (uint8_t*)malloc(saveBufferSize);
    assert(saveBuffer);

    // Put stuff into save buffer, so that batteryFileOpen can create an appropriately sized save file if it doesn't exist yet
    mbc3SaveBufferWrite(mbc3, saveBuffer);

    mbc3->batteryFile = batteryFileOpen(romFilename, saveBuffer, saveBufferSize);

    // Get stuff from save buffer (in case batteryFileOpen didn't write the file in the first place)
    // If a battery save was loaded, we're updating the current MBC3 state from that save
    // If we just created a new save file then we're simply (but unnecessarily) reloading the same state values as before, so there's no problem
    mbc3SaveBufferRead(mbc3, saveBuffer);

    free(saveBuffer);
  }

  memoryController->readByteImpl = &mbc3ReadByte;
  memoryController->writeByteImpl = &mbc3WriteByte;
  memoryController->cartridgeUpdateImpl = &mbc3CartridgeUpdate;
  memoryController->mbc = mbc3;

  // Fast-forward time
  // This must be done after battery-backed data is restored so we get the real time difference between now and the RTC data
  // It must ALSO be done AFTER we set the "mbc" pointer of the memoryController struct to the dynamic
  // MBC3 struct, because we may end up calling mbc3CartridgeUpdate() (also used by the main run loop,
  // so there's no direct access to the struct without knowing what type it is)
  if (timer && battery) {
    mbc3FastForwardRTC(memoryController, mbc3, now);
  }
}

void mbc3FinaliseMemoryController(MemoryController* memoryController)
{
  MBC3* mbc3 = (MBC3*)memoryController->mbc;

  mbc3UpdateLastSaveTime(mbc3);
  mbc3SaveRTC(mbc3);

  if (mbc3->batteryFile != NULL) {
    fclose(mbc3->batteryFile);
  }

  if (mbc3->externalRAM != NULL) {
    free(mbc3->externalRAM);
  }

  free(memoryController->mbc);
}