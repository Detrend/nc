#pragma once

#include <glad/glad.h>

#include <types.h>
#include <vector>

#include <SDL.h>
#include <SDL_opengl.h>

namespace nc
{
  struct vertex_3d {
    f32 x;
    f32 y;
    f32 z;

    vertex_3d() {
      x = 0;
      y = 0;
      z = 0;
    }

    vertex_3d(f32 x, f32 y, f32 z) {
      this->x = x;
      this->y = y;
      this->z = z;
    }
  };

  struct vertex_2d {
    f32 x;
    f32 y;

    vertex_2d() {
      x = 0;
      y = 0;
    }

    vertex_2d(f32 x, f32 y) {
      this->x = x;
      this->y = y;
    }

    vertex_2d operator+(vertex_2d other)
    {
      return vertex_2d(x + other.x, y + other.y);
    }

    vertex_2d operator-(vertex_2d other)
    {
      return vertex_2d(x - other.x, y - other.y);
    }

    vertex_2d operator*(float const& other)
    {
      return vertex_2d(x * other, y * other);
    }

    vertex_2d operator/(float const& other)
    {
      return vertex_2d(x / other, y / other);
    }

    vertex_2d& operator+=(vertex_2d const& other)
    {
      x += other.x;
      y += other.y;
      return *this;
    }

    vertex_2d& operator-=(vertex_2d const& other)
    {
      x -= other.x;
      y -= other.y;
      return *this;
    }
  };

  class MapPoint
  {
  public:
    MapPoint(vertex_2d position)
    {
      this->position[0] = vertex_3d(position.x, position.y, 1);
      this->position[1] = vertex_3d(1, 1, 0);
      init_gl();
    }

    void update();
    void draw();
    ~MapPoint();

  private:
    void init_gl();

    vertex_3d position[2];

    //OpenGL Stuff
    GLuint vertexBuffer;
    GLuint vertexArrayBuffer;
  };

  class WallDef;

  class Sector
  {
    double floorHeight;
    double ceilingHeight;

    // topTexture
    // bottomTexture

    std::vector<WallDef> walls;
  };

  class SideDef
  {
    // UV coordinates for the wall drawing are the size of the plane
    // texture shall be tiled (repeated) given a offset

    //topTexture
    //midTexture
    //bottomTexture

    double offsetX;
    double offsetY;

    Sector& sector; // sector we are pointing to, can be NULL
  };

  // Definition of a wall, a line in a editor
  class WallDef
  {
    vertex_2d start;
    vertex_2d end;
    double height;

    SideDef& frontSideDef;
    SideDef& backSideDef;

  };

  class Map
  {
    std::vector<Sector> sectors;
    std::vector<WallDef> walls;

    //entities
  };
}
