#include <Windows.h>
#include <string>

LRESULT CALLBACK Wndproc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
     switch (msg)
    {
        case WM_ACTIVATEAPP:{
            OutputDebugStringA("WM_ACTIVATEAPP\n");

        }break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            break;
        case WM_SIZE:{
            OutputDebugStringA("WM_SIZE\n");
                
        }break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            int x = ps.rcPaint.left;
            int y = ps.rcPaint.top;
            int width = ps.rcPaint.right - ps.rcPaint.left;
            int height = ps.rcPaint.bottom - ps.rcPaint.top;
            static DWORD Operation = WHITENESS; //Temp for testing
            PatBlt(hdc, x, y,width, height, Operation);
            Operation = (Operation == WHITENESS) ? BLACKNESS : WHITENESS;

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

    MSG msg{};
    bool running = true;
    while (running)
    {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
       
    }

    return 0;
}