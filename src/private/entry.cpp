#include <Windows.h>
#include <string>
#include <cstdint>

#define global static
#define local static
#define internal static

global bool running = true;

global BITMAPINFO bitmapInfo;
global void* bitmapData = nullptr;
global int bitmapWidth = 0;
global int bitmapHeight = 0;

internal void RenderGradiant(int xOffset,int yOffset)
{
    int width = bitmapWidth;
    int height = bitmapHeight;

    int Pitch = width * 4; 
    uint8_t* row = (uint8_t*)bitmapData;
    for (int y = 0; y < height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < width; ++x)
        {
            uint8_t red = (x + xOffset) % 256;
            uint8_t green = (y + yOffset) % 256;
            uint8_t blue = (x + y) % 256;
            *pixel++ = (red << 16) | (green << 8) | blue; // BGRA format
        }
        row += Pitch;
    }
}

internal void ResizeDIBSection(int width, int height)
{

    if (bitmapData)
    {
        VirtualFree(bitmapData, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight; // negative to flip the image
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapDataSize = (bitmapWidth * bitmapHeight) * 4; // 4 bytes per pixel (RGBA)
    bitmapData = VirtualAlloc(nullptr, bitmapDataSize,
                              MEM_COMMIT | MEM_RESERVE,
                              PAGE_READWRITE);
    if (!bitmapData)
    {
        OutputDebugStringA("Failed to allocate bitmap data\n");
        return;
    }
}

internal void Win32UpdateWindow(HDC hdc,RECT *windowRect, int x, int y, int width, int height)
{
   
    StretchDIBits(
        hdc, 
        0, 0, width, height,
        0, 0, bitmapWidth, bitmapHeight,
        bitmapData, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  
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
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right - rect.left; 
            int height = rect.bottom - rect.top;
            ResizeDIBSection(width, height);
                
        }break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
           

            int x = ps.rcPaint.left;
            int y = ps.rcPaint.top;
            int width = ps.rcPaint.right - ps.rcPaint.left;
            int height = ps.rcPaint.bottom - ps.rcPaint.top;

            RECT rect;
            GetClientRect(hwnd, &rect);
            Win32UpdateWindow(hdc,&rect ,x,y,width,height);

            EndPaint(hwnd, &ps);
            
        }break;
    }
    
     return DefWindowProcA(hwnd, msg, wParam, lParam);
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    WNDCLASSA wc{};
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
    //wc.hCursor = LoadCursorA(nullptr, IDC_ARROW),
    //wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1), //own drawing will be happening(no need brush)
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

        RenderGradiant(xOffset, 0);
        HDC hdc = GetDC(hwnd);
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        Win32UpdateWindow(hdc, &rect, 0, 0, width, height);
        ReleaseDC(hwnd, hdc);
        ++xOffset;
       
       
    }

    return 0;
}