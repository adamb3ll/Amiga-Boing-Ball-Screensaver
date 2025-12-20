// BoingConfig.h — Platform-independent configuration structure
// Defines all configurable options for the screensaver

#pragma once

struct BoingConfig {
    // Visual options
    bool enableFloorShadow;
    bool enableWallShadow;
    bool enableGrid;
    bool smoothGeometry;  // true = smooth 64x32, false = classic 16x8
    bool enableBallLighting;  // enable lighting on the ball (v1.3 feature)
    bool showFPS;  // show FPS counter in top-left corner
    
    // Audio options
    bool enableSound;
    
    // Background color (RGB, 0-255)
    unsigned char bgColorR;
    unsigned char bgColorG;
    unsigned char bgColorB;
    
    // Default constructor with sensible defaults
    BoingConfig()
        : enableFloorShadow(true)
        , enableWallShadow(true)
        , enableGrid(true)
        , smoothGeometry(true)
        , enableBallLighting(true)  // default: lighting enabled
        , showFPS(false)  // default: FPS counter off
        , enableSound(true)
        , bgColorR(192)
        , bgColorG(192)
        , bgColorB(192)
    {}
    
    // Get background color as normalized floats [0-1]
    void GetBackgroundColorFloat(float& r, float& g, float& b) const {
        r = bgColorR / 255.0f;
        g = bgColorG / 255.0f;
        b = bgColorB / 255.0f;
    }
    
    // Set background color from normalized floats [0-1]
    void SetBackgroundColorFloat(float r, float g, float b) {
        bgColorR = static_cast<unsigned char>(r * 255.0f);
        bgColorG = static_cast<unsigned char>(g * 255.0f);
        bgColorB = static_cast<unsigned char>(b * 255.0f);
    }
    
    // Restore factory defaults
    void RestoreDefaults() {
        *this = BoingConfig();
    }
};
