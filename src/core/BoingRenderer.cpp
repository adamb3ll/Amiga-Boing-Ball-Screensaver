// BoingRenderer.cpp — Platform-independent OpenGL rendering implementation

#include "BoingRenderer.h"
#include "BoingPhysics.h"
#include <cmath>

BoingRenderer::BoingRenderer()
    : m_checkerTexture(0)
    , m_sphereSlices(32)
    , m_sphereStacks(32)
{
}

BoingRenderer::~BoingRenderer() {
    Cleanup();
}

void BoingRenderer::Initialize(int width, int height) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    SetupLighting();
    CreateCheckerTexture();
    
    float wallX, wallZ, floorY;
    SetViewport(width, height, wallX, wallZ, floorY);
}

void BoingRenderer::Cleanup() {
    if (m_checkerTexture) {
        glDeleteTextures(1, &m_checkerTexture);
        m_checkerTexture = 0;
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

void BoingRenderer::RenderFrame(const BoingPhysics& physics, const RenderConfig& config) {
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
            physics.GetBallRadius(), physics.GetSpinAngle());
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

void BoingRenderer::DrawBall(float ballX, float ballY, float ballZ, float ballRadius, float spinAngle) {
    glEnable(GL_LIGHTING);
    
    glPushMatrix();
    glTranslatef(ballX, ballY, ballZ);
    
    // Initial orientation
    glRotatef(90.0f, 1, 0, 0);   // rotate 90° around X
    glRotatef(-15.0f, 0, 1, 0);  // rotate 15° around Y
    glRotatef(spinAngle, 0, 0, 1);   // dynamic spin around Z
    
    DrawSphere(ballRadius);
    glPopMatrix();
}

void BoingRenderer::DrawSphere(float radius) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, m_checkerTexture);
    gluSphere(quad, radius, m_sphereSlices, m_sphereStacks);
    gluDeleteQuadric(quad);
}
