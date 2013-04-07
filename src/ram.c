#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "ram.h"
#include "utils/os.h"

#define PATH_TO_BATTERY_RAM_DIR "/Library/Application Support/GBEmu1/Battery RAM/"
#define BATTERY_RAM_FILE_EXTENSION ".ram"

const char* ramSaveLocation(const char* romFilename)
{
  const char* pathToHomeDir = getenv("HOME");

  ExtSplitPathComponents romFilenameComponents = splitext(romFilename);

  size_t pathToHomeDirLength = strlen(pathToHomeDir);
  size_t pathToBatteryRAMDirLength = strlen(PATH_TO_BATTERY_RAM_DIR);
  size_t romFilenameNoExtLength = strlen(romFilenameComponents.left);
  size_t batteryRAMFileExtensionLength = strlen(BATTERY_RAM_FILE_EXTENSION);

  size_t pathToRAMFileLength = pathToHomeDirLength + pathToBatteryRAMDirLength + romFilenameNoExtLength + batteryRAMFileExtensionLength;

  char* pathToRAMFile = (char*)malloc((pathToRAMFileLength + 1) * sizeof(char));
  assert(pathToRAMFile);

  strncpy(&(pathToRAMFile[0]), pathToHomeDir, pathToHomeDirLength);
  strncpy(&(pathToRAMFile[pathToHomeDirLength]), PATH_TO_BATTERY_RAM_DIR, pathToBatteryRAMDirLength);
  strncpy(&(pathToRAMFile[pathToHomeDirLength + pathToBatteryRAMDirLength]), romFilenameComponents.left, romFilenameNoExtLength);
  strncpy(&(pathToRAMFile[pathToHomeDirLength + pathToBatteryRAMDirLength + romFilenameNoExtLength]), BATTERY_RAM_FILE_EXTENSION, batteryRAMFileExtensionLength);
  pathToRAMFile[pathToRAMFileLength] = '\0';

  free((void*)romFilenameComponents.left);
  free((void*)romFilenameComponents.right);

  return pathToRAMFile;
}

void ramSave(uint8_t* ram, uint32_t size, const char* romFilename)
{
  const char* pathToFile = ramSaveLocation(romFilename);

  const char* destinationDir = dirname(pathToFile);
  if (!exists(destinationDir)) {
    mkdirp(destinationDir);
  }
  free((void*)destinationDir);

  FILE* ramFile = fopen(pathToFile, "wb");
  assert(ramFile);
  size_t bytesWritten = fwrite((void*)ram, 1, size, ramFile);
  fclose(ramFile);
  if (bytesWritten != size) {
    warning("External RAM save incomplete - expected to write %u bytes, actually wrote %u bytes\n", size, bytesWritten);
  }

  free((void*)pathToFile);
}

void ramLoad(uint8_t* ram, uint32_t size, const char* romFilename)
{
  const char* pathToFile = ramSaveLocation(romFilename);

  // Read the RAM data from disk or create the file if it doesn't exist yet
  if (exists(pathToFile)) {
    FILE* ramFile = fopen(pathToFile, "rb");
    assert(ramFile);
    size_t bytesRead = fread((void*)ram, 1, size, ramFile);
    fclose(ramFile);
    if (bytesRead != size) {
      warning("External RAM load incomplete - expected to read %u bytes, actually wrote %u bytes\n", size, bytesRead);
    }
  } else {
    ramSave(ram, size, romFilename);
  }

  free((void*)pathToFile);
}