#include "GameObject.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>

GameObject::GameObject(float x, float y, float width, float height)
    : m_x(x), m_y(y), m_width(width), m_height(height)
{
}

void GameObject::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void GameObject::SetSize(float width, float height)
{
    m_width = width;
    m_height = height;
}

void GameObject::SetVelocity(float velocityX, float velocityY)
{
    m_velocityX = velocityX;
    m_velocityY = velocityY;
}

void GameObject::Update(float deltaTime)
{
    m_x += m_velocityX * deltaTime;
    m_y += m_velocityY * deltaTime;
}

void GameObject::RenderWhiteRectangle() const
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(m_x - HalfWidth(), m_y - HalfHeight());
    glVertex2f(m_x + HalfWidth(), m_y - HalfHeight());
    glVertex2f(m_x + HalfWidth(), m_y + HalfHeight());
    glVertex2f(m_x - HalfWidth(), m_y + HalfHeight());
    glEnd();
}
