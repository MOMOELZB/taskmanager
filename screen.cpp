#include "screen.h"
#include "gammaramp.h"
Screen::Screen()
{

}




void Screen::SetScreenBrightness(int value)
{
    CGammaRamp GammaRamp;
    GammaRamp.SetBrightness(NULL, value);
}

