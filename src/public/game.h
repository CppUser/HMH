#pragma once
#include "globals.h"

/*
    NOTE: Serviceses that the game provide to the platform layer
*/


struct OffscreenBuffer
{
    void* data;
    int width,height;
    int bpp{4};
    int pitch{0};
};

struct SoundOutputBuffer
{
    int16_t* samples = nullptr;
    int samplesPerSecond = 0;
    int sampleCount = 0;

};
//game needs 4 things timer , controller/keyboard input , bitmap buffer to use, sound buffer to use
void GameUpdateAndRender(OffscreenBuffer& buffer,SoundOutputBuffer& soundBuffer, int toneHz);

/*
   TODO: NOTE: Services that the platform layer provide to the game
*/