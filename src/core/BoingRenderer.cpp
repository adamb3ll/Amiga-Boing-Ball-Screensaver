// BoingRenderer.cpp — Platform-independent OpenGL rendering implementation

#include "BoingRenderer.h"
#include "BoingPhysics.h"
#include <cmath>
#include <cstdio>
#include <cstring>

BoingRenderer::BoingRenderer()
    : m_checkerTexture(0)
    , m_sphereSlices(32)
    , m_sphereStacks(32)
    , m_quadric(nullptr)
    , m_fpsAccumulator(0.0f)
    , m_fpsTimeAccumulator(0.0f)
    , m_fpsFrameCount(0)
    , m_lastDisplayedFPS(0.0f)
    , m_cachedViewportWidth(0)
    , m_cachedViewportHeight(0)
{
}

BoingRenderer::~BoingRenderer() {
    Cleanup();
}

void BoingRenderer::Initialize(int width, int height) {
    // Reset FPS accumulator state (in case renderer is reused)
    m_fpsAccumulator = 0.0f;
    m_fpsTimeAccumulator = 0.0f;
    m_fpsFrameCount = 0;
    m_lastDisplayedFPS = 0.0f;
    
    // Reset cached viewport
    m_cachedViewportWidth = 0;
    m_cachedViewportHeight = 0;
    
    // Clear any existing OpenGL errors from previous runs
    while (glGetError() != GL_NO_ERROR) {
        // Clear error queue
    }
    
    // Reset OpenGL state to known defaults
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    
    // Now set up our desired state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    SetupLighting();
    CreateCheckerTexture();
    
    // Create cached quadric for sphere rendering (reused every frame)
    // Ensure it's clean - delete old one if it exists (shouldn't happen, but safety)
    if (m_quadric) {
        gluDeleteQuadric(m_quadric);
        m_quadric = nullptr;
    }
    m_quadric = gluNewQuadric();
    gluQuadricTexture(m_quadric, GL_TRUE);
    
    float wallX, wallZ, floorY;
    SetViewport(width, height, wallX, wallZ, floorY);
    // Viewport dimensions are cached in SetViewport
}

void BoingRenderer::Cleanup() {
    if (m_checkerTexture) {
        glDeleteTextures(1, &m_checkerTexture);
        m_checkerTexture = 0;
    }
    if (m_quadric) {
        gluDeleteQuadric(m_quadric);
        m_quadric = nullptr;
    }
}

void BoingRenderer::SetupLighting() {
    GLfloat lightDir[] = { -0.5f, 0.8f, 0.6f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
    
    // Global ambient light
    GLfloat globalAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    
    // Per-light ambient boost
    GLfloat ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
}

void BoingRenderer::CreateCheckerTexture() {
    // Delete existing texture if it exists (defensive - prevents texture leak)
    if (m_checkerTexture != 0) {
        glDeleteTextures(1, &m_checkerTexture);
        m_checkerTexture = 0;
    }
    
    const int TEX_SIZE = 128;
    unsigned char* data = new unsigned char[TEX_SIZE * TEX_SIZE * 3];
    
    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int cx = x / (TEX_SIZE / 16);
            int cy = y / (TEX_SIZE / 8);
            bool red = ((cx + cy) % 2) == 0;
            unsigned char r = red ? 220 : 240;
            unsigned char g = red ? 30 : 240;
            unsigned char b = red ? 30 : 240;
            int i = (y * TEX_SIZE + x) * 3;
            data[i + 0] = r;
            data[i + 1] = g;
            data[i + 2] = b;
        }
    }
    
    glGenTextures(1, &m_checkerTexture);
    glBindTexture(GL_TEXTURE_2D, m_checkerTexture);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, TEX_SIZE, TEX_SIZE, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
    
    // Texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void BoingRenderer::SetViewport(int width, int height, float& outWallX, float& outWallZ, float& outFloorY) {
    // Ensure valid viewport dimensions
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    
    // Set OpenGL viewport - always use (0,0) as origin (view-relative coordinates)
    glViewport(0, 0, width, height);
    SetupProjection(width, height, outWallX, outWallZ, outFloorY);
    // Cache viewport dimensions for FPS rendering (avoid expensive glGetIntegerv call)
    m_cachedViewportWidth = width;
    m_cachedViewportHeight = height;
}

void BoingRenderer::SetupProjection(int width, int height, float& outWallX, float& outWallZ, float& outFloorY) {
    // Ensure valid dimensions
    if (width <= 0 || height <= 0) {
        width = 1;
        height = 1;
    }
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Calculate aspect ratio - handle extreme ratios (ultrawide, portrait)
    float aspect = (float)width / (float)height;
    
    // Clamp aspect ratio to reasonable bounds to prevent numerical issues
    // This shouldn't affect rendering but prevents division by zero or extreme values
    if (aspect < 0.1f) aspect = 0.1f;
    if (aspect > 10.0f) aspect = 10.0f;
    
    gluPerspective(45.0, aspect, 0.1, 50.0);
    
    // Compute dynamic bounds based on FOV and aspect ratio
    float fovRadians = 45.0f * (3.14159265f / 180.0f);
    float camDist = 2.0f; // matches camera distance in RenderFrame
    
    float halfHeight = tanf(fovRadians / 2.0f) * camDist;
    float halfWidth = halfHeight * aspect;
    
    outWallX = halfWidth;
    outWallZ = halfWidth;
    outFloorY = -halfHeight;
}

void BoingRenderer::RenderFrame(const BoingPhysics& physics, const RenderConfig& config, float deltaTime) {
    // Clear with background color
    glClearColor(config.backgroundColor[0], config.backgroundColor[1], config.backgroundColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2.0f);
    
    // Update geometry tessellation based on config
    if (config.smoothGeometry) {
        m_sphereSlices = 64;
        m_sphereStacks = 32;
    } else {
        m_sphereSlices = 16;
        m_sphereStacks = 8;
    }
    
    // Draw grid if enabled
    if (config.showGrid) {
        DrawGrid(physics.GetFloorY());
    }
    
    // Draw shadows if enabled
    if (config.showFloorShadow) {
        DrawFloorShadow(physics.GetBallX(), physics.GetBallY(), physics.GetBallZ(), 
                       physics.GetBallRadius(), physics.GetFloorY());
    }
    
    if (config.showWallShadow) {
        DrawWallShadow(physics.GetBallX(), physics.GetBallY(), physics.GetBallZ(), 
                      physics.GetBallRadius());
    }
    
    // Draw the ball
    DrawBall(physics.GetBallX(), physics.GetBallY(), physics.GetBallZ(), 
            physics.GetBallRadius(), physics.GetSpinAngle(), config.ballLightingEnabled);
    
    // Draw FPS counter if enabled
    if (config.showFPS && deltaTime > 0.0f) {
        // Cap deltaTime to prevent unrealistic FPS values
        // Minimum deltaTime of 0.0083 seconds = max 120 FPS (reasonable for screensavers)
        // Target is 60 FPS (0.0167 seconds), so cap at 120 FPS max
        float clampedDeltaTime = deltaTime;
        if (clampedDeltaTime < 0.0083f) clampedDeltaTime = 0.0083f;  // Max 120 FPS
        if (clampedDeltaTime > 0.1f) clampedDeltaTime = 0.1f;  // Cap very large deltas too
        
        // Smooth FPS over multiple frames (average over ~1 second for more stable display)
        m_fpsTimeAccumulator += clampedDeltaTime;
        float frameFPS = 1.0f / clampedDeltaTime;
        // Cap individual frame FPS to reasonable maximum (120 FPS)
        if (frameFPS > 120.0f) frameFPS = 120.0f;
        m_fpsAccumulator += frameFPS;
        m_fpsFrameCount++;
        
        // Update display every ~1 second or every 60 frames, whichever comes first
        // Longer smoothing window = more stable display
        float smoothedFPS = 0.0f;
        if (m_fpsTimeAccumulator >= 1.0f || m_fpsFrameCount >= 60) {
            smoothedFPS = m_fpsAccumulator / m_fpsFrameCount;
            // Final cap to ensure reasonable display value (max 120 FPS)
            if (smoothedFPS > 120.0f) smoothedFPS = 120.0f;
            m_fpsAccumulator = 0.0f;
            m_fpsTimeAccumulator = 0.0f;
            m_fpsFrameCount = 0;
        } else if (m_fpsFrameCount > 0) {
            // Use current average for display
            smoothedFPS = m_fpsAccumulator / m_fpsFrameCount;
            // Cap current average too
            if (smoothedFPS > 120.0f) smoothedFPS = 120.0f;
        }
        
        if (smoothedFPS > 0.0f) {
            // Get viewport dimensions - use cached if available, otherwise query (fallback)
            int viewportWidth = m_cachedViewportWidth;
            int viewportHeight = m_cachedViewportHeight;
            
            // Fallback: if cached viewport not set, query it (shouldn't happen, but safety check)
            if (viewportWidth <= 0 || viewportHeight <= 0) {
                GLint viewport[4];
                glGetIntegerv(GL_VIEWPORT, viewport);
                viewportWidth = viewport[2];
                viewportHeight = viewport[3];
                // Cache for next time
                m_cachedViewportWidth = viewportWidth;
                m_cachedViewportHeight = viewportHeight;
            }
            
            if (viewportWidth > 0 && viewportHeight > 0) {
                // Always render FPS counter every frame (screen gets cleared each frame)
                // Only update cached value if it changed significantly
                if (m_lastDisplayedFPS == 0.0f || fabs(smoothedFPS - m_lastDisplayedFPS) > 0.1f) {
                    m_lastDisplayedFPS = smoothedFPS;
                }
                DrawFPS(smoothedFPS, viewportWidth, viewportHeight);
            }
        }
    }
}

void BoingRenderer::DrawGrid(float floorY) {
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.6f, 1.0f);  // cyan grid lines
    glLineWidth(2.0f);
    
    // Grid floor
    glBegin(GL_LINES);
    for (float i = -1.0f; i <= 1.0f; i += 0.2f) {
        // X lines
        glVertex3f(i, floorY, -1.0f);
        glVertex3f(i, floorY, 1.0f);
        
        // Z lines
        glVertex3f(-1.0f, floorY, i);
        glVertex3f(1.0f, floorY, i);
    }
    glEnd();
    
    // Grid wall
    glBegin(GL_LINES);
    
    // Vertical lines (X direction)
    for (float x = -1.0f; x <= 1.0f; x += 0.2f) {
        glVertex3f(x, floorY, -1.0f);
        glVertex3f(x, floorY + 2.0f, -1.0f);
    }
    
    // Horizontal lines (Y direction)
    for (float y = floorY; y <= floorY + 2.0f; y += 0.2f) {
        glVertex3f(-1.0f, y, -1.0f);
        glVertex3f(1.0f, y, -1.0f);
    }
    
    glEnd();
}

void BoingRenderer::DrawFloorShadow(float ballX, float ballY, float ballZ, float ballRadius, float floorY) {
    glDisable(GL_LIGHTING);
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);  // semi-transparent black
    
    glPushMatrix();
    glTranslatef(ballX, floorY + 0.001f, ballZ);
    glScalef(1.0f, 0.1f, 1.0f);  // flatten into ellipse
    DrawSphere(ballRadius);
    glPopMatrix();
}

void BoingRenderer::DrawWallShadow(float ballX, float ballY, float ballZ, float ballRadius) {
    glDisable(GL_LIGHTING);
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);  // softer shadow
    
    glPushMatrix();
    glTranslatef(ballX, ballY, -1.0f);
    glScalef(1.0f, 1.0f, 0.1f);  // flatten into disk
    DrawSphere(ballRadius);
    glPopMatrix();
}

void BoingRenderer::DrawBall(float ballX, float ballY, float ballZ, float ballRadius, float spinAngle, bool lightingEnabled) {
    if (lightingEnabled) {
    glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);  // full bright texture when lighting disabled
    }
    
    glPushMatrix();
    glTranslatef(ballX, ballY, ballZ);
    
    // Initial orientation
    glRotatef(90.0f, 1, 0, 0);   // rotate 90° around X
    glRotatef(-15.0f, 0, 1, 0);  // rotate 15° around Y
    glRotatef(spinAngle, 0, 0, 1);   // dynamic spin around Z
    
    DrawSphere(ballRadius);
    glPopMatrix();
    
    if (!lightingEnabled) {
        glEnable(GL_LIGHTING);  // restore lighting state
    }
}

void BoingRenderer::DrawSphere(float radius) {
    // Use cached quadric (created in Initialize, deleted in Cleanup)
    // This avoids expensive create/delete every frame
    if (!m_quadric) {
        m_quadric = gluNewQuadric();
        gluQuadricTexture(m_quadric, GL_TRUE);
    }
    glBindTexture(GL_TEXTURE_2D, m_checkerTexture);
    gluSphere(m_quadric, radius, m_sphereSlices, m_sphereStacks);
}

void BoingRenderer::DrawFPS(float fps, int width, int height) {
    // Save current OpenGL state
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
    glPushMatrix();
    
    // Switch to 2D orthographic projection for text overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Disable depth testing and lighting for 2D text
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    
    // Set up for 2D rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Format FPS string with 2 decimal places
    char fpsText[16];
    snprintf(fpsText, sizeof(fpsText), "%.2f", fps);
    
    // Much larger size for readability - scale based on screen height
    float scale = height / 800.0f;  // Base scale for 800px height
    if (scale < 1.0f) scale = 1.0f;
    if (scale > 2.0f) scale = 2.0f;
    
    float charWidth = 20.0f * scale;
    float charHeight = 32.0f * scale;
    float strokeWidth = 4.0f * scale;
    
    // Position in top-left corner with padding
    float x = 20.0f * scale;
    float y = height - 40.0f * scale;
    
    // Calculate text width for background
    int textLen = strlen(fpsText);
    float textWidth = textLen * charWidth * 0.6f;  // Characters are narrower than full width
    
    // Draw semi-transparent background for readability
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(x - 10.0f * scale, y - charHeight - 10.0f * scale);
    glVertex2f(x + textWidth + 10.0f * scale, y - charHeight - 10.0f * scale);
    glVertex2f(x + textWidth + 10.0f * scale, y + 10.0f * scale);
    glVertex2f(x - 10.0f * scale, y + 10.0f * scale);
    glEnd();
    
    // Draw FPS number using batched quads for better performance
    glColor3f(0.0f, 1.0f, 0.0f);  // Bright green
    
    // Batch all rectangles into a single glBegin/glEnd for better performance
    glBegin(GL_QUADS);
    
    for (int i = 0; i < textLen; i++) {
        // Handle decimal point
        if (fpsText[i] == '.') {
            float charX = x + i * charWidth * 0.6f;
            float charY = y - charHeight * 0.7f;
            float dotX1 = charX + charWidth * 0.2f;
            float dotX2 = charX + charWidth * 0.4f;
            float dotY1 = charY;
            float dotY2 = charY - strokeWidth;
            glVertex2f(dotX1, dotY1);
            glVertex2f(dotX2, dotY1);
            glVertex2f(dotX2, dotY2);
            glVertex2f(dotX1, dotY2);
            continue;
        }
        if (fpsText[i] < '0' || fpsText[i] > '9') continue;
        
        int digit = fpsText[i] - '0';
        float charX = x + i * charWidth * 0.6f;
        float charY = y;
        float w = charWidth * 0.5f;
        float h = charHeight;
        float segW = strokeWidth;
        float segH = h * 0.15f;
        float midY = charY - h * 0.5f;
        
        // 7-segment style using filled rectangles - batched for performance
        // Top segment (a)
        if (digit != 1 && digit != 4 && digit != 7) {
            glVertex2f(charX, charY);
            glVertex2f(charX + w, charY);
            glVertex2f(charX + w, charY - segH);
            glVertex2f(charX, charY - segH);
        }
        // Top-right segment (b)
        if (digit != 5 && digit != 6) {
            glVertex2f(charX + w - segW, charY);
            glVertex2f(charX + w, charY);
            glVertex2f(charX + w, midY);
            glVertex2f(charX + w - segW, midY);
        }
        // Bottom-right segment (c)
        if (digit != 2) {
            glVertex2f(charX + w - segW, midY);
            glVertex2f(charX + w, midY);
            glVertex2f(charX + w, charY - h);
            glVertex2f(charX + w - segW, charY - h);
        }
        // Bottom segment (d)
        if (digit != 1 && digit != 4 && digit != 7) {
            glVertex2f(charX, charY - h + segH);
            glVertex2f(charX + w, charY - h + segH);
            glVertex2f(charX + w, charY - h);
            glVertex2f(charX, charY - h);
        }
        // Bottom-left segment (e)
        if (digit != 1 && digit != 3 && digit != 4 && digit != 5 && digit != 7 && digit != 9) {
            glVertex2f(charX, midY);
            glVertex2f(charX + segW, midY);
            glVertex2f(charX + segW, charY - h);
            glVertex2f(charX, charY - h);
        }
        // Top-left segment (f)
        if (digit != 1 && digit != 2 && digit != 3 && digit != 7) {
            glVertex2f(charX, charY);
            glVertex2f(charX + segW, charY);
            glVertex2f(charX + segW, midY);
            glVertex2f(charX, midY);
        }
        // Middle segment (g)
        if (digit != 0 && digit != 1 && digit != 7) {
            glVertex2f(charX, midY - segH*0.5f);
            glVertex2f(charX + w, midY - segH*0.5f);
            glVertex2f(charX + w, midY + segH*0.5f);
            glVertex2f(charX, midY + segH*0.5f);
        }
    }
    
    glEnd();
    
    // Restore OpenGL state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}
