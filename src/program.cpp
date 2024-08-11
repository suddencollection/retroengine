#include "program.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <SFML/Window/Keyboard.hpp>
#include <cstdlib>
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
  m_textureWall{},
  m_cameraSensitivity{3},
  m_cameraVelocity{2},
  m_cameraPos{6, 12},
  m_cameraDir{-1, 0},
  m_cameraPlane{0, 2 / 3.0}

{
  loadTextures();

  m_window.create(sf::VideoMode(m_windowSize.x, m_windowSize.y, sf::Style::Titlebar | sf::Style::Close), "");
  m_window.setVerticalSyncEnabled(true);
  resizePixelBuffer();
  m_texture.setSmooth(true);
  m_sprite.setTexture(m_texture);
}

void Program::loadTextures()
{
  // assets path
  char* env = std::getenv("ASSETS_PATH");
  if(!env) {
    throw std::runtime_error("Undefined ASSETS_PATH environment variable.");
  }
  std::string assetsPath{env};

  // loading
  if(!m_textureWall.loadFromFile(assetsPath + "textures/256_Tiles Circle 08.png")) {
    throw std::runtime_error("Failed to load wall texture");
  }
}

void Program::resizePixelBuffer()
{
  m_windowSize = m_window.getSize();
  m_image.create(m_windowSize.x, m_windowSize.y); // Pixel Buffer
  m_texture.create(m_windowSize.x, m_windowSize.y);

  // camera plane
  // float aspectRatio = m_windowSize.x / static_cast<float>(m_windowSize.y);
  // m_cameraPlane = glm::normalize(m_cameraPlane) * aspectRatio / 2.f;

  spdlog::info("Window resize [" + std::to_string(m_windowSize.x) + " " + std::to_string(m_windowSize.y) + "]");
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
    float cameraX = 0.0;
    cameraX = x / static_cast<float>(screenSize.x); // normalize the x position between 0 and 1
    cameraX *= 2;                                   // double it, now it's between 0 and 2
    cameraX -= 1;                                   // subtract 1, now it's between -1 and 1

    // We don't use the Euclidean distance to the point representing
    // player,but instead the distance to the camera plane (or, the
    // distance of the point projected on the camera direction to the
    // player), to avoid the fisheye effect.
    //
    // The fisheye effect is an effect you see if
    // you use the real distance, where all the walls
    // become rounded, and can make you sick if you rotate.
    //
    auto rayDirection = glm::normalize(glm::vec2{
      m_cameraDir.x + m_cameraPlane.x * cameraX,
      m_cameraDir.y + m_cameraPlane.y * cameraX,
    });
    glm::vec2& rayStart = m_cameraPos;
    float distance = 0.f; // perpendicular wall distance
    glm::vec2 intersection;
    glm::ivec2 cellPosition;
    Side hitSide = raycast(rayStart, rayDirection, &intersection, &distance, &cellPosition);
    assert(hitSide != Side::Invalid && "raycast function error");

    // calculate the perpendicular distance, to avoid fisheye effect
    //
    // when you're really close to a wall, the euclidean distance will be really small,
    // but the planeLength will still be increasing as cameraX increases, resulting in values
    // that, apparently, don't form a triangle. To fix this I simply picked the smallest of
    // the distances to substitute the perpendicular one.
    //
    // The result, as it seems to be, is that the vertical line of pixels filled entirely by
    // the wall gets repeated for the remaining planeLength positions. I still don't know
    // if this is gonna play well with textures, but its a working fix, for now.
    //
    float planeLength = std::min(glm::length(m_cameraPlane * cameraX), distance);
    float perpDistance = std::sqrt(distance * distance - planeLength * planeLength); // pythagoras

    unsigned wallHeight = std::min(screenSize.y / perpDistance, static_cast<float>(screenSize.y));
    unsigned floorHeight = (screenSize.y - wallHeight) / 2;
    unsigned ceillingHeight = screenSize.y - (floorHeight + wallHeight);
    assert((floorHeight + wallHeight + ceillingHeight) == unsigned(screenSize.y));

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

    float coordX = 0.f;
    glm::vec2 unitIntersection = (intersection - glm::trunc(intersection));
    coordX = hitSide == Side::Horizontal ? unitIntersection.x :
                                           unitIntersection.y;
    float coordY = 0.f;
    int pixelCount = 0;
    spdlog::info("coordX {}", coordX);
    assert(coordX >= 0 && coordX <= 1.f);
    while(y < maxY) {
      assert(y >= 0);
      assert(y < static_cast<int>(screenSize.y));

      coordY = static_cast<float>(pixelCount) / wallHeight;
      glm::vec2 pixelCoords{
        coordX * (m_textureWall.getSize().x - 1),
        coordY * (m_textureWall.getSize().y - 1),
      };

      framebuffer.setPixel(x, y, m_textureWall.getPixel(pixelCoords.x, pixelCoords.y));

      // int maxDistance = 16;
      // sf::Uint8 shade = 200 * (1 - std::min(1.f, perpDistance / maxDistance));
      // framebuffer.setPixel(x, y, sf::Color{shade, shade, shade});
      ++pixelCount;
      ++y;
    }

    // floor
    maxY += floorHeight;
    pixelCount = 0.f;
    while(y < maxY) {
      assert(y >= 0);
      assert(y < static_cast<int>(screenSize.y));

      float range = static_cast<float>(pixelCount) / floorHeight;
      sf::Uint8 shade = 130.f * range;
      framebuffer.setPixel(x, y, sf::Color(shade, shade, shade));
      ++pixelCount;
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

Program::Side Program::raycast(
  glm::vec2 const& rayStart,
  glm::vec2 const& unitRayDirection,
  glm::vec2* intersection,
  float* distance,
  glm::ivec2* cellPosition)
{
  // checks
  bool isUnitVector = glm::epsilonEqual(glm::length(unitRayDirection), 1.f, 0.00001f);
  if(!isUnitVector) {
    spdlog::error("non unit vector of length {}", glm::length(unitRayDirection));
    throw std::runtime_error("unitRayDirection is not an unit vector");
  }

  // aliases
  auto& rayDir = unitRayDirection;

  //// The DDA Algorithm

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
    std::sqrt(1 + (rayDir.y / rayDir.x) * (rayDir.y / rayDir.x)),
    std::sqrt((rayDir.x / rayDir.y) * (rayDir.x / rayDir.y) + 1),
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
  float maxDistance = 128.f;          // so we don't loop forever.
  float dist = 0.f;                   // current distance.
  Program::Side side = Side::Invalid; // side hit

  while(!cellFound && dist < maxDistance) {
    // whichever distance is shorter, is the one walked in
    if(rayLenght.x < rayLenght.y) {
      cellPos.x += step.x;
      dist = rayLenght.x; // for the next iteration check
      rayLenght.x += rayLenghtUnitStep.x;
      side = Side::Vertical;
    } else {
      cellPos.y += step.y;
      dist = rayLenght.y; // for the next iteration check
      rayLenght.y += rayLenghtUnitStep.y;
      side = Side::Horizontal;
    }

    // out of bounds
    if(cellPos.x < 0 || cellPos.x >= m_worldWidth ||
       cellPos.y < 0 || cellPos.y >= m_worldHeight) {
      spdlog::error("Cell Position: [" + std::to_string(cellPos.x) + " " + std::to_string(cellPos.y) + "]");
      spdlog::error("Max Position: [" + std::to_string(m_worldWidth) + " " + std::to_string(m_worldHeight) + "]");
      throw std::runtime_error("Out of bounds");
    }

    // whether some non empty cell was found
    if(m_worldMap[cellPos.x][cellPos.y] != 0) {
      cellFound = true;
    }
  }

  if(cellFound) {
    *intersection = rayStart + rayDir * dist; // final intersection position
    *distance = dist;                         // final distance
    *cellPosition = cellPos;                  // world coords
  }

  return cellFound ? side : Side::Invalid;
}

void Program::handleKeyboardInput(float timeStep)
{
  // left right movement
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::J)) {
    m_cameraPos += glm::rotate(m_cameraVelocity * m_cameraDir, glm::radians(90.f)) * timeStep;
  } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::SemiColon)) {
    m_cameraPos += glm::rotate(m_cameraVelocity * m_cameraDir, glm::radians(-90.f)) * timeStep;
  }

  // front back movement
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
    m_cameraPos += glm::rotate(m_cameraVelocity * m_cameraDir, glm::radians(0.f)) * timeStep;
  } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::K)) {
    m_cameraPos += glm::rotate(m_cameraVelocity * m_cameraDir, glm::radians(180.f)) * timeStep;
  }

  // camera rotation
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    rotateCamera(m_cameraSensitivity * timeStep);
  } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    rotateCamera(-m_cameraSensitivity * timeStep);
  }

  // camera plane
  if(sf::Keyboard::isKeyPressed(sf::Keyboard::Add)) {
    m_cameraPlane *= 1 + (0.6 * timeStep); // 1.6x
    spdlog::info("cameraPlane [" + std::to_string(m_cameraPlane.x) + " " + std::to_string(m_cameraPlane.y) + "]");
  } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract)) {
    m_cameraPlane *= 1 - (0.6 * timeStep); // 0.6x
    spdlog::info("cameraPlane [" + std::to_string(m_cameraPlane.x) + " " + std::to_string(m_cameraPlane.y) + "]");
  }
}

void Program::rotateCamera(float angle)
{
  // painless rotation
  m_cameraDir = glm::rotate(m_cameraDir, angle);
  m_cameraPlane = glm::rotate(m_cameraPlane, angle);

  // sanity checks
  bool isUnitVector = glm::epsilonEqual(glm::length(m_cameraDir), 1.f, 0.00001f);
  if(!isUnitVector) {
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
  Timepoint frameStartTime = now();
  Timepoint frameEndTime;
  Duration frameDuration;

  while(m_window.isOpen()) {
    frameEndTime = now();
    frameDuration = frameEndTime - frameStartTime;
    frameStartTime = frameEndTime;

    // Events and Input
    sf::Event event;
    while(m_window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
          m_window.close();
          break;
        case sf::Event::Resized:
          // unecessary for now
          // resizePixelBuffer();
          break;
        default:
          break;
      }
    }

    // input
    handleKeyboardInput(frameDuration.count());

    // Rendering
    writePixelBuffer();
    m_window.clear();
    m_window.draw(m_sprite);
    m_window.display();
  }
  spdlog::info("main loop ended");
}
