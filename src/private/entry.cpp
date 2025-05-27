#include <Windows.h>
#include <string>
#include <cstdint>
#include <iostream>
#include <xinput.h>
#include <dsound.h>

#define global static
#define local static
#define internal static

global bool running = true;
static LPDIRECTSOUNDBUFFER secondaryBuffer;

#pragma region XInput stubs old style
// #define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
// typedef X_INPUT_GET_STATE(x_input_get_state);
// X_INPUT_GET_STATE(XInputGetStateStub)
// {
//     return ERROR_DEVICE_NOT_CONNECTED;
// };


// #define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
// typedef X_INPUT_SET_STATE(x_input_set_state);
// X_INPUT_SET_STATE(XInputSetStateStub)
// {
//     return ERROR_DEVICE_NOT_CONNECTED;
// };

// global x_input_get_state* XInputGetState_ = XInputGetStateStub; // Pointer to the XInputGetState function
// global x_input_set_state* XInputSetState_ = XInputSetStateStub; // Pointer to the XInputSetState function
// #define XInputGetState XInputGetState_
// #define XInputSetState XInputSetState_

// internal void LoadInputLibrary()
// {
//     HMODULE xinputModule = LoadLibraryA("xinput1_4.dll");
//     if (xinputModule)
//     {
//         XInputGetState = (x_input_get_state*)GetProcAddress(xinputModule, "XInputGetState");
//         XInputSetState = (x_input_set_state*)GetProcAddress(xinputModule, "XInputSetState");
//     }
//     else
//     {
//         OutputDebugStringA("Failed to load xinput1_4.dll\n");
    
//     }
// }
#pragma endregion xInput stubs old style

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
    //TODO: start it playing sound
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


struct OffscreenBuffer
{
    BITMAPINFO info;
    void* data;
    int width,height;
    int bpp{4};
    int pitch{0};
};
global OffscreenBuffer backBuffer;


//NOTE: on previous version i have % 256
//modulo operator can be slightly slower than bitwise AND for powers of two.
// Since 256 is a power of two, val % 256 is equivalent to val & 255 (or val & 0xFF).
//Casting to uint8_t will naturally take the lower 8 bits, which is the same as % 256 
//for positive results of the sum. This is usually handled efficiently by the compiler anyway

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

//TODO:internal bool  ResizeDIBSection(OffscreenBuffer* buffer,int width, int height) //would be better choice
internal void ResizeDIBSection(OffscreenBuffer* buffer,int width, int height)
{

    if (buffer->data)
    {
        VirtualFree(buffer->data, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    

    buffer->info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // negative to flip the image
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

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
        buffer.data, &buffer.info, DIB_RGB_COLORS, SRCCOPY);
  
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
    int Hz = 256;
    int SquareWavePeriod = 44100 / Hz; // Calculate the period of the square wave based on the sample rate
    int BytesPerSample = sizeof(int16_t) * 2; // 16-bit stereo audio (2 channels)
    uint32_t SampleIndex = 0; // Initialize sample index for audio playback

    InitDSound(hwnd, 44100, 2, 44100*BytesPerSample); // Initialize DirectSound with sample rate, channels, and buffer size

    secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
    MSG msg{};
    while (running)
    {
        
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }



        //TODO: should we poll more friquently 

        for (DWORD cIndex = 0; cIndex < XUSER_MAX_COUNT; ++cIndex)
        {
            XINPUT_STATE state;
            if(Input::XInputGetState(cIndex, &state) == ERROR_SUCCESS)
            {
                //TODO: See if state.dwPacketNumber is increment too rapidly

                auto& gamepad = state.Gamepad;
                //Process gamepad input
                bool Up = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
                bool Down = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
                bool Left = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
                bool Right = (gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
                bool Start = (gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
                bool Back = (gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
                bool A = (gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
                bool B = (gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
                bool X = (gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
                bool Y = (gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
                float LeftTrigger = gamepad.bLeftTrigger / 255.0f; // Normalize to [0, 1]
                float RightTrigger = gamepad.bRightTrigger / 255.0f; // Normalize to [0, 1]
                float LeftThumbX = gamepad.sThumbLX / 32767.0f; // Normalize to [-1, 1]
                float LeftThumbY = gamepad.sThumbLY / 32767.0f; // Normalize to [-1, 1]
                float RightThumbX = gamepad.sThumbRX / 32767.0f; // Normalize to [-1, 1]
                float RightThumbY = gamepad.sThumbRY / 32767.0f; // Normalize to [-1, 1]

                //Debug output for gamepad state
                std::string debugOutput = "Gamepad " + std::to_string(cIndex) + " State:\n";
                debugOutput += "Up: " + std::to_string(Up) + "\n";
                debugOutput += "Down: " + std::to_string(Down) + "\n";
                debugOutput += "Left: " + std::to_string(Left) + "\n";
                debugOutput += "Right: " + std::to_string(Right) + "\n";
                debugOutput += "Start: " + std::to_string(Start) + "\n";
                debugOutput += "Back: " + std::to_string(Back) + "\n";
                debugOutput += "A: " + std::to_string(A) + "\n";
                debugOutput += "B: " + std::to_string(B) + "\n";
                debugOutput += "X: " + std::to_string(X) + "\n";
                debugOutput += "Y: " + std::to_string(Y) + "\n";
                debugOutput += "Left Trigger: " + std::to_string(LeftTrigger) + "\n";
                debugOutput += "Right Trigger: " + std::to_string(RightTrigger) + "\n";
                debugOutput += "Left Thumb X: " + std::to_string(LeftThumbX) + "\n";
                debugOutput += "Left Thumb Y: " + std::to_string(LeftThumbY) + "\n";
                debugOutput += "Right Thumb X: " + std::to_string(RightThumbX) + "\n";
                debugOutput += "Right Thumb Y: " + std::to_string(RightThumbY) + "\n";
                OutputDebugStringA(debugOutput.c_str());

               if (Up)
               {
                   yOffset -= 1;
               }
               if (Down)
               {
                   yOffset += 1;
               }
               if (Left)
               {
                   xOffset -= 1;
               }
               if (Right)
               {
                   xOffset += 1;
               }
               


            }
            else
            {
                //Controller is not connected
            }
        }

        // XINPUT_VIBRATION vibration = {};
        // vibration.wLeftMotorSpeed = 3000; // No vibration
        // vibration.wRightMotorSpeed = 3000; // No vibration
        // XInputSetState(0, &vibration); // Reset vibration for controller 0

        RenderGradiant(backBuffer,xOffset, yOffset);

        //NOTE: Direct sound output test
        //Circular buffer output
        if (secondaryBuffer)
        {
            DWORD PlayCursor, WriteCursor;
            HRESULT hr = secondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
            if (FAILED(hr))
            {
                OutputDebugStringA("Failed to get current position of secondary buffer\n");
                continue; // Skip this iteration if we can't get the position
            }

            DWORD BytesToLock = SampleIndex * BytesPerSample % (44100 * BytesPerSample);
            DWORD BytesToWrite;

            if (BytesToLock > PlayCursor)
            {
                BytesToWrite = (44100 * BytesPerSample) - BytesToLock;
                BytesToWrite += PlayCursor;
            }
            else
            {
                BytesToWrite = PlayCursor - BytesToLock;
            }

            void* Region1;
            DWORD Region1Size;
            void* Region2;
            DWORD Region2Size;

            hr = secondaryBuffer->Lock(BytesToLock, BytesToWrite,
                                       &Region1, &Region1Size,
                                       &Region2, &Region2Size,
                                       0);
            
            if (SUCCEEDED(hr))
            {
                // Process Region 1
                int16_t* pData1 = (int16_t*)Region1;
                DWORD Region1SampleCount = Region1Size / BytesPerSample;

                for (DWORD index = 0; index < Region1SampleCount; ++index)
                {
                    // Generate square wave: alternate between high and low every half period
                    int16_t SampleValue = ((SampleIndex / (SquareWavePeriod / 2)) % 2) ? 16000 : -16000;
                    *pData1++ = SampleValue; // Left channel
                    *pData1++ = SampleValue; // Right channel
                    SampleIndex++;
                }

                // Process Region 2 (if it exists)
                if (Region2 && Region2Size > 0)
                {
                    int16_t* pData2 = (int16_t*)Region2;
                    DWORD Region2SampleCount = Region2Size / BytesPerSample;

                    for (DWORD index = 0; index < Region2SampleCount; ++index)
                    {
                        int16_t SampleValue = ((SampleIndex / (SquareWavePeriod / 2)) % 2) ? 16000 : -16000;
                        *pData2++ = SampleValue; // Left channel
                        *pData2++ = SampleValue; // Right channel
                        SampleIndex++;
                    }
                }

                // Unlock the buffer
                secondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);

                // Start playing if not already playing
                DWORD status;
                if (SUCCEEDED(secondaryBuffer->GetStatus(&status)))
                {
                    if (!(status & DSBSTATUS_PLAYING))
                    {
                        
                    }
                }
            }
            else
            {
                OutputDebugStringA("Failed to lock DirectSound buffer\n");
            }
        }

        HDC hdc = GetDC(hwnd);
        auto dimensions = GetWindowDimensions(hwnd);

        DrawBuffer(hdc, dimensions.width,dimensions.height,backBuffer, 0, 0);
        ReleaseDC(hwnd, hdc);
       
       
    }

    return 0;
}