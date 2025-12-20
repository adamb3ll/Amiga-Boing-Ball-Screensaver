// Platform.h — Platform abstraction interface
// Defines the interface for platform-specific services

#pragma once

#include "BoingConfig.h"

// Sound types that can be played
enum class SoundType {
    FloorBounce,
    WallHit
};

// Abstract interface for platform-specific functionality
class IPlatform {
public:
    virtual ~IPlatform() {}
    
    // Audio
    virtual void PlaySound(SoundType type) = 0;
    
    // Time
    virtual double GetHighResolutionTime() = 0;
    
    // Configuration persistence
    virtual void SaveConfig(const BoingConfig& config) = 0;
    virtual BoingConfig LoadConfig() = 0;
    
    // Platform info (optional, for debugging)
    virtual const char* GetPlatformName() const = 0;
};
