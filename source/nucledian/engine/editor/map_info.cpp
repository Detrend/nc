#include <engine/editor/map_info.h>

//================================================================================

void nc::MapPoint::move(f32 x, f32 y)
{
  position[0].x += x;
  position[0].y += y;

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2, &position, GL_DYNAMIC_DRAW);

  //bind to VAO
  glBindVertexArray(vertexArrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2, &position, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//================================================================================

void nc::MapPoint::move_to(f32 x, f32 y)
{
  position[0].x = x;
  position[0].y = y;

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2, &position, GL_DYNAMIC_DRAW);

  //bind to VAO
  glBindVertexArray(vertexArrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2, &position, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//================================================================================

void nc::MapPoint::update()
{

}

//================================================================================

void nc::MapPoint::draw()
{
  glBindVertexArray(vertexArrayBuffer);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glDrawArrays(GL_POINTS, 0, 1);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindVertexArray(0);
}

void nc::MapPoint::cleanup()
{
  glDeleteBuffers(1, &vertexBuffer);
  glDeleteVertexArrays(1, &vertexArrayBuffer);
}

nc::vertex_3d nc::MapPoint::get_pos()
{
  return position[0];
}

//============================================================

nc::MapPoint::~MapPoint()
{
  
}

//============================================================

void nc::MapPoint::init_gl()
{
  // bind to VBO
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, &position, GL_DYNAMIC_DRAW);

  //bind to VAO
  glGenVertexArrays(1, &vertexArrayBuffer);

  glBindVertexArray(vertexArrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), &position, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

// ===================================================================

void nc::Map::draw()
{
  for (size_t i = 0; i < mapPoints.size(); i++)
  {
    mapPoints[i].get()->draw();
  }
}

void nc::Map::add_point(vertex_2d position)
{
  mapPoints.push_back(std::make_shared<MapPoint>(MapPoint(position)));
}

void nc::Map::remove_point(size_t index)
{
  mapPoints[index].get()->cleanup();
  mapPoints.erase(mapPoints.begin() + index);
}

void nc::Map::move_vertex(vertex_2d positionTo, size_t index)
{
  mapPoints[index].get()->move_to(positionTo.x, positionTo.y);
}

size_t nc::Map::get_closest_point_index(vertex_2d position)
{
  f32 minDist = 666;
  int candidate = -1;

  for (size_t i = 0; i < mapPoints.size(); i++)
  {
    f32 dist = mapPoints[i].get()->get_distance(position);
    if (dist > 0.125)
    {
      continue;
    }

    if (dist < minDist)
    {
      minDist = dist;
      candidate = i;
    }
  }

  return candidate;
}

void nc::WallDef::cleanup()
{
  glDeleteBuffers(1, &vertexBuffer);
  glDeleteVertexArrays(1, &vertexArrayBuffer);
}

nc::WallDef::~WallDef()
{

}

void nc::WallDef::init_gl()
{
  // bind to VBO
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, &position, GL_DYNAMIC_DRAW);

  //bind to VAO
  glGenVertexArrays(1, &vertexArrayBuffer);

  glBindVertexArray(vertexArrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), &position, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
