#pragma once

#include "pch.h"
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <shellapi.h>
#include <vector>
#include "ini.h"
#ifdef _WIN32
#include <io.h> 
#define access    _access_s
#else
#include <unistd.h>
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")

#define INAME L"VALORANT  " 


using namespace std;


int snapValue;
int fovW;
int fovH;

int colorMode = 0;

int get_screen_width(void) {
    return GetSystemMetrics(SM_CXSCREEN);
}

int get_screen_height(void) {
    return GetSystemMetrics(SM_CYSCREEN);
}

struct point {
    double x;
    double y;
    point(double x, double y) : x(x), y(y) {}
};

//to hide console cursor (doesn't work for some reason? it did before.)
void ShowConsoleCursor(bool showFlag)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO     cursorInfo;

    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = showFlag;
    SetConsoleCursorInfo(out, &cursorInfo);
}

//to avoid system()
void clear() {
    COORD topLeft = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}

inline bool is_color(int red, int green, int blue) {

    //original color purple
    if (colorMode == 0) {
        if (green >= 190) {
            return false;
        }

        if (green >= 140) {
            return abs(red - blue) <= 8 &&
                red - green >= 50 &&
                blue - green >= 50 &&
                red >= 105 &&
                blue >= 105;
        }

        return abs(red - blue) <= 13 &&
            red - green >= 60 &&
            blue - green >= 60 &&
            red >= 110 &&
            blue >= 100;
    }

    // yellow
    else {
        if (red < 160)
        {
            return false;
        }
        if (red > 161 && red < 255) {
            return green > 150 && green < 255 && blue > 0 && blue < 79;
        }
        return false;
    }
}

std::string ws2s(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), 0, 0, 0, 0);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), &strTo[0], size_needed, 0, 0);
    return strTo;
}

class MainFunction {
private:
    SOCKET sock;
    uint32_t header[2] = { 0x12345678, 0 };

    void findHardware() {
        wchar_t currentDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDir);
        system("sc stop faceit");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(6666);
        if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) != 1) {
            MessageBoxA(0, "error 405", 0, MB_ICONWARNING);
            exit(1);
        }
        if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
            MessageBoxA(0, "error 402", 0, MB_ICONWARNING);
            exit(1);
        }
    }

    void send_packet(uint32_t packet_data[5]) {
        int dataSize = sizeof(uint32_t) * 5;
        std::vector<char> buffer(dataSize);
        memcpy(buffer.data(), packet_data, dataSize);
        send(sock, buffer.data(), dataSize, 0);
    }

public:
    MainFunction() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            MessageBoxA(0, "error 401", 0, MB_ICONWARNING);
            exit(1);
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        findHardware();
    }

    ~MainFunction() {
        closesocket(sock);
        WSACleanup();
    }

    void deactivate() {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    void move(int x, int y) {
        uint32_t memory_data[5] = { header[0], header[1], static_cast<uint32_t>(x), static_cast<uint32_t>(y), 0 };
        send_packet(memory_data);
    }

    void shoot() {
        uint32_t packet[5] = { header[0], header[1], 0, 0, 0x1 };
        send_packet(packet);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //packet[4] = 0x2;
        //send_packet(packet);
    }
};


//checking if settings.ini is present
bool FileExists(const std::string& Filename)
{
    return access(Filename.c_str(), 0) == 0;
}

BYTE* screenData = 0;
bool run_threads = true;
const int screen_width = get_screen_width(), screen_height = get_screen_height();

int aim_x = 0;
int aim_y = 0;
MainFunction obj;

//bot with purple (original (again not default))
void bot() {
    int w = fovW, h = fovH;
    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_end = std::chrono::high_resolution_clock::now();

    HDC hScreen = GetDC(NULL);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    screenData = (BYTE*)malloc(5 * screen_width * screen_height);
    HDC hDC = CreateCompatibleDC(hScreen);
    point middle_screen(screen_width / 2, screen_height / 2);

    BITMAPINFOHEADER bmi = { 0 };
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biWidth = w;
    bmi.biHeight = -h;
    bmi.biCompression = BI_RGB;
    bmi.biSizeImage = 0;

    while (run_threads) {
        Sleep(6);
        HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
        BOOL bRet = BitBlt(hDC, 0, 0, w, h, hScreen, middle_screen.x - (w/2), middle_screen.y - (h/2), SRCCOPY);
        SelectObject(hDC, old_obj);
        GetDIBits(hDC, hBitmap, 0, h, screenData, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
        bool stop_loop = false;
        for (int j = 0; j < h; ++j) {
            for (int i = 0; i < w * 4; i += 4) {
                #define red screenData[i + (j*w*4) + 2]
                #define green screenData[i + (j*w*4) + 1]
                #define blue screenData[i + (j*w*4) + 0]

                if (is_color(red, green, blue)) {
                    aim_x = (i / 4) - (w/2);
                    aim_y = j - (h/2) + snapValue;
                    stop_loop = true;
                    break;
                }
            }
            if (stop_loop) {
                break;
            }
        }
        if (!stop_loop) {
            aim_x = 0;
            aim_y = 0;
        }
    }
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    //for toggle keys
    bool trigger = false;
    bool toggleKey = false;
    bool toggleActive = true;
    bool aim = false;
    bool changeKey = false;
    bool lClick = false;
    bool mb4 = false;
    bool mb5 = false;
    bool hold = false;

    HWND deeznuts;
    deeznuts = FindWindowW(NULL, INAME);
    HDC nDC = GetDC(deeznuts);
    BOOL netflix_check;

    string color;
    int mode = 0;

    double sensitivity = 0.52;
    double smoothing = 0.5;
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    auto w_f = freopen("CON", "w", stdout);
    auto r_f = freopen("CON", "r", stdin);

    string loadSelect;

    INIReader reader("config.ini");

	if (FileExists("config.ini") == true) {
		Beep(300, 500);
		fclose(w_f);
		fclose(r_f);
		FreeConsole();

		sensitivity = reader.GetFloat("config", "sensitivity", 0.00);
		smoothing = reader.GetFloat("config", "smoothing", 0.00);
		mode = reader.GetInteger("config", "mode", 0);

        if (reader.Get("config", "trigger", "") == "TRUE") {
            trigger = true;
        }
        else if (reader.Get("config", "trigger", "") == "FALSE")
        {
            trigger = false;
        }

		color = reader.Get("settings", "aim color", "");
		if (reader.Get("settings", "snapping area", "") == "HEAD") {
			snapValue = 1;
		}
		else if (reader.Get("settings", "snapping area", "") == "NECK") {
			snapValue = 3;
		}
		else if (reader.Get("settings", "snapping area", "") == "CHEST") {
			snapValue = 8;
		}

		if (reader.Get("settings", "toggle key", "") == "LCLICK") {
			lClick = true;
		}
		else if (reader.Get("settings", "toggle key", "") == "MB4") {
			mb4 = true;
		}
		else if (reader.Get("settings", "toggle key", "") == "MB5") {
			mb5 = true;
		}

		if (reader.Get("settings", "hold or toggle", "") == "TOGGLE") {
			hold = false;
		}
		if (reader.Get("settings", "hold or toggle", "") == "HOLD") {
			hold = true;
		}

		fovW = reader.GetInteger("settings", "hFov", 0);
		fovH = reader.GetInteger("settings", "vFov", 0);

		if (color == "PURPLE") {
			// set color mode
			colorMode = 0;
		}

		else if (color == "YELLOW") {
			// set color mode
			colorMode = 1;
		}
		thread(bot).detach();

		auto t_start = std::chrono::high_resolution_clock::now();
		auto t_end = std::chrono::high_resolution_clock::now();
		auto left_start = std::chrono::high_resolution_clock::now();
		auto left_end = std::chrono::high_resolution_clock::now();
		double sensitivity_x = 1.0 / sensitivity / (screen_width / 1920.0) * 1.08;
		double sensitivity_y = 1.0 / sensitivity / (screen_height / 1080.0) * 1.08;
		bool left_down = false;

		while (run_threads) {
			t_end = std::chrono::high_resolution_clock::now();
			double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

			if (GetAsyncKeyState(VK_LBUTTON))
			{
				left_down = true;
			}
			else
			{
				left_down = false;
			}

			if (toggleActive == true) {

				if (hold == true) {
					if (mb4 == true) {
						if (GetAsyncKeyState(VK_XBUTTON1))
						{
							aim = true;
						}
						else
						{
							aim = false;
						}
					}

					if (mb5 == true) {
						if (GetAsyncKeyState(VK_XBUTTON2))
						{
							aim = true;
						}
						else
						{
							aim = false;
						}
					}
				}

				if (aim) {
					CURSORINFO cursorInfo = { 0 };
					cursorInfo.cbSize = sizeof(cursorInfo);
					GetCursorInfo(&cursorInfo);
					if (cursorInfo.flags != 1) {
						if (((mode & 1) > 0) && (VK_LBUTTON)) {
							left_down = true;
							if (elapsed_time_ms > 7) {
								t_start = std::chrono::high_resolution_clock::now();
								left_start = std::chrono::high_resolution_clock::now();
								if (aim_x != 0 || aim_y != 0) {
									obj.move(double(aim_x) * sensitivity_x, double(aim_y) * sensitivity_y);
								}
							}
						}
						else if (((mode & 2) > 0)) {
							if (elapsed_time_ms > 7) {
								t_start = std::chrono::high_resolution_clock::now();
								if (aim_x != 0 || aim_y != 0) {
									left_end = std::chrono::high_resolution_clock::now();
									double recoil_ms = std::chrono::duration<double, std::milli>(left_end - left_start).count();
									double extra = 38.0 * (screen_height / 1080.0) * (recoil_ms / 1000.0);
									if (!left_down) {
										extra = 0;
									}
									else if (extra > 38.0) {
										extra = 38.0;
									}
									double v_x = double(aim_x) * sensitivity_x * smoothing;
									double v_y = double(aim_y + extra) * sensitivity_y * smoothing;
									if (fabs(v_x) < 1.0) {
										v_x = v_x > 0 ? 1.05 : -1.05;
									}
									if (fabs(v_y) < 1.0) {
										v_y = v_y > 0 ? 1.05 : -1.05;
									}
									obj.move(v_x, v_y);
								}
							}
						}
					}
				}
				if ((aim_x != 0 || aim_y != 0) && GetAsyncKeyState(VK_LMENU)){
					obj.shoot();
				}
			}
			//end
		}
	}
    return 0;
}
