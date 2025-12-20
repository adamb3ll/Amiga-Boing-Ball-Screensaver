// MacPlatform.h — macOS-specific platform implementation

#pragma once

#include "core/Platform.h"
#import <Foundation/Foundation.h>

@class NSSound;

class MacPlatform : public IPlatform {
public:
    MacPlatform();
    virtual ~MacPlatform();
    
    // IPlatform implementation
    virtual void PlaySound(SoundType type) override;
    virtual double GetHighResolutionTime() override;
    virtual void SaveConfig(const BoingConfig& config) override;
    virtual BoingConfig LoadConfig() override;
    virtual const char* GetPlatformName() const override { return "macOS"; }
    
    // Additional methods
    void StopAllSounds();
    void DisableSounds();  // Disable sound playback entirely
    void EnableSounds();   // Re-enable sound playback
    void ReleaseSounds();  // Release NSSound objects completely
    
private:
    NSSound* m_floorSound;
    NSSound* m_wallSound;
    bool m_soundEnabled;
    
    // Helper methods
    void LoadSounds();
    void WritePref(NSString* key, int value);
    int ReadPref(NSString* key, int defaultValue);
};
