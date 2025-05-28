#include "globals.h"
#include <string>

#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include "game.h"
#include <stdio.h>

global bool running = true;
global LPDIRECTSOUNDBUFFER secondaryBuffer;
internal BITMAPINFO bitmapInfo = {}; // Global variable for bitmap info

#pragma region XInput stubs new style
namespace Input
{
    using XInputGetStateFunc = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
    using XInputSetStateFunc = DWORD(WINAPI*)(DWORD, XINPUT_VIBRATION*);

    auto XInputGetStateStub(DWORD, XINPUT_STATE*) -> DWORD { 
        return ERROR_DEVICE_NOT_CONNECTED; 
    }

    auto XInputSetStateStub(DWORD, XINPUT_VIBRATION*) -> DWORD { 
        return ERROR_DEVICE_NOT_CONNECTED; 
    }

    inline XInputGetStateFunc XInputGetState = XInputGetStateStub;
    inline XInputSetStateFunc XInputSetState = XInputSetStateStub;

    auto LoadInputLibrary() -> bool {
    

        auto module = LoadLibraryA("xinput1_4.dll");
        if (!module)
        {
             module = LoadLibraryA("xinput1_3.dll");
        }

        if (auto proc = GetProcAddress(module, "XInputGetState")) {
            XInputGetState = reinterpret_cast<XInputGetStateFunc>(proc);
        }

        if (auto proc = GetProcAddress(module, "XInputSetState")) {
            XInputSetState = reinterpret_cast<XInputSetStateFunc>(proc);
        }

        return true;
    }

} // namespace Input
#pragma endregion XInput stubs new style\

auto InitDSound(HWND hwnd, int sampleRate, int channels, int bufferSize) -> bool
{
    // Initialize DirectSound
    HMODULE dsoundModule = LoadLibraryA("dsound.dll");
    if (!dsoundModule) {
        OutputDebugStringA("Failed to load dsound.dll\n");
        return false;
    }

    typedef HRESULT(WINAPI* DirectSoundCreateFunc)(LPCGUID, LPDIRECTSOUND*, LPUNKNOWN);
    DirectSoundCreateFunc DirectSoundCreate = (DirectSoundCreateFunc)GetProcAddress(dsoundModule, "DirectSoundCreate");

    if (!DirectSoundCreate) {
        OutputDebugStringA("Failed to get DirectSoundCreate function\n");
        return false;
    }

    // Create DirectSound object
    LPDIRECTSOUND directSound;
    HRESULT hr = DirectSoundCreate(nullptr, &directSound, nullptr);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create DirectSound object\n");
        return false;
    }

    // Set the cooperative level
    // DSSCL_DEFAULT would use the default cooperative level
    // DSSCL_PRIORITY is often used for games to ensure low latency and high performance
    // DSSCL_EXCLUSIVE is used for applications that need exclusive access to the sound device
    // DSSCL_BACKGROUND is used for applications that do not need exclusive access to the sound device
    // DSSCL_WRITEPRIMARY is used for applications that need to write to the primary buffer
    // DSSCL_DEFAULT is used for applications that do not need to set a specific cooperative level
    hr = directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to set cooperative level\n");
        return false;
    }

    // Create a primary buffer
    // The primary buffer is used to control the sound device and set the format
    // It is not used for playing sound directly, but rather to set the format for secondary buffers
    
    DSBUFFERDESC bufferDesc = {};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC); // Size of the structure
    //bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPOSITIONNOTIFY; // Flags for the primary buffer
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER; 


    LPDIRECTSOUNDBUFFER primaryBuffer;
    hr = directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, nullptr);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create primary sound buffer\n");
        return false;
    }

    // Set the format of the primary buffer
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = channels;
    waveFormat.nSamplesPerSec = sampleRate;
    waveFormat.wBitsPerSample = 16; // 16-bit samples
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    hr = primaryBuffer->SetFormat(&waveFormat);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to set format on primary sound buffer\n");
        return false;
    }

    // Create a secondary buffer
    // The secondary buffer is used for playing sound

    // Secondary buffers are used to play sound
    // The primary buffer can be created with different flags and formats
    // DSBCAPS_PRIMARYBUFFER is used to indicate that this is a primary buffer
    // DSBCAPS_CTRLVOLUME is used to indicate that the buffer can control volume
    // DSBCAPS_CTRLFREQUENCY is used to indicate that the buffer can control frequency
    // DSBCAPS_CTRLPOSITIONNOTIFY is used to indicate that the buffer can control position notifications
    // DSBCAPS_GLOBALFOCUS is used to indicate that the buffer can be used when the application is not in focus
    // DSBCAPS_GETCURRENTPOSITION2 is used to indicate that the buffer can get the current position
    // DSBCAPS_CTRL3D is used to indicate that the buffer can control 3D sound
    // DSBCAPS_CTRLFX is used to indicate that the buffer can control effects
    // DSBCAPS_CTRLDEFAULT is used to indicate that the buffer can control default settings
    // DSBCAPS_CTRLALL is used to indicate that the buffer can control all settings
    DSBUFFERDESC secondaryBufferDesc = {};
    secondaryBufferDesc.dwSize = sizeof(DSBUFFERDESC); // Size of the structure
    secondaryBufferDesc.dwFlags = 0; // No special flags for the secondary buffer
    secondaryBufferDesc.dwBufferBytes = bufferSize; // Size of the buffer in bytes
    secondaryBufferDesc.lpwfxFormat = &waveFormat; // Format of the buffer

    
    hr = directSound->CreateSoundBuffer(&secondaryBufferDesc, &secondaryBuffer, nullptr);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create secondary sound buffer\n");
        return false;
    }
    return true;
}

struct SoundOutput{

    int samplesPerSecond = 48000; // Sample rate
    int toneHz = 256;             // Frequency of the sine wave
    int16_t toneVolume = 3800;        // Volume (amplitude)
    uint32_t sampleIndex = 0;         // Current sample index;
    int WavePeriod = samplesPerSecond/toneHz;
    int bytesPerSample = sizeof(int16_t) * 2;
    int secondaryBufferSize = samplesPerSecond * bytesPerSample;
    int latencySampleCount = samplesPerSecond / 15;

};

internal void FillSoundBuffer(SoundOutput& soundOutput, DWORD ByteToLock, DWORD BytesToWrite,SoundOutputBuffer& source)
{

    if (!running || !secondaryBuffer || !source.samples) {
        return;  // Don't fill buffer if shutting down
    }

    VOID* region1;
    DWORD region1Size;
    VOID* region2;
    DWORD region2Size;
    if(SUCCEEDED(secondaryBuffer->Lock(ByteToLock, BytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput.bytesPerSample;
        DWORD region2SampleCount = region2Size / soundOutput.bytesPerSample;
        DWORD totalSampleCount = region1SampleCount + region2SampleCount;

        int16_t* destSample = (int16_t*)region1;
        int16_t* sourceSample = source.samples;

        for(DWORD i = 0; i < totalSampleCount; ++i)
        {
            // Switch to region2 when we've filled region1
            if(i == region1SampleCount)
            {
                destSample = (int16_t*)region2;
            }
            
            *destSample++ = *sourceSample++; // Left channel
            *destSample++ = *sourceSample++; // Right channel
            soundOutput.sampleIndex++;
        }

        secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
 
}

internal void ClearSoundBuffer(SoundOutput& soundOutput)
{
    VOID* region1;
    DWORD region1Size;
    VOID* region2;
    DWORD region2Size;
    if(SUCCEEDED(secondaryBuffer->Lock(0, soundOutput.secondaryBufferSize, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput.bytesPerSample;
        DWORD region2SampleCount = region2Size / soundOutput.bytesPerSample;
        DWORD totalSampleCount = region1SampleCount + region2SampleCount;

        int16_t* destSample = (int16_t*)region1;

        for(DWORD i = 0; i < totalSampleCount; ++i)
        {
            // Switch to region2 when we've filled region1
            if(i == region1SampleCount)
            {
                destSample = (int16_t*)region2;
            }
            
            *destSample++ = 0; // Left channel
            *destSample++ = 0; // Right channel
        }

        secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

struct Dimensions
{
    int width;
    int height;
};

//Helper function to get window dimensions
internal Dimensions GetWindowDimensions(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left; 
    int height = rect.bottom - rect.top;
    return {width, height};
}

global OffscreenBuffer backBuffer;

internal void ResizeDIBSection(OffscreenBuffer* buffer,int width, int height)
{

    if (buffer->data)
    {
        VirtualFree(buffer->data, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = buffer->width;
    bitmapInfo.bmiHeader.biHeight = -buffer->height; // negative to flip the image
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapDataSize = (buffer->width * buffer->height) * buffer->bpp; // 4 bytes per pixel (RGBA)
    buffer->data = VirtualAlloc(nullptr, bitmapDataSize,
                              MEM_COMMIT | MEM_RESERVE,
                              PAGE_READWRITE);
    if (!buffer->data)
    {
        OutputDebugStringA("Failed to allocate bitmap data\n");
        //TODO: CRITICAL ISSUE: What happens if VirtualAlloc fails?
        // The function returns, but buffer->data is nullptr.
        // Subsequent calls to RenderGradiant or DrawBuffer will try to dereference a null pointer,
        // leading to a crash.
        return;


        //TODO: Handle this case properly
        /*
        If VirtualAlloc fails, you output a debug string and return. However, buffer->data remains nullptr.
         The backBuffer (which is likely the buffer being passed here) will have nullptr for its data. 
         The main loop will continue, call RenderGradiant which will try to write to nullptr, 
         and then DrawBuffer will try to read from nullptr. This will cause a crash.
        
        */
       //return false; // Indicate failure
    }

    buffer->pitch = width * buffer->bpp;
    //return true;

    // In WinMain:
    // if (!ResizeDIBSection(&backBuffer, 800, 600)) {
    //     MessageBoxA(nullptr, "Failed to initialize back buffer", "Error", MB_OK | MB_ICONERROR);
    //     return -1; // or handle appropriately
    // }
}

internal void DrawBuffer(HDC hdc,int windowWidth,int windowHeight,OffscreenBuffer& buffer, int x, int y)
{

    //Fixing aspect ration since we call resizeDibSection in begining with predefined width and height
    //TODO:: Fix aspec ration


    StretchDIBits(
        hdc, 
        0, 0, windowWidth, windowHeight,    // Destination rectangle
        0, 0, buffer.width, buffer.height,  // Source rectangle  
        buffer.data, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  
}

LRESULT CALLBACK Wndproc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
     switch (msg)
    {
        case WM_ACTIVATEAPP:{
            OutputDebugStringA("WM_ACTIVATEAPP\n");

        }break;
        case WM_DESTROY:
            //TODO: Handle as error to recreate window
            running = false;
            break;
        case WM_CLOSE:
            //TODO: handle as message to user
            running = false;
            break;
        case WM_SIZE:{
            //Creating a DIB section
           
                
        }break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32_t key = (uint32_t)wParam;
            bool isDown = (lParam & (1 << 31)) == 0; // Check if the high bit is not set
            bool wasDown = (lParam & (1 << 30)) != 0; // Check if the second high bit is set

            if (isDown)
            {
                if (key == VK_ESCAPE)
                {
                    // Handle escape key to close the application
                    running = false;
                }
                
                // Key is pressed
                std::string keyName = "Key Pressed: " + std::to_string(key) + "\n";
                std::cout << keyName;

                if (wasDown)
                {
                    // Key was already down, this is a repeat
                    std::cout << "Key Repeat: " + std::to_string(key) + "\n";
                }
                else
                {
                    // Key was just pressed
                    std::cout << "Key Just Pressed: " + std::to_string(key) + "\n";
                }
            }
            else
            {
                // Key is released
                std::string keyName = "Key Released: " + std::to_string(key) + "\n";
                std::cout << keyName;
            }


        }break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
           

            int x = ps.rcPaint.left;
            int y = ps.rcPaint.top;

            auto dimensions = GetWindowDimensions(hwnd);
            DrawBuffer(hdc,dimensions.width,dimensions.height,backBuffer,x,y);

            EndPaint(hwnd, &ps);
            
        }break;
    }
    
     return DefWindowProcA(hwnd, msg, wParam, lParam);
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
     LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency))
    {
        MessageBoxA(nullptr, "Failed to query performance frequency", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }


    if(!Input::LoadInputLibrary())
    {
        MessageBoxA(nullptr, "Failed to load xinput1_4.dll", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    //Initialize the back buffer
    ResizeDIBSection(&backBuffer,800, 600);

    WNDCLASSA wc{};
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "hmhWindowClass";

   
    if(!RegisterClassA(&wc))
    {
        MessageBoxA(nullptr, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "hmhWindowClass",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    if(!hwnd)
    {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }


    int xOffset = 0;
    int yOffset = 0;

    SoundOutput soundOutput;
    if(!InitDSound(hwnd, soundOutput.samplesPerSecond, 2, soundOutput.secondaryBufferSize)){
        MessageBoxA(nullptr, "Failed to initialize DirectSound", "Error", MB_OK | MB_ICONERROR);
    }
    bool SoundIsPlaying = false;
    ClearSoundBuffer(soundOutput);
    
    int16_t* samples = (int16_t*)VirtualAlloc(nullptr, soundOutput.secondaryBufferSize,
                              MEM_COMMIT | MEM_RESERVE,
                              PAGE_READWRITE);

    secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING); // Start playing the sound buffer
    MSG msg{};
    
    LARGE_INTEGER lastCounter;
    QueryPerformanceCounter(&lastCounter);

    //int64_t LastCycleCount = __rdtsc();

    while (running)
    {        

        //TODO: That platform message handling can be moved to platform HandleEvents
        #pragma region Platform Message Handling
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        #pragma endregion Platform Message Handling



        //TODO: should we poll more friquently 
#pragma region Input Handling
        for (DWORD cIndex = 0; cIndex < XUSER_MAX_COUNT; ++cIndex)
        {
            XINPUT_STATE state;
            if(Input::XInputGetState(cIndex, &state) == ERROR_SUCCESS)
            {
                //TODO: See if state.dwPacketNumber is increment too rapidly

                auto& gamepad = state.Gamepad;
                //Process gamepad input
                // bool Up = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
                // bool Down = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
                // bool Left = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
                // bool Right = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

                int16_t StickX = gamepad.sThumbLX;
                int16_t StickY = gamepad.sThumbLY;

                xOffset += StickX / 4096;
                yOffset += StickY / 4096;

                soundOutput.toneHz = 512 + (int)(256.0f*(float)StickY / 30000.0f);
                soundOutput.WavePeriod = soundOutput.samplesPerSecond/soundOutput.toneHz;


            }
            else
            {
                //Controller is not connected
            }
        }
#pragma endregion

        DWORD PlayCursor = 0;
        DWORD WriteCursor = 0;
        DWORD ByteToLock;
        DWORD TargetCursor;
        DWORD BytesToWrite;
        bool SoundIsValid = false;
        if(SUCCEEDED(secondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
        {
            ByteToLock = (soundOutput.sampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;

            TargetCursor = ((PlayCursor + (soundOutput.latencySampleCount*soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize);
            
            if (ByteToLock > TargetCursor)
            {
                BytesToWrite = (soundOutput.secondaryBufferSize - ByteToLock);
                BytesToWrite += TargetCursor;
            }
            else
            {
                BytesToWrite = TargetCursor - ByteToLock;
            }

            SoundIsValid = true;
        }

        
        SoundOutputBuffer soundBuffer;
        soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
        soundBuffer.sampleCount = BytesToWrite/ soundOutput.bytesPerSample; //for 30 fps
        soundBuffer.samples = samples;


        OffscreenBuffer buffer = {};
        buffer.data = backBuffer.data;
        buffer.width = backBuffer.width;
        buffer.height = backBuffer.height;
        buffer.pitch = backBuffer.pitch;
        buffer.bpp = backBuffer.bpp;

        GameUpdateAndRender(buffer,soundBuffer,soundOutput.toneHz);

        if(SoundIsValid)
        {
            
            FillSoundBuffer(soundOutput, ByteToLock, BytesToWrite, soundBuffer);
        }
        
        if (!SoundIsPlaying)
        {
            HRESULT hr = secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            if (SUCCEEDED(hr))
            {
                SoundIsPlaying = true;
            }
            else
            {
                OutputDebugStringA("Failed to play sound buffer\n");
            }
        }
        

       

        HDC hdc = GetDC(hwnd);
        auto dimensions = GetWindowDimensions(hwnd);

        DrawBuffer(hdc, dimensions.width,dimensions.height,backBuffer, 0, 0);
        ReleaseDC(hwnd, hdc);
       
        //int64_t endCycleCount = __rdtsc();

        LARGE_INTEGER endCounter;
        QueryPerformanceCounter(&endCounter);

        //int64_t cyclesElapsed = endCycleCount - LastCycleCount;
        //int64_t elapsedCounter = endCounter.QuadPart - lastCounter.QuadPart;
        //double msPerFrame = (double)(elapsedCounter * 1000) / (double)frequency.QuadPart;
        //double fps = (double)frequency.QuadPart / (double)elapsedCounter;

       

        //lastCounter = endCounter;
        //LastCycleCount = endCycleCount;
    }

    return 0;
}