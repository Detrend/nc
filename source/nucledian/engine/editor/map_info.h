#pragma once

#include <types.h>

#include <vector>

namespace nc
{
  struct vertex_3d {
    f64 x;
    f64 y;
    f64 z;

    vertex_3d() {
      x = 0;
      y = 0;
      z = 0;
    }

    vertex_3d(f64 x, f64 y, f64 z) {
      this->x = x;
      this->y = y;
      this->z = z;
    }
  };

  struct vertex_2d {
    f64 x;
    f64 y;

    vertex_2d() {
      x = 0;
      y = 0;
    }

    vertex_2d(f64 x, f64 y) {
      this->x = x;
      this->y = y;
    }
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