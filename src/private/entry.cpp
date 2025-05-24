#include <Windows.h>


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MessageBoxA(NULL, "Hello, World!", "My First Windows App", MB_OK);
    return 0;
}