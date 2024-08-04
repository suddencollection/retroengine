#include "program.hpp"

Program::Program() :
  m_window{},
  m_windowSize{800, 600},
  m_image{},
  m_texture{},
  m_sprite{}
{
  m_window.create(sf::VideoMode(m_windowSize.x, m_windowSize.y), "Raycaster");
  m_window.setVerticalSyncEnabled(true);

  resizePixelBuffer();
  m_sprite.setTexture(m_texture);
}

void Program::resizePixelBuffer()
{
  m_windowSize = m_window.getSize();
  m_image.create(m_windowSize.x, m_windowSize.y); // Pixel Buffer
  m_texture.create(m_windowSize.x, m_windowSize.y);
}

void Program::writePixelBuffer()
{
  // Single Pixel Write
  for(unsigned x = 0; x < m_windowSize.x; ++x) {
    for(unsigned y = 0; y < m_windowSize.y; ++y) {
      auto color = sf::Color{
        sf::Uint8(x + (y * 2) % 255),
        sf::Uint8(y - (x * 2) % 255),
        sf::Uint8((x + y) % 255),
      };
      m_image.setPixel(x, y, color);
    }
  }

  m_texture.update(m_image);
}

void Program::run()
{
  while(m_window.isOpen()) {
    // Events
    sf::Event event;
    while(m_window.pollEvent(event)) {
      if(event.type == sf::Event::Closed)
        m_window.close();
    }

    // Rendering
    writePixelBuffer();
    m_window.clear();
    m_window.draw(m_sprite);
    m_window.display();
  }
}
