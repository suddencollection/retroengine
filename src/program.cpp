#include "program.hpp"

#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

Program::Program() :
  m_window{},
  m_windowSize{800, 600},
  m_image{},
  m_texture{},
  m_sprite{},
  m_cameraPos{m_worldWidth / 2, m_worldHeight / 2},
  m_cameraDir{0, 1},
  m_cameraPlane{1, 0}

{
  m_window.create(sf::VideoMode(m_windowSize.x, m_windowSize.y), "Raycaster");
  m_window.setVerticalSyncEnabled(true);

  resizePixelBuffer();
  m_texture.setSmooth(true);
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
  auto& screenSize = m_windowSize;
  auto& framebuffer = m_image;

  // Single Pixel Write
  for(unsigned x = 0; x < screenSize.x; ++x) {
    //
    // cameraX is the x-coordinate on the camera plane that the
    // current x-coordinate of the screen represents.
    //
    // right side of the screen will get coordinate 1,
    // the center of the screen gets coordinate 0,
    // and the left side of the screen gets coordinate -1
    //
    double cameraX = 0.0;
    cameraX = x / static_cast<double>(m_windowSize.x); // normalize the x position between 0 and 1
    cameraX *= 2;                                      // double it, now it's between 0 and 2
    cameraX -= 1;                                      // subtract 1, now it's between -1 and 1

    // We don't use the Euclidean distance to the point representing
    // player,but instead the distance to the camera plane (or, the
    // distance of the point projected on the camera direction to the
    // player), to avoid the fisheye effect.
    //
    // The fisheye effect is an effect you see if
    // you use the real distance, where all the walls
    // become rounded, and can make you sick if you rotate.
    //
    glm::vec2 rayDirection{
      m_cameraDir.x + m_cameraPlane.x * cameraX,
      m_cameraDir.y + m_cameraPlane.y * cameraX,
    };

    float distance = 0.f; // perpendicular wall distance
    glm::vec2 intersection;
    glm::ivec2 cellPosition;
    raycast(rayDirection, &intersection, &distance, &cellPosition);

    int wallHeight = screenSize.x / distance;
    int floorHeight = (screenSize.y - wallHeight) / 2;
    int ceillingHeight = floorHeight;

    // floor
    int y = 0;
    int maxY = floorHeight;
    while(y < maxY) {
      framebuffer.setPixel(x, y, sf::Color::Black);
      ++y;
    }

    // wall
    y = maxY;
    maxY += wallHeight;
    while(y < maxY) {
      framebuffer.setPixel(x, y, sf::Color::Green);
      ++y;
    }

    // ceilling
    y = maxY;
    maxY += ceillingHeight;
    while(y < maxY) {
      framebuffer.setPixel(x, y, sf::Color::White);
      ++y;
    }

    // for(unsigned y = 0; y < m_windowSize.y; ++y) {
    //   auto color = sf::Color{
    //     sf::Uint8(x + (y * 2) % 255),
    //     sf::Uint8(y - (x * 2) % 255),
    //     sf::Uint8((x + y) % 255),
    //   };
    //   m_image.setPixel(x, y, color);
    // }
  }
  m_texture.update(m_image);
}

bool Program::raycast(glm::vec2 const& rayDirection, glm::vec2* intersection, float* distance, glm::ivec2* cellPosition)
{
  //// The DDA Algorithm

  // aliases
  auto& rayStart = m_cameraPos;
  auto& rayDir = rayDirection;

  // which cell of the map we're in
  glm::ivec2 cellPos = rayStart; // truncated (ex: 1.33 becomes 1)
  assert(cellPos.x == std::trunc(rayStart.x));
  assert(cellPos.y == std::trunc(rayStart.y));

  // Given we have a point moving in a certain direction,
  // the bellow values represents the distance (lenght)
  // needded to be walked alongside this direction in order
  // to move exactly 1 unit of distance in the x or y axis
  //
  // Division by 0 is handed properly with floating point numbers
  //
  glm::vec2 rayLenghtUnitStep = {
    std::sqrt(1 + (rayDir.y / rayDirection.x) * (rayDirection.y / rayDirection.x)),
    std::sqrt((rayDir.x / rayDirection.y) * (rayDirection.x / rayDirection.y) + 1),
  };

  // total (accumulated) ray length from starting position
  // to current checking points, for the x or y rays
  glm::vec2 rayLenght;

  // one unit distance in the direction we will be walking
  glm::ivec2 step;

  if(rayDir.x < 0) {
    step.x = -1;                            // left
    rayLenght.x = (rayStart.x - cellPos.x); // get distance (ex: 12.55 - 12 = 0.55)
    rayLenght.x *= rayLenghtUnitStep.x;     // get ray length, as explained previously
  } else {
    step.x = 1;                                 // right
    rayLenght.x = (cellPos.x + 1 - rayStart.x); // get remaining (ex: 12 + 1 - 12.55 = 0.45)
    rayLenght.x *= rayLenghtUnitStep.x;         // get ray length, as explained previously
  }

  // same as before, but for y
  if(rayDir.y < 0) {
    step.y = -1;
    rayLenght.y = (rayStart.y - cellPos.y) * rayLenghtUnitStep.y;
  } else {
    step.y = 1;
    rayLenght.y = (cellPos.y + 1 - rayStart.y) * rayLenghtUnitStep.y;
  }

  bool cellFound = false;
  float maxDistance = 128.f; // so we don't loop forever
  float dist = 0.f;          // current distance
  while(!cellFound && dist < maxDistance) {
    // whichever distance is shorter, is the one walked in
    if(rayLenght.x < rayLenght.y) {
      cellPos.x -= step.x;
      dist = rayLenght.x; // for the next iteration check
      rayLenght.x += rayLenghtUnitStep.x;
    } else {
      cellPos.y -= step.y;
      dist = rayLenght.y; // for the next iteration check
      rayLenght.y += rayLenghtUnitStep.y;
    }

    // out of bounds
    // shouldn't be necessary, as long as the world is enclosed in walls
    if(cellPos.x < 0 || cellPos.x >= m_worldWidth ||
       cellPos.y < 0 || cellPos.y >= m_worldHeight) {
      spdlog::error("Out of bounds");
      spdlog::error("Cell Position: " + std::to_string(cellPos.x) + " " + std::to_string(cellPos.y));
      throw std::runtime_error("Out of bounds");
      break;
    }

    // whether some non empty cell was found
    if(m_worldMap[cellPos.x][cellPos.y] != 0) {
      cellFound = true;
    }
  }

  if(cellFound) {
    *intersection = rayStart + rayDir * dist; // final intersection position
    *distance = dist;                         // final distance
    *cellPosition = cellPos;
  }
  return cellFound;
}

void Program::handleInput(float timeStep)
{
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::J)) {
    // left key is pressed: move our character
    m_cameraPos.x += -1 * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::K)) {
    // left key is pressed: move our character
    m_cameraPos.y += -1 * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
    // left key is pressed: move our character
    m_cameraPos.y += 1 * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::SemiColon)) {
    // left key is pressed: move our character
    m_cameraPos.x += 1 * timeStep;
  }
}

void Program::run()
{
  spdlog::info("main loop started");

  // Timestep
  auto& now = std::chrono::steady_clock::now;
  using Timepoint = std::chrono::steady_clock::time_point;
  using Duration = std::chrono::duration<double>; // by default, represented in seconds
  //
  Timepoint frameStartTime = now();
  Timepoint frameEndTime;
  Duration frameDuration;

  while(m_window.isOpen()) {
    frameEndTime = now();
    frameDuration = frameEndTime - frameStartTime;
    frameStartTime = frameEndTime;

    // Events
    sf::Event event;
    while(m_window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
          m_window.close();
          break;
        case sf::Event::KeyPressed:
          handleInput(frameDuration.count());
          break;
        default:
          break;
      }
    }

    // Rendering
    writePixelBuffer();
    m_window.clear();
    m_window.draw(m_sprite);
    m_window.display();
  }
  spdlog::info("main loop ended");
}
