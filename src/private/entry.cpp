#include <Windows.h>
#include <string>
#include <cstdint>

#define global static
#define local static
#define internal static

global bool running = true;


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




internal void RenderGradiant(OffscreenBuffer buffer,int xOffset,int yOffset)
{
   
    uint8_t* row = (uint8_t*)buffer.data;
    for(int y = 0; y < buffer.height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for(int x = 0; x < buffer.width; ++x)
        {
            uint8_t red = (x + xOffset) % 256;
            uint8_t green = (y + yOffset) % 256;
            uint8_t blue = (x + yOffset) % 256;
            *pixel++ = (red << 16) | (green << 8) | blue;
        }
        row += buffer.pitch;
    }
}

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
        return;
    }

    buffer->pitch = width * buffer->bpp;
}

internal void DrawBuffer(HDC hdc,int windowWidth,int windowHeight,OffscreenBuffer buffer, int x, int y)
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

        RenderGradiant(backBuffer,xOffset, 0);
        HDC hdc = GetDC(hwnd);
        auto dimensions = GetWindowDimensions(hwnd);

        DrawBuffer(hdc, dimensions.width,dimensions.height,backBuffer, 0, 0);
        ReleaseDC(hwnd, hdc);
        ++xOffset;
       
       
    }

    return 0;
}