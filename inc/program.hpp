#pragma once

#include <SFML/Graphics.hpp>

class Program
{
public:
  Program(Program&&) = delete;
  Program(const Program&) = delete;
  Program& operator=(Program&&) = delete;
  Program& operator=(const Program&) = delete;

  Program();
  ~Program() = default;

  void run();
  void resizePixelBuffer();
  void writePixelBuffer();

private:
  // SFML
  sf::RenderWindow m_window;
  sf::Vector2u m_windowSize;
  sf::Image m_image;
  sf::Texture m_texture;
  sf::Sprite m_sprite;
};
