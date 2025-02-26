#include <engine/editor/map_info.h>



void nc::MapPoint::move(f32 x, f32 y)
{
  position[0].x += x;
  position[0].y += y;
}

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

void nc::MapPoint::update()
{

}

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

//============================================================

nc::MapPoint::~MapPoint()
{
  //glDeleteVertexArrays(1, &vertexArrayBuffer);
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
