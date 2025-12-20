// MacPlatform.mm — macOS-specific platform implementation

#include "MacPlatform.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <mach/mach_time.h>

#ifndef DEBUG
#define DEBUG 0
#endif

// Global shared flag to disable sounds across ALL instances and processes
// This is critical because macOS can create multiple screensaver instances
// (e.g., for wallpaper integration), potentially in different processes
// We use NSUserDefaults so it persists across processes
static NSString* const kSoundGloballyDisabledKey = @"BoingBallSaver_SoundGloballyDisabled";
static NSLock* g_soundLock = nil;

// Initialize the lock once
static void InitSoundLock() {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        g_soundLock = [[NSLock alloc] init];
    });
}

// Check if sounds are globally disabled (persists across processes)
static bool IsSoundGloballyDisabled() {
    [g_soundLock lock];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    bool disabled = [defaults boolForKey:kSoundGloballyDisabledKey];
    [g_soundLock unlock];
    return disabled;
}

// Set global sound disable flag (persists across processes)
static void SetSoundGloballyDisabled(bool disabled) {
    [g_soundLock lock];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setBool:disabled forKey:kSoundGloballyDisabledKey];
    [defaults synchronize];  // Force immediate write
    [g_soundLock unlock];
}

// Clear the global disable flag - allows sounds to play
// No coordination between instances - each instance enables sounds independently
static void ClearSoundDisabled() {
    [g_soundLock lock];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setBool:false forKey:kSoundGloballyDisabledKey];
    [defaults synchronize];
    [g_soundLock unlock];
}

MacPlatform::MacPlatform()
    : m_floorSound(nil)
    , m_wallSound(nil)
    , m_soundEnabled(false)  // Start disabled - EnableSounds() will enable
{
    InitSoundLock();
    LoadSounds();
    // Clear the global disable flag on construction - ensures sounds can play
    // This fixes the issue where the flag persists from a previous exit
    ClearSoundDisabled();
}

MacPlatform::~MacPlatform() {
    if (m_floorSound) {
        [m_floorSound release];
        m_floorSound = nil;
    }
    if (m_wallSound) {
        [m_wallSound release];
        m_wallSound = nil;
    }
}

void MacPlatform::LoadSounds() {
    @autoreleasepool {
        // CRITICAL: Release existing sounds first to prevent memory leaks
        // This can happen if LoadSounds() is called multiple times (e.g., from PlaySound)
        if (m_floorSound) {
            [m_floorSound release];
            m_floorSound = nil;
        }
        if (m_wallSound) {
            [m_wallSound release];
            m_wallSound = nil;
        }
        
        // Load sounds from bundle resources
        // For screensavers, we need to find the .saver bundle
        // Try multiple bundle lookup methods to find the screensaver bundle
        NSBundle* bundle = nil;
        
        // First, try to find by identifier (most reliable for .saver bundles)
        bundle = [NSBundle bundleWithIdentifier:@"com.adamb3ll.BoingBallSaver"];
        
        if (!bundle) {
            // Try to find the bundle containing the screensaver view class
            Class saverClass = NSClassFromString(@"MacBoingBallView");
            if (saverClass) {
                bundle = [NSBundle bundleForClass:saverClass];
            }
        }
        
        if (!bundle) {
            // Fallback: try main bundle (works for standalone apps)
            bundle = [NSBundle mainBundle];
        }
        
        if (!bundle) {
#if DEBUG
            NSLog(@"MacPlatform: ERROR - Could not find screensaver bundle for sound loading");
#endif
            return;
        }
        
#if DEBUG
        NSString* bundlePath = [bundle bundlePath];
        NSString* resourcesPath = [bundle resourcePath];
        NSLog(@"MacPlatform: Loading sounds from bundle: %@", bundlePath);
        NSLog(@"MacPlatform: Resources path: %@", resourcesPath);
#endif
        
        NSString* floorPath = [bundle pathForResource:@"BoingBallF" ofType:@"wav"];
        if (floorPath) {
#if DEBUG
            NSLog(@"MacPlatform: Found floor sound at: %@", floorPath);
#endif
            m_floorSound = [[NSSound alloc] initWithContentsOfFile:floorPath byReference:NO];
#if DEBUG
            if (m_floorSound) {
                NSLog(@"MacPlatform: Floor sound loaded successfully");
            } else {
                NSLog(@"MacPlatform: ERROR - Failed to create NSSound from floor path");
            }
#endif
        } else {
#if DEBUG
            NSLog(@"MacPlatform: ERROR - Could not find BoingBallF.wav in bundle resources");
#endif
            // Try direct path as fallback
#if DEBUG
            NSString* bundlePath = [bundle bundlePath];
            NSString* directPath = [NSString stringWithFormat:@"%@/Contents/Resources/BoingBallF.wav", bundlePath];
#else
            NSString* directPath = [NSString stringWithFormat:@"%@/Contents/Resources/BoingBallF.wav", [bundle bundlePath]];
#endif
            if ([[NSFileManager defaultManager] fileExistsAtPath:directPath]) {
#if DEBUG
                NSLog(@"MacPlatform: Trying direct path: %@", directPath);
#endif
                m_floorSound = [[NSSound alloc] initWithContentsOfFile:directPath byReference:NO];
#if DEBUG
                if (m_floorSound) {
                    NSLog(@"MacPlatform: Floor sound loaded from direct path");
                }
#endif
            }
        }
        
        NSString* wallPath = [bundle pathForResource:@"BoingBallW" ofType:@"wav"];
        if (wallPath) {
#if DEBUG
            NSLog(@"MacPlatform: Found wall sound at: %@", wallPath);
#endif
            m_wallSound = [[NSSound alloc] initWithContentsOfFile:wallPath byReference:NO];
#if DEBUG
            if (m_wallSound) {
                NSLog(@"MacPlatform: Wall sound loaded successfully");
            } else {
                NSLog(@"MacPlatform: ERROR - Failed to create NSSound from wall path");
            }
#endif
        } else {
#if DEBUG
            NSLog(@"MacPlatform: ERROR - Could not find BoingBallW.wav in bundle resources");
#endif
            // Try direct path as fallback
#if DEBUG
            NSString* bundlePath = [bundle bundlePath];
            NSString* directPath = [NSString stringWithFormat:@"%@/Contents/Resources/BoingBallW.wav", bundlePath];
#else
            NSString* directPath = [NSString stringWithFormat:@"%@/Contents/Resources/BoingBallW.wav", [bundle bundlePath]];
#endif
            if ([[NSFileManager defaultManager] fileExistsAtPath:directPath]) {
#if DEBUG
                NSLog(@"MacPlatform: Trying direct path: %@", directPath);
#endif
                m_wallSound = [[NSSound alloc] initWithContentsOfFile:directPath byReference:NO];
#if DEBUG
                if (m_wallSound) {
                    NSLog(@"MacPlatform: Wall sound loaded from direct path");
                }
#endif
            }
        }
    }
}

void MacPlatform::PlaySound(SoundType type) {
    // Check instance sound is enabled before playing
    // No global flag check - each instance manages its own sounds independently
    if (!m_soundEnabled) {
        return;
    }
    
    @autoreleasepool {
        // Reload sounds if they were released
        if (!m_floorSound || !m_wallSound) {
            if (m_soundEnabled) {
                LoadSounds();
            } else {
                return;
            }
        }
        
        NSSound* sound = (type == SoundType::FloorBounce) ? m_floorSound : m_wallSound;
        if (sound) {
            // Check again inside autoreleasepool (in case it was disabled between checks)
            if (!m_soundEnabled) {
                return;
            }
            
            // Stop any previous playback first
            if ([sound isPlaying]) {
                [sound stop];
            }
            
            // Final check before playing
            if (!m_soundEnabled) {
                return;
            }
            
            [sound play];
        }
    }
}

void MacPlatform::StopAllSounds() {
    @autoreleasepool {
        // Force stop all sounds aggressively
        // NSSound can continue playing asynchronously, so we need to be thorough
        if (m_floorSound) {
            // Stop regardless of playing state - stop() is safe to call multiple times
            [m_floorSound stop];
            // Force synchronous stop by waiting for it to actually stop (with timeout)
            NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:0.1]; // 100ms max wait
            int attempts = 0;
            while ([m_floorSound isPlaying] && [timeout timeIntervalSinceNow] > 0 && attempts < 20) {
                [m_floorSound stop]; // Call stop again
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
                attempts++;
            }
        }
        if (m_wallSound) {
            // Stop regardless of playing state
            [m_wallSound stop];
            // Force synchronous stop by waiting for it to actually stop (with timeout)
            NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:0.1]; // 100ms max wait
            int attempts = 0;
            while ([m_wallSound isPlaying] && [timeout timeIntervalSinceNow] > 0 && attempts < 20) {
                [m_wallSound stop]; // Call stop again
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
                attempts++;
            }
        }
    }
}

void MacPlatform::ReleaseSounds() {
    @autoreleasepool {
        // Stop and release sounds completely
        if (m_floorSound) {
            [m_floorSound stop];
            [m_floorSound release];
            m_floorSound = nil;
        }
        if (m_wallSound) {
            [m_wallSound stop];
            [m_wallSound release];
            m_wallSound = nil;
        }
    }
}

void MacPlatform::DisableSounds() {
    // Set GLOBAL flag FIRST - this prevents ALL instances/processes from playing sounds
    // Uses NSUserDefaults so it persists across processes
    SetSoundGloballyDisabled(true);
    
    // Stop all sounds first
    StopAllSounds();
    // Then disable sound playback entirely for this instance
    m_soundEnabled = false;
    // Release sounds completely to ensure they stop
    ReleaseSounds();
}

void MacPlatform::EnableSounds() {
    // Clear the global disable flag - allows sounds to play
    // No coordination between instances - each instance enables sounds independently
    ClearSoundDisabled();
    
    // Get user's sound preference directly
    bool userWantsSounds = ReadPref(@"Sound", 1) != 0;
    
    // Enable sounds for this instance if user wants them
    m_soundEnabled = userWantsSounds;
}

double MacPlatform::GetHighResolutionTime() {
    static mach_timebase_info_data_t timebase;
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        mach_timebase_info(&timebase);
    });
    
    uint64_t time = mach_absolute_time();
    double nanoseconds = (double)time * timebase.numer / timebase.denom;
    return nanoseconds / 1.0e9;  // Convert to seconds
}

void MacPlatform::SaveConfig(const BoingConfig& config) {
    WritePref(@"FloorShadow", config.enableFloorShadow ? 1 : 0);
    WritePref(@"WallShadow", config.enableWallShadow ? 1 : 0);
    WritePref(@"Grid", config.enableGrid ? 1 : 0);
    WritePref(@"Sound", config.enableSound ? 1 : 0);
    WritePref(@"SmoothGeometry", config.smoothGeometry ? 1 : 0);
    WritePref(@"BallLighting", config.enableBallLighting ? 1 : 0);
    WritePref(@"ShowFPS", config.showFPS ? 1 : 0);
    WritePref(@"BgColorR", config.bgColorR);
    WritePref(@"BgColorG", config.bgColorG);
    WritePref(@"BgColorB", config.bgColorB);
    
    // Synchronize defaults
    [[NSUserDefaults standardUserDefaults] synchronize];
}

BoingConfig MacPlatform::LoadConfig() {
    BoingConfig config;
    config.enableFloorShadow = ReadPref(@"FloorShadow", 1) != 0;
    config.enableWallShadow = ReadPref(@"WallShadow", 1) != 0;
    config.enableGrid = ReadPref(@"Grid", 1) != 0;
    config.enableSound = ReadPref(@"Sound", 1) != 0;
    config.smoothGeometry = ReadPref(@"SmoothGeometry", 1) != 0;
    config.enableBallLighting = ReadPref(@"BallLighting", 1) != 0;
    config.showFPS = ReadPref(@"ShowFPS", 0) != 0;  // Default to off
    config.bgColorR = static_cast<unsigned char>(ReadPref(@"BgColorR", 192));
    config.bgColorG = static_cast<unsigned char>(ReadPref(@"BgColorG", 192));
    config.bgColorB = static_cast<unsigned char>(ReadPref(@"BgColorB", 192));
    
    // NOTE: m_soundEnabled is NOT set here - it's managed by EnableSounds()/DisableSounds()
    // This ensures the instance flag is only set when explicitly enabling/disabling sounds
    // No global flag check - each instance manages sounds independently
    
    return config;
}

void MacPlatform::WritePref(NSString* key, int value) {
    @autoreleasepool {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        NSString* prefKey = [NSString stringWithFormat:@"BoingBallSaver_%@", key];
        [defaults setInteger:value forKey:prefKey];
    }
}

int MacPlatform::ReadPref(NSString* key, int defaultValue) {
    @autoreleasepool {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        NSString* prefKey = [NSString stringWithFormat:@"BoingBallSaver_%@", key];
        
        if ([defaults objectForKey:prefKey] == nil) {
            return defaultValue;
        }
        
        return (int)[defaults integerForKey:prefKey];
    }
}
