#pragma once

#include <glad/glad.h>

#include <types.h>
#include <vector>

#include <SDL.h>
#include <SDL_opengl.h>

#include <memory>

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

    f32 dot(vertex_3d other)
    {
      return x * other.x + y * other.y + z * other.z;
    }

    vertex_3d operator+(vertex_3d other)
    {
      return vertex_3d(x + other.x, y + other.y, z + other.z);
    }

    vertex_3d operator-(vertex_3d other)
    {
      return vertex_3d(x - other.x, y - other.y, z - other.z);
    }

    vertex_3d operator*(float const& other)
    {
      return vertex_3d(x * other, y * other, z * other);
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

    f32 dot(vertex_2d other)
    {
      return x * other.x + y * other.y;
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

    f32 distance(vertex_2d other)
    {
      f32 difX = x - other.x;
      f32 difY = y - other.y;
      return sqrt(difX * difX + difY * difY);
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

    f32 get_distance(vertex_2d point)
    {
      f32 difx = abs(point.x - position[0].x);
      f32 dify = abs(point.y - position[0].y);
      return difx > dify ? difx : dify;
    }

    void move(f32 x, f32 y);
    void move_to(f32 x, f32 y);
    void update();
    void draw();
    void cleanup();
    vertex_3d get_pos();
    ~MapPoint();

  private:
    void init_gl();

    vertex_3d position[2];

    //OpenGL Stuff
    GLuint vertexBuffer;
    GLuint vertexArrayBuffer;
  };

  class WallDef;

  //class Sector
  //{
  //  double floorHeight;
  //  double ceilingHeight;

  //  // topTexture
  //  // bottomTexture

  //  std::vector<WallDef> walls;
  //};

  //class SideDef
  //{
  //  // UV coordinates for the wall drawing are the size of the plane
  //  // texture shall be tiled (repeated) given a offset

  //  //topTexture
  //  //midTexture
  //  //bottomTexture

  //  double offsetX;
  //  double offsetY;

  //  Sector& sector; // sector we are pointing to, can be NULL
  //};

  // Definition of a wall, a line in the editor
  class WallDef
  {
  public:
    WallDef(MapPoint* start, MapPoint* end)
    {
      this->start = start;
      this->end = end;

      position[0] = start->get_pos();
      position[1] = vertex_3d(1, 1, 0);
      position[2] = end->get_pos();
      position[3] = vertex_3d(1, 1, 0);

      init_gl();
    }

    f32 get_distance(vertex_2d point)
    {
      f32 difX = start->get_pos().x - start->get_pos().x;
      f32 difY = start->get_pos().y - start->get_pos().y;

      const f32 l2 = difX * difX + difY * difY;

      if (l2 == 0.0f)
      {
        return start->get_distance(point); // yes, this is manhattan
      }

      vertex_2d v = vertex_2d(start->get_pos().x, start->get_pos().y);
      vertex_2d w = vertex_2d(end->get_pos().x, end->get_pos().y);

      vertex_2d p_v = point - v;
      vertex_2d w_v = w - v;

      f32 t = std::max(0.0f, std::min(1.0f, (p_v.dot(w_v) / l2)));

      vertex_2d projection = v + w_v * t;

      return point.distance(projection);
      
    }

    void move(f32 x, f32 y);
    void update();
    void draw();
    void cleanup();
    ~WallDef();  

    MapPoint* start;
    MapPoint* end;

  private:
    

    void init_gl();

    vertex_3d position[4];

    //OpenGL Stuff
    GLuint vertexBuffer;
    GLuint vertexArrayBuffer;
    //SideDef& frontSideDef;
    //SideDef& backSideDef;

  };

  class Map
  {
    std::vector<std::shared_ptr<MapPoint>> mapPoints;
   //std::vector<Sector> sectors;
    std::vector<std::shared_ptr<WallDef>> walls;

    //entities

  public:
    void draw();
    void add_point(vertex_2d position);
    void remove_point(size_t index);
    void remove_wall(size_t index);
    void move_vertex(vertex_2d positionTo, size_t index);
    size_t get_closest_point_index(vertex_2d posiiton);
    size_t get_closest_wall_index(vertex_2d position);
  };
}
