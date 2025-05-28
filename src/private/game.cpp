#include "game.h"
#include <math.h>



void GameOutputSound(SoundOutputBuffer& buffer,int toneHz)
{
    local float tSine;
    int16_t ToneVolume = 3000;
    int WavePeriod = buffer.samplesPerSecond / toneHz;

    int16_t* SampleOut = buffer.samples;
    for(int i = 0; i < buffer.sampleCount; ++i)
    {
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * M_PI * 1.0f / (float)WavePeriod;
    }
}

internal void RenderGradiant(OffscreenBuffer& buffer,int xOffset,int yOffset)
{
   
    uint8_t* row = (uint8_t*)buffer.data;
    for(int y = 0; y < buffer.height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for(int x = 0; x < buffer.width; ++x)
        {
            uint8_t red = (uint8_t)(x + xOffset);
            uint8_t green = (uint8_t)(y + yOffset);
            uint8_t blue = (uint8_t)(y + yOffset);
            *pixel++ = (red << 16) | (green << 8) | blue;
        }
        row += buffer.pitch;
    }
}


void GameUpdateAndRender(OffscreenBuffer& buffer,SoundOutputBuffer& soundBuffer, int toneHz)
{
    //TODO: Allow sample offsets for more robust platform options
    GameOutputSound(soundBuffer,toneHz);
    RenderGradiant(buffer, 0, 0);
}