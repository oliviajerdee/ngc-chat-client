#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <lmcons.h>
#include <functional>
#include "resource.h"

using namespace std;

#define ID_SEND_BUTTON 1
#define ID_INPUT_EDIT 2
#define ID_OUTPUT_EDIT 3

string fileName = "\\\\VBOXSVR\\application\\sharedDrive\\chat_data.csv";
mutex messageMutex;
LARGE_INTEGER previousFileSize = { 0 };

string getUserName() {
    wchar_t buffer[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserNameW(buffer, &size);
    wstring wideUsername(buffer);
    string username(wideUsername.begin(), wideUsername.end());
    return username;
}

string getCurrentTime() {
    auto now = chrono::system_clock::now();
    time_t timestamp = chrono::system_clock::to_time_t(now);
    auto local_time = localtime(&timestamp);
    ostringstream stream;
    stream << put_time(local_time, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

void appendMessageToCsv(const string& fileName, const string& message, const string& timestamp, const string& username) {
    ofstream outFile(fileName, ios_base::app);
    if (!outFile.is_open()) {
        cerr << "Failed to open the file: " << fileName << endl;
        exit(1);
    }

    string quote = "\"" + message + "\"";

    outFile << username << "," << quote << "," << timestamp << '\n';
    outFile.close();
}

bool isFileSizeChanged(const string& fileName, LARGE_INTEGER& previousFileSize) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;

    if (!GetFileAttributesEx(fileName.c_str(), GetFileExInfoStandard, &fileInfo)) {
        cerr << "Error: Could not get file attributes for file: " << fileName << endl;
        return false;
    }

    LARGE_INTEGER currentFileSize;
    currentFileSize.LowPart = fileInfo.nFileSizeLow;
    currentFileSize.HighPart = fileInfo.nFileSizeHigh;

    bool changed = currentFileSize.QuadPart != previousFileSize.QuadPart;
    previousFileSize = currentFileSize;
    return changed;
}

vector<vector<string>> readCsvFile(const string& fileName) {
    ifstream inFile(fileName);
    if (!inFile.is_open()) {
        cerr << "Failed to open the file: " << fileName << endl;
        exit(1);
    }

    vector<vector<string>> data;
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        vector<string> row;
        string cell;
        bool inQuotes = false;
        stringstream cellStream;
        char c;
        
        while (ss.get(c)) {
            if (c == '\"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                row.push_back(cellStream.str());
                cellStream.str("");
                cellStream.clear();
            } else {
                cellStream << c;
            }
        }

        row.push_back(cellStream.str());
        data.push_back(row);
    }
    inFile.close();
    return data;
}

LRESULT CALLBACK InputEditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC oldWndProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (uMsg == WM_CHAR && wParam == VK_RETURN) {
        // Send a button click message to the Send button
        HWND parentHwnd = GetParent(hwnd);
        SendMessage(parentHwnd, WM_COMMAND, MAKEWPARAM(ID_SEND_BUTTON, BN_CLICKED), (LPARAM)GetDlgItem(parentHwnd, ID_SEND_BUTTON));
        return 0;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            HWND outputEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_WANTRETURN | ES_AUTOVSCROLL | WS_VSCROLL, 10, 10, 760, 450, hwnd, (HMENU)ID_OUTPUT_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            HWND inputEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 10, 470, 660, 30, hwnd, (HMENU)ID_INPUT_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            WNDPROC oldInputEditWndProc = (WNDPROC)SetWindowLongPtr(inputEdit, GWLP_WNDPROC, (LONG_PTR)InputEditWndProc);
            SetWindowLongPtr(inputEdit, GWLP_USERDATA, (LONG_PTR)oldInputEditWndProc);
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
            SendMessage(outputEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            CreateWindow("BUTTON", "Send", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 680, 470, 100, 30, hwnd, (HMENU)ID_SEND_BUTTON, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            return 0;
        }

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            SetWindowPos(GetDlgItem(hwnd, ID_OUTPUT_EDIT), NULL, 10, 10, width - 20, height - 60, SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd, ID_INPUT_EDIT), NULL, 10, height - 40, width - 120, 30, SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd, ID_SEND_BUTTON), NULL, width - 100, height - 40, 100, 30, SWP_NOZORDER);

            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_SEND_BUTTON && HIWORD(wParam) == BN_CLICKED) {
                // Handle send button click
                char inputText[1024];
                GetDlgItemText(hwnd, ID_INPUT_EDIT, inputText, 1024);

                // Append the message to the CSV file
                appendMessageToCsv(fileName, inputText, getCurrentTime(), getUserName());

                // Clear the input edit control
                SetDlgItemText(hwnd, ID_INPUT_EDIT, "");
            }
            return 0;
        }

        case WM_VSCROLL: {
            HWND outputEdit = GetDlgItem(hwnd, ID_OUTPUT_EDIT);
            int nScrollCode = (int)LOWORD(wParam);

            SCROLLINFO si;
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            GetScrollInfo(outputEdit, SB_VERT, &si);

            int yPos = si.nPos;
            switch (nScrollCode) {
                case SB_LINEUP:
                    si.nPos -= 1;
                    break;
                case SB_LINEDOWN:
                    si.nPos += 1;
                    break;
                case SB_PAGEUP:
                    si.nPos -= si.nPage;
                    break;
                case SB_PAGEDOWN:
                    si.nPos += si.nPage;
                    break;
                case SB_THUMBTRACK:
                    si.nPos = si.nTrackPos;
                    break;
                default:
                    break;
            }

            si.fMask = SIF_POS;
            SetScrollInfo(outputEdit, SB_VERT, &si, TRUE);
            int newPos = GetScrollPos(outputEdit, SB_VERT);
            ScrollWindowEx(outputEdit, 0, yPos - newPos, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
            UpdateWindow(outputEdit);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Redraw all child windows
            RedrawWindow(GetDlgItem(hwnd, ID_OUTPUT_EDIT), NULL, NULL, RDW_INVALIDATE);
            RedrawWindow(GetDlgItem(hwnd, ID_INPUT_EDIT), NULL, NULL, RDW_INVALIDATE);
            RedrawWindow(GetDlgItem(hwnd, ID_SEND_BUTTON), NULL, NULL, RDW_INVALIDATE);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void updateOutputEditControl(HWND hwnd) {
    auto chatData = readCsvFile(fileName);

    stringstream ss;

    const int usernameWidth = 10;
    const int messageWidth = 60;
    const int timestampWidth = 20;

    for (const auto& row : chatData) {
        string username = row[0];
        string message = row[1];
        string timestamp = row[2];

        username.resize(usernameWidth, ' ');
        message.resize(messageWidth, ' ');
        timestamp.resize(timestampWidth, ' ');

        ss << username << message << timestamp << endl;
    }

    SetDlgItemText(hwnd, ID_OUTPUT_EDIT, ss.str().c_str());

    // Scroll to the bottom
    HWND outputEdit = GetDlgItem(hwnd, ID_OUTPUT_EDIT);
    int textLength = GetWindowTextLength(outputEdit);
    SendMessage(outputEdit, EM_SETSEL, textLength, textLength);
    SendMessage(outputEdit, EM_SCROLLCARET, 0, 0);
}


void timerFunction(reference_wrapper<HWND> hwnd) {
    while (true) {

        if (isFileSizeChanged(fileName, previousFileSize)) {
            updateOutputEditControl(hwnd);
        }
        
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

// Add the necessary functions from the provided code here

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register the window class
    const char CLASS_NAME[] = "ChatAppClass";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

    RegisterClassEx(&wc);

    // Create the main window
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "NGChat", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Start the timer function in a separate thread
    thread timerThread(timerFunction, ref(hwnd));
    timerThread.detach();

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
