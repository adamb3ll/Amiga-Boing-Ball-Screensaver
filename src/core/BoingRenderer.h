// BoingRenderer.h — Platform-independent OpenGL renderer for the Boing Ball
// Handles all OpenGL rendering including ball, shadows, and grid

#pragma once

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

class BoingPhysics;

struct RenderConfig {
    bool showFloorShadow;
    bool showWallShadow;
    bool showGrid;
    bool smoothGeometry;  // true = 64x32, false = 16x8 classic
    bool ballLightingEnabled;  // enable lighting on the ball (v1.3 feature)
    bool showFPS;  // show FPS counter in top-left corner
    float backgroundColor[3];  // RGB [0-1]
    
    RenderConfig()
        : showFloorShadow(true)
        , showWallShadow(true)
        , showGrid(true)
        , smoothGeometry(true)
        , ballLightingEnabled(true)  // default: lighting enabled
        , showFPS(false)  // default: FPS counter off
        , backgroundColor{0.75f, 0.75f, 0.75f}
    {}
};

class BoingRenderer {
public:
    BoingRenderer();
    ~BoingRenderer();
    
    // Initialize OpenGL state and resources
    // Must be called after OpenGL context is created
    void Initialize(int width, int height);
    
    // Clean up OpenGL resources
    void Cleanup();
    
    // Update viewport (for window resize)
    void SetViewport(int width, int height, float& outWallX, float& outWallZ, float& outFloorY);
    
    // Render a complete frame
    void RenderFrame(const BoingPhysics& physics, const RenderConfig& config, float deltaTime = 0.0f);
    
    // Configuration
    void SetConfig(const RenderConfig& config) { m_config = config; }
    const RenderConfig& GetConfig() const { return m_config; }

private:
    GLuint m_checkerTexture;
    RenderConfig m_config;
    int m_sphereSlices;
    int m_sphereStacks;
    
    // Cached GLU quadric (avoid creating/deleting every frame)
    GLUquadric* m_quadric;
    
    // FPS smoothing
    float m_fpsAccumulator;
    float m_fpsTimeAccumulator;
    int m_fpsFrameCount;
    float m_lastDisplayedFPS;  // Cache last displayed FPS to avoid unnecessary re-renders
    
    // Cached viewport for FPS rendering (avoid expensive glGetIntegerv call)
    int m_cachedViewportWidth;
    int m_cachedViewportHeight;
    
    // Rendering methods
    void CreateCheckerTexture();
    void SetupLighting();
    void SetupProjection(int width, int height, float& outWallX, float& outWallZ, float& outFloorY);
    
    void DrawGrid(float floorY);
    void DrawFloorShadow(float ballX, float ballY, float ballZ, float ballRadius, float floorY);
    void DrawWallShadow(float ballX, float ballY, float ballZ, float ballRadius);
    void DrawBall(float ballX, float ballY, float ballZ, float ballRadius, float spinAngle, bool lightingEnabled);
    void DrawSphere(float radius);
    void DrawFPS(float fps, int width, int height);
};
