#include "TruePeakMeter.h"

namespace LUFS
{

TruePeakMeter::TruePeakMeter()
{
    int error;
    src_new(SRC_SINC_MEDIUM_QUALITY, 1, &error);
}

}