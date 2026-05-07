#pragma once

class GameObject
{
public:
    GameObject() = default;
    GameObject(float x, float y, float width, float height);

    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    void SetVelocity(float velocityX, float velocityY);
    void Update(float deltaTime);
    void RenderWhiteRectangle() const;

    float X() const { return m_x; }
    float Y() const { return m_y; }
    float Width() const { return m_width; }
    float Height() const { return m_height; }
    float HalfWidth() const { return m_width * 0.5f; }
    float HalfHeight() const { return m_height * 0.5f; }
    float VelocityX() const { return m_velocityX; }
    float VelocityY() const { return m_velocityY; }

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 0.0f;
    float m_height = 0.0f;
    float m_velocityX = 0.0f;
    float m_velocityY = 0.0f;
};
