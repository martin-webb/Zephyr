#include "displaylink.h"

#include "logging.h"

#include <stdint.h>
#include <stdlib.h>

#import <CoreVideo/CVDisplayLink.h>


double cvDisplayLinkGetNominalOutputVideoRefreshPeriod()
{
  double refreshPeriod;

  CVDisplayLinkRef displayLinkRef;
  CVDisplayLinkCreateWithActiveCGDisplays(&displayLinkRef);

  CVTime nominal = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLinkRef);
  if (nominal.flags & kCVTimeIsIndefinite) {
    error("Failed to get nominal output video refresh period");
    exit(EXIT_FAILURE);
  }

  int32_t timeScale = nominal.timeScale;
  int64_t timeValue = nominal.timeValue;
  refreshPeriod = (double)timeScale / (double)timeValue;

  CVDisplayLinkRelease(displayLinkRef);

  return refreshPeriod;
}
