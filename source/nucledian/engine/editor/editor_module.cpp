#include <engine/editor/editor_module.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <vector>

// #include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui/imgui_internal.h>

namespace nc
{
  //EditorSystem::EditorSystem()
  //{
  //  this->io = ImGui::GetIO();
  //}
  //=========================================================
  EngineModuleId EditorSystem::get_module_id()
  {
    return EngineModule::editor_system;
  }
  //=========================================================

  void EditorSystem::on_event(ModuleEvent& event)
  {
    switch (event.type)
    {
    case ModuleEventType::post_init:
    {
      break;
    }
    case ModuleEventType::editor_update:
    {
      get_mouse_input();    
      update_map(gridOffset);
      switch (editMode2D)
      {
      case nc::move:
        break;
      case nc::vertex:
        update_cursor_gl();  
        check_for_delete();     
        break;
      case nc::line:
        update_cursor_gl();
        break;
      case nc::sector:
        break;
      default:
        break;
      }
      break;
    }
    case ModuleEventType::editor_render:
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      grid.render_grid(windowSize, gridOffset, zoom);

      draw_map();

      if (editMode2D == vertex || editMode2D == line)
      {
        draw_cursor();
      }

      draw_ui(windowSize);

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(window);

      break;
    }
    case ModuleEventType::terminate:
    {
      terminate_imgui();
      break;
    }
    default:
      break;
    }
  }

  //================================================================================

  bool EditorSystem::init(SDL_Window* window, void* gl_context)
  {
    this->window = window;

    zoom = 200.0f;
    gridOffset = vertex_2d();

    gladLoadGLLoader(SDL_GL_GetProcAddress);

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

    int width;
    int height;

    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);

    windowSize = vertex_2d(width, height);

    grid.init();

    init_imgui(window, gl_context);

    init_cursor_gl();
    return true;
  }

  //================================================================================

  EditorSystem::~EditorSystem()
  {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteVertexArrays(1, &vertexArrayBuffer);
  }

  //=================================================================
  // MOUSE HANDLES
  //=================================================================

  void EditorSystem::get_mouse_input()
  {
    int x;
    int y;
    Uint32 mouseState = SDL_GetMouseState(&x, &y);
    io.AddMousePosEvent(x, y);

    for (int i = 0; i < 5; i++) {
      prevMouse[i] = curMouse[i];
      curMouse[i] = false;
    }

    prevMousePos = curMousePos;
    curMousePos = get_mouse_pos(x, y);

    get_left_mouse_button(mouseState);
    get_right_mouse_button(mouseState);
    get_closest_point();

    curGridMousePos = curMousePos + gridOffset;
    
    
  }

  //=================================================================

  void EditorSystem::get_left_mouse_button(Uint32 mouseState)
  {
    if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
      curMouse[0] = true;

      

      if (editMode2D == move)
      {
        gridOffset += (prevMousePos - curMousePos);
      }

      if (editMode2D == vertex && !prevMouse[0])
      {
        select_point(curMousePos);
      }

      if (editMode2D == vertex) 
      {
        move_point(curGridMousePos);
      }

      if (!prevMouse[0])
      {
        io.AddMouseButtonEvent(0, true);
      }
    }

    if (prevMouse[0] && !curMouse[0])
    {
      if (editMode2D == vertex) {

        if (selected >= 0) {
          vertex_2d snapPos = get_snap_to_grid_pos(curGridMousePos.x, curGridMousePos.y);
          map.move_vertex(snapPos, selected);
          selected = -1;
        }
      }

      io.AddMouseButtonEvent(0, false);
    }
  }

  //================================================================================

  void EditorSystem::get_right_mouse_button(Uint32 mouseState)
  {
    if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
    {
      curMouse[1] = true;

      if (editMode2D == vertex)
      {
        if (!prevMouse[1])
        {
          vertex_2d snapPos = get_snap_to_grid_pos(curGridMousePos.x, curGridMousePos.y);
          //mapPoints.push_back(std::make_shared<MapPoint>(MapPoint(snapPos)));
          map.add_point(snapPos);
        }
      }

      if (!prevMouse[1])
      {
        io.AddMouseButtonEvent(1, true);
      }

    }

    if (prevMouse[1] && !curMouse[1])
    {
      io.AddMouseButtonEvent(1, false);
    }
  }

  //================================================================================

  void EditorSystem::get_closest_point()
  {
    /*f32 minDist = 666;
    int candidate = -1;

    for (size_t i = 0; i < mapPoints.size(); i++)
    {
      f32 dist = mapPoints[i].get()->get_distance(curGridMousePos);
      if (dist > 0.125)
      {
        continue;
      }

      if (dist < minDist)
      {
        minDist = dist;
        candidate = i;
      }
    }*/

    closest = map.get_closest_point_index(curGridMousePos);
  }

  //================================================================================

  void EditorSystem::check_for_delete()
  {
    const u8* keyboard_state = SDL_GetKeyboardState(nullptr);

    if (keyboard_state[SDL_SCANCODE_DELETE]) {
      if (closest > -1) {
        map.remove_point(closest);
      }
    }

    /*SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_DELETE) {
          if (closest > -1) {
            mapPoints[closest].get()->cleanup();
            mapPoints.erase(mapPoints.begin() + closest);
          }
        }
      }
    }*/
  }

  //================================================================================

  void EditorSystem::select_point(vertex_2d mousePos)
  {
    /*f32 minDist = 666;
    int candidate = -1;

    for (size_t i = 0; i < mapPoints.size(); i++)
    {
      f32 dist = mapPoints[i].get()->get_distance(mousePos);
      if (dist > 0.1)
      {
        continue;
      }

      if (dist < minDist)
      {
        minDist = dist;
        candidate = i;
      }
    }*/

    selected = map.get_closest_point_index(mousePos);
  }

  void EditorSystem::move_point(vertex_2d mousePos)
  {
    if (selected < 0)
    {
      return;
    }

    //mapPoints[selected].get()->move_to(mousePos.x, mousePos.y);
    map.move_vertex(mousePos, selected);
  }

  //=================================================================

  vertex_2d EditorSystem::get_mouse_pos(int x, int y)
  {
    x = x - windowSize.x / 2; // shift
    y = -y + windowSize.y / 2;
    return vertex_2d(x / zoom, y / zoom) * 2; // zoom
  }

  //=================================================================

  vertex_2d EditorSystem::get_snap_to_grid_pos(float x, float y)
  {
    float newX = floor(x / GRID_SIZE) * GRID_SIZE;
    if (x - newX > GRID_SIZE / 2) {
      newX += GRID_SIZE;
    }
    float newY = floor(y / GRID_SIZE) * GRID_SIZE;
    if (y - newY > GRID_SIZE / 2) {
      newY += GRID_SIZE;
    }
    return vertex_2d(newX, newY);
  }

  //=================================================================

  vertex_2d EditorSystem::get_snap_to_grid_pos()
  {
    float x = curGridMousePos.x;
    float y = curGridMousePos.y;

    float newX = floor(x / GRID_SIZE) * GRID_SIZE;
    if (x - newX > GRID_SIZE / 2) {
      newX += GRID_SIZE;
    }
    float newY = floor(y / GRID_SIZE) * GRID_SIZE;
    if (y - newY > GRID_SIZE / 2) {
      newY += GRID_SIZE;
    }
    return vertex_2d(newX, newY);
  }

  //=================================================================
  // IMGUI HANDLES
  //=================================================================

  void EditorSystem::init_imgui(SDL_Window* window, void* gl_context)
  {
    editMode2D = move;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();

    io.WantCaptureMouse = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");
  }

  //=================================================================

  void EditorSystem::draw_ui(vertex_2d windowSize)
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    create_menu_bar();
    create_bottom_bar(windowSize);
  }

  //=================================================================

  void EditorSystem::create_bottom_bar(vertex_2d& windowSize)
  {
    vertex_2d snapPos = get_snap_to_grid_pos(curGridMousePos.x, curGridMousePos.y);

    ImGui::SetNextWindowPos(ImVec2(0, windowSize.y - 50.0f));
    ImGui::SetNextWindowSize(ImVec2(windowSize.x, 50.0f));
    ImGui::Begin("Info");

    ImGui::Text("MousePos: %.2f:%.2f", curMousePos.x, curMousePos.y);
    ImGui::SameLine();
    ImGui::Text("GridPos: %.2f:%.2f", curGridMousePos.x, curGridMousePos.y);
    ImGui::SameLine();
    ImGui::Text("SnapToPos: %.2f: %.2f", snapPos.x, snapPos.y);
    ImGui::SameLine();
    ImGui::Text("Closest: %d", closest);
    ImGui::End();
  }

  //=================================================================

  void EditorSystem::create_menu_bar()
  {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(400, 80));
    ImGui::Begin("MENU");

    if (ImGui::Button("ZOOM IN"))
    {
      if (zoom < 400)
      {
        zoom *= 2;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("ZOOM OUT"))
    {
      if (zoom > 50)
      {
        zoom /= 2;
      }
    }
    ImGui::SameLine();
    ImGui::Text("Zoom: %f", zoom);


    ImGui::Text("EDITOR MODE:");
    ImGui::SameLine();

    ImGui::BeginDisabled(editMode2D == move);
    if (ImGui::Button("MOVE"))
    {
      editMode2D = move;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(editMode2D == vertex);
    if (ImGui::Button("VERTEX"))
    {
      editMode2D = vertex;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(editMode2D == line);
    if (ImGui::Button("LINE"))
    {
      editMode2D = line;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(editMode2D == sector);
    if (ImGui::Button("SECTOR"))
    {
      editMode2D = sector;
    }
    ImGui::EndDisabled();

    ImGui::End();
  }

  //=================================================================

  void EditorSystem::terminate_imgui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  }

  //=================================================================

  void EditorSystem::update_map(vertex_2d offset)
  {
    glUseProgram(shaderProgram);

    viewMatrix = glm::ortho(-windowSize.x / (zoom)+offset.x, windowSize.x / (zoom)+offset.x,
      -windowSize.y / (zoom)+offset.y, windowSize.y / (zoom)+offset.y);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glPointSize(10);
    /*for (size_t i = 0; i < mapPoints.size(); ++i)
    {
      mapPoints[i].get()->draw();
    }*/
    map.draw();

    glUseProgram(0);
  }

  //================================================================================

  void EditorSystem::draw_map()
  {

    glUseProgram(shaderProgram);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glPointSize(10);
    /*for (size_t i = 0; i < mapPoints.size(); ++i)
    {
      mapPoints[i].get()->draw();
    }*/
    map.draw();

    glUseProgram(0);
  }

  //=================================================================
  // OPENGL CURSOR
  //=================================================================

  void EditorSystem::init_cursor_gl()
  {
    vertex_2d snappedPos = get_snap_to_grid_pos();

    onGridPoint[0] = vertex_3d(snappedPos.x, snappedPos.y, -1);
    onGridPoint[1] = vertex_3d(1.0f, 0.5f, 0);

    // vertex shader
    const char* vertexShaderSource = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "layout (location = 1) in vec3 color;\n"
      "out vec4 vertexColor;\n"
      "uniform mat4 transform;\n"
      "void main()\n"
      "{\n"
      "   vertexColor = vec4(color, 1.0);\n"
      "   gl_Position = transform * vec4(aPos, 1.0);\n"
      "}\0";

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // fragment shader
    const char* fragmentShaderSource = "#version 330 core\n"
      "in vec4 vertexColor;\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      "  FragColor = vertexColor;\n"
      "}\0";

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    //Link Program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // bind to VBO
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, &onGridPoint, GL_DYNAMIC_DRAW);

    //bind to VAO
    glGenVertexArrays(1, &vertexArrayBuffer);
    glBindVertexArray(vertexArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), &onGridPoint, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  //=================================================================

  void EditorSystem::update_cursor_gl()
  {
    vertex_2d snappedPos = get_snap_to_grid_pos();
    onGridPoint[0] = vertex_3d(snappedPos.x, snappedPos.y, 1);
    onGridPoint[1] = vertex_3d(1, 0.5, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2, &onGridPoint, GL_DYNAMIC_DRAW);

    //bind to VAO
    glBindVertexArray(vertexArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2, &onGridPoint, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  //=================================================================

  void EditorSystem::draw_cursor()
  {
    viewMatrix = glm::ortho(-windowSize.x / (zoom)+gridOffset.x, windowSize.x / (zoom)+gridOffset.x,
      -windowSize.y / (zoom)+gridOffset.y, windowSize.y / (zoom)+gridOffset.y);

    glUseProgram(shaderProgram);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glPointSize(6);
    glBindVertexArray(vertexArrayBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_POINTS, 0, 1);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindVertexArray(0);
    glUseProgram(0);
  }

  //============================================================
  //
  // GRID
  // 
  //===========================================================

  void Grid::init()
  {
    points.resize((800 + 1) * 2 * 2 * 2);

    size_t i = 0;
    for (f32 x = -50; x <= 50 + GRID_SIZE / 2; x += GRID_SIZE)
    {
      points[i] = vertex_3d(x, -50, 0);
      points[i + 2] = vertex_3d(x, 50, 0);
      points[i + 4] = vertex_3d(-50, x, 0);
      points[i + 6] = vertex_3d(50, x, 0);
      if (i % 32 == 0)
      {
        points[i + 1] = vertex_3d(1.0f, 1.0f, 1.0f);
        points[i + 3] = vertex_3d(1.0f, 1.0f, 1.0f);
        points[i + 5] = vertex_3d(1.0f, 1.0f, 1.0f);
        points[i + 7] = vertex_3d(1.0f, 1.0f, 1.0f);
      }
      else
      {
        points[i + 1] = vertex_3d(0.25f, 0.25f, 0.25f);
        points[i + 3] = vertex_3d(0.25f, 0.25f, 0.25f);
        points[i + 5] = vertex_3d(0.25f, 0.25f, 0.25f);
        points[i + 7] = vertex_3d(0.25f, 0.25f, 0.25f);
      }

      i += 8;
    }

    // vertex shader
    const char* vertexShaderSource = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "layout (location = 1) in vec3 color;\n"
      "out vec4 vertexColor;\n"
      "uniform mat4 transform;\n"
      "void main()\n"
      "{\n"
      "   vertexColor = vec4(color, 1.0);\n"
      "   gl_Position = transform * vec4(aPos, 1.0);\n"
      "}\0";

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // fragment shader
    const char* fragmentShaderSource = "#version 330 core\n"
      "in vec4 vertexColor;\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      "  FragColor = vertexColor;\n"
      "}\0";

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    //Link Program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // bind to VBO
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * points.size(), &points[0], GL_DYNAMIC_DRAW);

    //bind to VAO
    glGenVertexArrays(1, &vertexArrayBuffer);
    glBindVertexArray(vertexArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, points.size() * 3 * sizeof(float), &points[0], GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  //=================================================================

  void Grid::render_grid(vertex_2d windowSize, vertex_2d offset, float zoom)
  {
    offset.x -= floor(offset.x);
    offset.y -= floor(offset.y);

    viewMatrix = glm::ortho(-windowSize.x / (zoom)+offset.x, windowSize.x / (zoom)+offset.x,
      -windowSize.y / (zoom)+offset.y, windowSize.y / (zoom)+offset.y);

    glUseProgram(shaderProgram);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glLineWidth(1);
    glBindVertexArray(vertexArrayBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINES, 0, points.size());
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindVertexArray(0);
    glUseProgram(0);
  }

  Grid::~Grid()
  {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteVertexArrays(1, &vertexArrayBuffer);
  }
}