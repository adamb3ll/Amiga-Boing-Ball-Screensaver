// WindowsPlatform.h — Windows-specific platform implementation

#pragma once

#include "../core/Platform.h"
#include <windows.h>

class WindowsPlatform : public IPlatform {
public:
    WindowsPlatform(HINSTANCE hInstance);
    virtual ~WindowsPlatform();
    
    // IPlatform implementation
    virtual void PlaySound(SoundType type) override;
    virtual double GetHighResolutionTime() override;
    virtual void SaveConfig(const BoingConfig& config) override;
    virtual BoingConfig LoadConfig() override;
    virtual const char* GetPlatformName() const override { return "Windows"; }

private:
    HINSTANCE m_hInstance;
    LARGE_INTEGER m_frequency;
    bool m_soundEnabled;
    
    // Registry helpers
    void WriteRegistryInt(const wchar_t* name, int value);
    int ReadRegistryInt(const wchar_t* name, int defaultValue);
};
