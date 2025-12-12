// WindowsPlatform.cpp — Windows-specific platform implementation

#include "WindowsPlatform.h"
#include "../../resource.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

WindowsPlatform::WindowsPlatform(HINSTANCE hInstance)
    : m_hInstance(hInstance)
    , m_soundEnabled(true)
{
    QueryPerformanceFrequency(&m_frequency);
}

WindowsPlatform::~WindowsPlatform() {
}

void WindowsPlatform::PlaySound(SoundType type) {
    if (!m_soundEnabled) return;
    
    int resourceId = (type == SoundType::FloorBounce) ? BOINGF : BOINGW;
    // Use PlaySoundW directly since we're using UNICODE and undef'd the macro
    PlaySoundW(MAKEINTRESOURCEW(resourceId), m_hInstance, SND_RESOURCE | SND_ASYNC);
}

double WindowsPlatform::GetHighResolutionTime() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)m_frequency.QuadPart;
}

void WindowsPlatform::SaveConfig(const BoingConfig& config) {
    WriteRegistryInt(L"FloorShadow", config.enableFloorShadow ? 1 : 0);
    WriteRegistryInt(L"WallShadow", config.enableWallShadow ? 1 : 0);
    WriteRegistryInt(L"Grid", config.enableGrid ? 1 : 0);
    WriteRegistryInt(L"Sound", config.enableSound ? 1 : 0);
    WriteRegistryInt(L"SmoothGeometry", config.smoothGeometry ? 1 : 0);
    WriteRegistryInt(L"BgColorR", config.bgColorR);
    WriteRegistryInt(L"BgColorG", config.bgColorG);
    WriteRegistryInt(L"BgColorB", config.bgColorB);
}

BoingConfig WindowsPlatform::LoadConfig() {
    BoingConfig config;
    config.enableFloorShadow = ReadRegistryInt(L"FloorShadow", 1) != 0;
    config.enableWallShadow = ReadRegistryInt(L"WallShadow", 1) != 0;
    config.enableGrid = ReadRegistryInt(L"Grid", 1) != 0;
    config.enableSound = ReadRegistryInt(L"Sound", 1) != 0;
    config.smoothGeometry = ReadRegistryInt(L"SmoothGeometry", 1) != 0;
    config.bgColorR = static_cast<unsigned char>(ReadRegistryInt(L"BgColorR", 192));
    config.bgColorG = static_cast<unsigned char>(ReadRegistryInt(L"BgColorG", 192));
    config.bgColorB = static_cast<unsigned char>(ReadRegistryInt(L"BgColorB", 192));
    
    m_soundEnabled = config.enableSound;
    return config;
}

void WindowsPlatform::WriteRegistryInt(const wchar_t* name, int value) {
    HKEY hKey;
    LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, 
        L"Software\\BoingBallSaver", 
        0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    
    if (result == ERROR_SUCCESS) {
        RegSetValueEx(hKey, name, 0, REG_DWORD, 
            (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

int WindowsPlatform::ReadRegistryInt(const wchar_t* name, int defaultValue) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, 
        L"Software\\BoingBallSaver", 
        0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        DWORD value = defaultValue;
        DWORD size = sizeof(DWORD);
        DWORD type = REG_DWORD;
        
        result = RegQueryValueEx(hKey, name, NULL, &type, 
            (BYTE*)&value, &size);
        RegCloseKey(hKey);
        
        if (result == ERROR_SUCCESS) {
            return (int)value;
        }
    }
    
    return defaultValue;
}
