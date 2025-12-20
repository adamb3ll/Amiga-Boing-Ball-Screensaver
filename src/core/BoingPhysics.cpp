// BoingPhysics.cpp — Platform-independent physics implementation

#include "BoingPhysics.h"
#include <cmath>

BoingPhysics::BoingPhysics()
    : m_ballRadius(0.25f)
    , m_restitution(1.0f)
    , m_gravity(-9.8f)
    , m_timeScale(0.5f)
    , m_wallX(1.0f)
    , m_wallZ(1.0f)
    , m_floorY(-1.0f)
    , m_ballX(0.0f)
    , m_ballY(0.0f)
    , m_ballZ(0.0f)
    , m_vx(0.8f)
    , m_vy(4.5f)
    , m_vz(0.0f)
    , m_spinAngle(0.0f)
    , m_spinSpeed(120.0f)
    , m_spinDir(1)
    , m_floorCollisionThisFrame(false)
    , m_wallCollisionThisFrame(false)
{
}

void BoingPhysics::Initialize(float wallX, float wallZ, float floorY) {
    m_wallX = wallX;
    m_wallZ = wallZ;
    m_floorY = floorY;
    
    // Set initial ball position based on bounds
    m_ballX = -m_wallX + m_ballRadius;
    m_ballY = m_floorY + m_ballRadius;
    m_ballZ = 0.0f;
}

void BoingPhysics::Update(float deltaTime) {
    // Clear collision flags
    m_floorCollisionThisFrame = false;
    m_wallCollisionThisFrame = false;
    
    // Apply time scale
    float dt = deltaTime * m_timeScale;
    
    // Update components in order
    UpdateSpin(dt);
    UpdateVelocity(dt);
    UpdatePosition(dt);
    CheckFloorCollision();
    CheckWallCollisions();
}

void BoingPhysics::UpdateSpin(float dt) {
    m_spinAngle += m_spinDir * m_spinSpeed * dt;
    
    // Keep angle in [0, 360] range
    if (m_spinAngle > 360.0f) m_spinAngle -= 360.0f;
    if (m_spinAngle < 0.0f) m_spinAngle += 360.0f;
}

void BoingPhysics::UpdateVelocity(float dt) {
    m_vy += m_gravity * dt;
}

void BoingPhysics::UpdatePosition(float dt) {
    m_ballX += m_vx * dt;
    m_ballY += m_vy * dt;
    m_ballZ += m_vz * dt;
}

void BoingPhysics::CheckFloorCollision() {
    if (m_ballY < m_floorY + m_ballRadius) {
        m_ballY = m_floorY + m_ballRadius;
        m_vy = 4.5f;  // Reset to bounce velocity
        m_floorCollisionThisFrame = true;
    }
}

void BoingPhysics::CheckWallCollisions() {
    // Check X walls (left/right)
    if (m_ballX > m_wallX - m_ballRadius) {
        m_ballX = m_wallX - m_ballRadius;
        m_vx = -fabsf(m_vx);
        m_spinDir *= -1;
        m_wallCollisionThisFrame = true;
    }
    else if (m_ballX < -m_wallX + m_ballRadius) {
        m_ballX = -m_wallX + m_ballRadius;
        m_vx = +fabsf(m_vx);
        m_spinDir *= -1;
        m_wallCollisionThisFrame = true;
    }
    
    // Check Z walls (front/back)
    if (m_ballZ > m_wallZ - m_ballRadius) {
        m_ballZ = m_wallZ - m_ballRadius;
        m_vz = -fabsf(m_vz);
    }
    else if (m_ballZ < -m_wallZ + m_ballRadius) {
        m_ballZ = -m_wallZ + m_ballRadius;
        m_vz = +fabsf(m_vz);
    }
}

void BoingPhysics::Reset() {
    m_ballX = -m_wallX + m_ballRadius;
    m_ballY = m_floorY + m_ballRadius;
    m_ballZ = 0.0f;
    m_vx = 0.8f;
    m_vy = 4.5f;
    m_vz = 0.0f;
    m_spinAngle = 0.0f;
    m_spinDir = 1;
    m_floorCollisionThisFrame = false;
    m_wallCollisionThisFrame = false;
}
