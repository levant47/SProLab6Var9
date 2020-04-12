#include <windows.h>

HINSTANCE hInst;
LPCTSTR szWindowClass = "WindowClass";
LPCTSTR szTitle = "Title";

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

enum MenuOptionId
{
    FILE_CREATE = 1,
    FILE_DELETE = 2,
    FILE_READ = 3,
    FILE_COPY = 4,
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_HAND);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = GetSysColorBrush(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL;

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    hInst = hInstance;
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_CREATE:
        {
            HMENU hMenubar;
            HMENU hMenu;

            hMenubar = CreateMenu();
            hMenu = CreateMenu();

            AppendMenu(hMenu, MF_STRING, MenuOptionId::FILE_CREATE, "Створити файл");
            AppendMenu(hMenu, MF_STRING, MenuOptionId::FILE_DELETE, "Видалити файли");
            AppendMenu(hMenu, MF_STRING, MenuOptionId::FILE_READ, "Прочитати файл");
            AppendMenu(hMenu, MF_STRING, MenuOptionId::FILE_COPY, "Копiювати файли");

            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, "File");
            SetMenu(hWnd, hMenubar);

            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            DrawText(hdc, "Привіт, світ!", -1, &clientRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            EndPaint(hWnd, &ps);

            break;
        }
        case WM_COMMAND:
        {
            auto menuOptionId = (MenuOptionId)LOWORD(wParam);

            switch (menuOptionId)
            {
                case MenuOptionId::FILE_CREATE:
                {
                    char fileName[MAX_PATH] = "";
                    OPENFILENAME openFileName{};
                    openFileName.lStructSize = sizeof(openFileName);
                    openFileName.hwndOwner = hWnd;
                    openFileName.hInstance = hInst;
                    openFileName.lpstrFilter = "New File\0\0";
                    openFileName.lpstrFile = fileName;
                    openFileName.nMaxFile = MAX_PATH;
                    openFileName.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                    openFileName.lpstrDefExt = "txt";

                choose_file:
                    BOOL wasFileChosen = GetSaveFileName(&openFileName);
                    if (!wasFileChosen)
                    {
                        break;
                    }

                    WIN32_FIND_DATA findData{};
                    auto doesFileAlreadyExist = FindFirstFile(fileName, &findData) != INVALID_HANDLE_VALUE;
                    if (doesFileAlreadyExist)
                    {
                        MessageBox(hWnd, "File already exists", "Error", MB_OK);
                        goto choose_file;
                    }

                    auto createdFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (createdFile == INVALID_HANDLE_VALUE)
                    {
                        MessageBox(hWnd, "Failed to create file", "Error", MB_OK);
                        break;
                    }

                    CloseHandle(createdFile);

                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
