#include "program.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/mat2x2.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

Program::Program() :
  m_window{},
  m_windowSize{800, 600},
  m_image{},
  m_texture{},
  m_sprite{},
  m_cameraSensitivity{2},
  m_cameraPos{6, 12},
  m_cameraDir{0, 1},
  m_cameraPlane{1, 0}

{
  m_window.create(sf::VideoMode(m_windowSize.x, m_windowSize.y), "");
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
  glm::ivec2 screenSize{m_windowSize.x, m_windowSize.y};
  auto& framebuffer = m_image;

  for(int x = 0; x < screenSize.x; ++x) {
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

    glm::vec2 rayStart = m_cameraPos;

    float distance = 0.f; // perpendicular wall distance
    glm::vec2 intersection;
    glm::ivec2 cellPosition;
    bool result = raycast(rayDirection, rayStart, &intersection, &distance, &cellPosition);
    assert(result && "raycast function error");

    int wallHeight = std::min(screenSize.y / distance, static_cast<float>(screenSize.y));
    int floorHeight = (screenSize.y - wallHeight) / 2;
    int ceillingHeight = screenSize.y - (floorHeight + wallHeight);
    assert((floorHeight + wallHeight + ceillingHeight) == screenSize.y);

    // ceilling
    int y = 0;
    int maxY = ceillingHeight;
    while(y < maxY) {
      assert(y >= 0);
      assert(y < static_cast<int>(screenSize.y));
      framebuffer.setPixel(x, y, sf::Color::Cyan);
      ++y;
    }

    // wall
    maxY += wallHeight;
    while(y < maxY) {
      assert(y >= 0);
      assert(y < static_cast<int>(screenSize.y));

      int maxDistance = 16;
      sf::Uint8 shade = 200 * (1 - std::min(1.f, distance / maxDistance));
      framebuffer.setPixel(x, y, sf::Color{shade, shade, shade});
      ++y;
    }

    // floor
    maxY += floorHeight;
    while(y < maxY) {
      assert(y >= 0);
      assert(y < static_cast<int>(screenSize.y));

      framebuffer.setPixel(x, y, sf::Color(100.f, 100.f, 100.f));
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

bool Program::raycast(glm::vec2 const& rayDirection, glm::vec2 const& rayStart, glm::vec2* intersection, float* distance, glm::ivec2* cellPosition)
{
  //// The DDA Algorithm

  // aliases
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
    rayLenght.x = (cellPos.x + 1 - rayStart.x); // get remainder (ex: 12 + 1 - 12.55 = 0.45)
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
      cellPos.x += step.x;
      dist = rayLenght.x; // for the next iteration check
      rayLenght.x += rayLenghtUnitStep.x;
    } else {
      cellPos.y += step.y;
      dist = rayLenght.y; // for the next iteration check
      rayLenght.y += rayLenghtUnitStep.y;
    }

    // out of bounds
    if(cellPos.x < 0 || cellPos.x >= m_worldWidth ||
       cellPos.y < 0 || cellPos.y >= m_worldHeight) {
      spdlog::error("Cell Position: [" + std::to_string(cellPos.x) + " " + std::to_string(cellPos.y) + "]");
      spdlog::error("Max Position: [" + std::to_string(m_worldWidth) + " " + std::to_string(m_worldHeight) + "]");
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

void Program::handleKeyboardInput(float timeStep)
{
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::J)) {
    m_cameraPos += glm::rotate(m_cameraDir, glm::radians(90.f)) * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::K)) {
    m_cameraPos += glm::rotate(m_cameraDir, glm::radians(180.f)) * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
    m_cameraPos += glm::rotate(m_cameraDir, glm::radians(0.f)) * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::SemiColon)) {
    m_cameraPos += glm::rotate(m_cameraDir, glm::radians(-90.f)) * timeStep;
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    rotateCamera(m_cameraSensitivity * timeStep);
  }
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    rotateCamera(-m_cameraSensitivity * timeStep);
  }
}

void Program::rotateCamera(float angle)
{
  // [cos(a), -sin(a)]
  // [sin(a),  cos(a)]
  // auto& a = angle;
  // glm::mat2 rotationMatrix = {
  //   std::cos(a),
  //   std::sin(a),
  //   -std::sin(a),
  //   std::cos(a),
  // };

  // m_cameraDir = (rotationMatrix * m_cameraDir);
  // m_cameraPlane = (rotationMatrix * m_cameraPlane);

  // camera direction rotation
  // float rotSpeed = angle;

  // camera dir
  // double oldDirX = m_cameraDir.x;
  // m_cameraDir.x = m_cameraDir.x * cos(-rotSpeed) - m_cameraDir.y * sin(-rotSpeed);
  // m_cameraDir.y = oldDirX * sin(-rotSpeed) + m_cameraDir.y * cos(-rotSpeed);
  //
  // // camera plane
  // oldDirX = m_cameraPlane.x;
  // m_cameraPlane.x = m_cameraPlane.x * cos(-rotSpeed) - m_cameraPlane.y * sin(-rotSpeed);
  // m_cameraPlane.y = oldDirX * sin(-rotSpeed) + m_cameraPlane.y * cos(-rotSpeed);

  // painless rotation
  m_cameraDir = glm::rotate(m_cameraDir, angle);
  m_cameraPlane = glm::rotate(m_cameraPlane, angle);

  // some checks, for my sanity
  bool areUnitVectors =
    glm::epsilonEqual(glm::length(m_cameraDir), 1.f, 0.000001f) &&
    glm::epsilonEqual(glm::length(m_cameraPlane), 1.f, 0.000001f);

  if(!areUnitVectors) {
    spdlog::info("cameraDir [" + std::to_string(m_cameraDir.x) + " " + std::to_string(m_cameraDir.y) + "]");
    spdlog::info("width [" + std::to_string(glm::length(m_cameraDir)) + "]");
    spdlog::info("angle [" + std::to_string(angle) + "]");
    throw std::runtime_error("cameraDir is not an unit vector.");
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
          handleKeyboardInput(frameDuration.count());
          break;
        case sf::Event::Resized:
          resizePixelBuffer();
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
