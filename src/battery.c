#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "battery.h"
#include "utils/os.h"

#define PATH_TO_BATTERY_SAVE_DIR "/Library/Application Support/GBEmu1/Battery/"
#define BATTERY_SAVE_FILE_EXTENSION ".bat"

const char* batterySaveLocation(const char* romFilename)
{
  const char* pathToHomeDir = getenv("HOME");

  ExtSplitPathComponents romFilenameComponents = splitext(romFilename);

  size_t pathToHomeDirLength = strlen(pathToHomeDir);
  size_t pathToBatterySaveDirLength = strlen(PATH_TO_BATTERY_SAVE_DIR);
  size_t romFilenameNoExtLength = strlen(romFilenameComponents.left);
  size_t batterySaveFileExtensionLength = strlen(BATTERY_SAVE_FILE_EXTENSION);

  size_t pathToSaveFileLength = pathToHomeDirLength + pathToBatterySaveDirLength + romFilenameNoExtLength + batterySaveFileExtensionLength;

  char* pathToSaveFile = (char*)malloc((pathToSaveFileLength + 1) * sizeof(char));
  assert(pathToSaveFile);

  strncpy(&(pathToSaveFile[0]), pathToHomeDir, pathToHomeDirLength);
  strncpy(&(pathToSaveFile[pathToHomeDirLength]), PATH_TO_BATTERY_SAVE_DIR, pathToBatterySaveDirLength);
  strncpy(&(pathToSaveFile[pathToHomeDirLength + pathToBatterySaveDirLength]), romFilenameComponents.left, romFilenameNoExtLength);
  strncpy(&(pathToSaveFile[pathToHomeDirLength + pathToBatterySaveDirLength + romFilenameNoExtLength]), BATTERY_SAVE_FILE_EXTENSION, batterySaveFileExtensionLength);
  pathToSaveFile[pathToSaveFileLength] = '\0';

  free((void*)romFilenameComponents.left);
  free((void*)romFilenameComponents.right);

  return pathToSaveFile;
}

void batterySave(uint8_t* data, uint32_t size, const char* romFilename)
{
  const char* pathToFile = batterySaveLocation(romFilename);

  const char* destinationDir = dirname(pathToFile);
  if (!exists(destinationDir)) {
    mkdirp(destinationDir);
  }
  free((void*)destinationDir);

  FILE* saveFile = fopen(pathToFile, "wb");
  assert(saveFile);
  size_t bytesWritten = fwrite((void*)data, 1, size, saveFile);
  fclose(saveFile);
  if (bytesWritten != size) {
    warning("Battery save incomplete - expected to write %u bytes, actually wrote %u bytes\n", size, bytesWritten);
  }

  free((void*)pathToFile);
}

void batteryLoad(uint8_t* data, uint32_t size, const char* romFilename)
{
  const char* pathToFile = batterySaveLocation(romFilename);

  // Read the save data from disk or create the file if it doesn't exist yet
  if (exists(pathToFile)) {
    FILE* saveFile = fopen(pathToFile, "rb");
    assert(saveFile);
    size_t bytesRead = fread((void*)data, 1, size, saveFile);
    fclose(saveFile);
    if (bytesRead != size) {
      warning("Battery save load incomplete - expected to read %u bytes, actually read %u bytes\n", size, bytesRead);
    }
  } else {
    batterySave(data, size, romFilename);
  }

  free((void*)pathToFile);
}