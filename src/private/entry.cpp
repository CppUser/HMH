#include <Windows.h>
#include <string>
#include <cstdint>
#include <iostream>
#include <xinput.h>


#define global static
#define local static
#define internal static

global bool running = true;

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
        if (!module) return false;

        if (auto proc = GetProcAddress(module, "XInputGetState")) {
            XInputGetState = reinterpret_cast<XInputGetStateFunc>(proc);
        }

        if (auto proc = GetProcAddress(module, "XInputSetState")) {
            XInputSetState = reinterpret_cast<XInputSetStateFunc>(proc);
        }

        return true;
    }

} // namespace Input


#pragma endregion XInput stubs new style

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
        HDC hdc = GetDC(hwnd);
        auto dimensions = GetWindowDimensions(hwnd);

        DrawBuffer(hdc, dimensions.width,dimensions.height,backBuffer, 0, 0);
        ReleaseDC(hwnd, hdc);
       
       
    }

    return 0;
}