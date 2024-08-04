#include <SFML/Graphics.hpp>
#include <iostream>

void run()
{
  sf::RenderWindow window(sf::VideoMode(800, 600), "Raycaster");
  sf::Vector2u windowSize = window.getSize();

  // Pixel Buffer
  sf::Image image;
  image.create(windowSize.x, windowSize.y);

  // Texture
  sf::Texture texture;
  texture.create(windowSize.x, windowSize.y);

  // Sprite
  sf::Sprite sprite{texture};

  // Rendering
  while(window.isOpen()) {
    // Events
    sf::Event event;
    while(window.pollEvent(event)) {
      if(event.type == sf::Event::Closed)
        window.close();
    }

    // Single Pixel Write
    for(unsigned x = 0; x < windowSize.x; ++x) {
      for(unsigned y = 0; y < windowSize.y; ++y) {
        image.setPixel(x, y, sf::Color{255, 255, 255, 255});
      }
    }

    // Texture update
    texture.update(image);

    // Drawing
    window.clear(); // may not be neededd, since the entire screen is overwritten
    window.draw(sprite);
    window.display();
  }
}

int main()
{
  run();
  return 0;
}
