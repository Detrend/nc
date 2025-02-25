#include <engine/editor/map_info.h>

void nc::MapPoint::draw()
{
  glBindVertexArray(vertexArrayBuffer);
  glDrawArrays(GL_POINTS, 0, 1);
}

//============================================================

nc::MapPoint::~MapPoint()
{
  glDeleteVertexArrays(1, &vertexArrayBuffer);
}

//============================================================

void nc::MapPoint::init_gl()
{
  // bind to VBO
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, &position, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  //bind to VAO
  glGenVertexArrays(1, &vertexArrayBuffer);
  glBindVertexArray(vertexArrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), &position, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
