// BoingPhysics.h — Platform-independent physics engine for the Boing Ball
// Handles position, velocity, gravity, collisions, and spin

#pragma once

class BoingPhysics {
public:
    BoingPhysics();
    
    // Initialize physics with world bounds
    void Initialize(float wallX, float wallZ, float floorY);
    
    // Update physics simulation
    void Update(float deltaTime);
    
    // Getters for rendering
    float GetBallX() const { return m_ballX; }
    float GetBallY() const { return m_ballY; }
    float GetBallZ() const { return m_ballZ; }
    float GetSpinAngle() const { return m_spinAngle; }
    float GetBallRadius() const { return m_ballRadius; }
    
    // World bounds getters
    float GetWallX() const { return m_wallX; }
    float GetWallZ() const { return m_wallZ; }
    float GetFloorY() const { return m_floorY; }
    
    // Setters for customization
    void SetTimeScale(float scale) { m_timeScale = scale; }
    void SetGravity(float gravity) { m_gravity = gravity; }
    void SetRestitution(float restitution) { m_restitution = restitution; }
    void SetSpinSpeed(float speed) { m_spinSpeed = speed; }
    
    // Event callbacks (return true if collision occurred)
    bool DidFloorCollision() const { return m_floorCollisionThisFrame; }
    bool DidWallCollision() const { return m_wallCollisionThisFrame; }
    
    // Reset to initial state
    void Reset();

private:
    // Physics constants
    float m_ballRadius;
    float m_restitution;
    float m_gravity;
    float m_timeScale;
    
    // World boundaries
    float m_wallX;
    float m_wallZ;
    float m_floorY;
    
    // Ball state
    float m_ballX;
    float m_ballY;
    float m_ballZ;
    float m_vx;  // velocity X
    float m_vy;  // velocity Y
    float m_vz;  // velocity Z
    
    // Spin state
    float m_spinAngle;
    float m_spinSpeed;
    int m_spinDir;  // +1 or -1
    
    // Collision detection flags
    bool m_floorCollisionThisFrame;
    bool m_wallCollisionThisFrame;
    
    // Helper methods
    void UpdateSpin(float dt);
    void UpdateVelocity(float dt);
    void UpdatePosition(float dt);
    void CheckFloorCollision();
    void CheckWallCollisions();
};
