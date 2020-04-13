#define _CRT_SECURE_NO_WARNINGS
#include <ShlObj.h>
#include <stdio.h>
#include <windows.h>

HINSTANCE hInst;
LPCTSTR szWindowClass = "WindowClass";
LPCTSTR szTitle = "Title";
HWND fileContentsTextBox;
char *fileContentBuffer = NULL;
int fileSize = 0;

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

INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        char currentDirectoryPath[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, currentDirectoryPath);
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)currentDirectoryPath);
    }

    return 0;
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
            AppendMenu(hMenu, MF_STRING, MenuOptionId::FILE_COPY, "Копiювати текст");

            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, "File");
            SetMenu(hWnd, hMenubar);

            fileContentsTextBox = CreateWindowEx(
                NULL,
                "EDIT",
                NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                0, 35, 0, 0,
                hWnd,
                NULL,
                (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
                NULL);

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
                case MenuOptionId::FILE_DELETE:
                {
                    TCHAR selectedDirectoryPath[MAX_PATH];

                    BROWSEINFO browseInfo{};
                    browseInfo.lpszTitle = ("Browse for folder...");
                    browseInfo.lpfn = BrowseCallbackProc;

                    LPITEMIDLIST selectedDirectory = SHBrowseForFolder(&browseInfo);

                    if (selectedDirectory == 0)
                    {
                        break;
                    }

                    SHGetPathFromIDList(selectedDirectory, selectedDirectoryPath);

                    IMalloc *imalloc = 0;
                    if (SUCCEEDED(SHGetMalloc(&imalloc)))
                    {
                        imalloc->Free(selectedDirectory);
                        imalloc->Release();
                    }

                    char pattern[MAX_PATH] = {0};
                    sprintf(pattern, "%s\\*\0\0", selectedDirectoryPath);

                    SHFILEOPSTRUCTA fileOp{};
                    fileOp.hwnd = hWnd;
                    fileOp.wFunc = FO_DELETE;
                    fileOp.pFrom = pattern;
                    fileOp.pTo = NULL;

                    int resultCode = SHFileOperationA(&fileOp);
                    if (resultCode != 0)
                    {
                        MessageBox(hWnd, "Failed to delete files", "Error", MB_OK);
                        break;
                    }

                    break;
                }
                case MenuOptionId::FILE_READ:
                {
                    char fileName[MAX_PATH]{0};

                    OPENFILENAME openFileName{};
                    openFileName.lStructSize = sizeof(openFileName);
                    openFileName.hwndOwner = hWnd;
                    openFileName.hInstance = hInst;
                    openFileName.lpstrFilter = "Text Files\0*.txt\0\0";
                    openFileName.lpstrFile = fileName;
                    openFileName.nMaxFile = MAX_PATH;
                    openFileName.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                    openFileName.lpstrDefExt = "txt";

                    BOOL wasFileChosen = GetOpenFileName(&openFileName);
                    if (!wasFileChosen)
                    {
                        break;
                    }

                    OFSTRUCT of;
                    auto targetFile = CreateFile(openFileName.lpstrFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    fileSize = GetFileSize(targetFile, NULL);
                    if (fileContentBuffer != NULL)
                    {
                        delete[] fileContentBuffer;
                    }
                    fileContentBuffer = new char[fileSize + 1]{0};
                    DWORD bytesRead;
                    BOOL wasReadingSuccessful = ReadFile(targetFile, fileContentBuffer, fileSize, &bytesRead, NULL);
                    if (!wasReadingSuccessful)
                    {
                        MessageBox(hWnd, "Failed to read file", "Read File", MB_OK);
                        break;
                    }

                    CloseHandle(targetFile);

                    SetWindowText(fileContentsTextBox, fileContentBuffer);

                    break;
                }
                case MenuOptionId::FILE_COPY:
                {
                    if (fileContentBuffer == NULL)
                    {
                        MessageBox(hWnd, "No file is opened", "Error", MB_OK);
                        break;
                    }

                    char currentDirectoryPath[MAX_PATH]{0};
                    GetModuleFileName(NULL, currentDirectoryPath, MAX_PATH);
                    char targetFilePath[MAX_PATH];
                    sprintf(targetFilePath, "%s\\..\\Readme.txt\0", currentDirectoryPath);
                    auto targetFile = CreateFile(targetFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

                    DWORD bytesWritten;
                    BOOL wasWritingSuccessful = WriteFile(targetFile, fileContentBuffer, fileSize, &bytesWritten, NULL);
                    if (!wasWritingSuccessful)
                    {
                        MessageBox(hWnd, "Failed to write file", "Error", MB_OK);
                        break;
                    }

                    CloseHandle(targetFile);

                    MessageBox(hWnd, "Success", "Success", MB_OK);

                    break;
                }
            }
            break;
        }
        case WM_SIZE:
        {
            MoveWindow(fileContentsTextBox, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
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
