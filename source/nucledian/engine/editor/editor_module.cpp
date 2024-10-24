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
      prevMousePos = curMousePos;
      curMousePos = getMousePos();
      break;
    }
    case ModuleEventType::editor_render:
    {
      draw_ui(windowSize);
      grid.render_grid(windowSize, vertex_2d(), zoom);

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

  EditorSystem::~EditorSystem()
  {
    
  }

  bool EditorSystem::init(SDL_Window* window, void* gl_context)
  {
    zoom = 200.0f;

    gladLoadGLLoader(SDL_GL_GetProcAddress);
    
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

    int width;
    int height;

    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);

    windowSize = vertex_2d(width, height);

    grid.init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    this->window = window;

    return true;
  }

  void EditorSystem::draw_ui(vertex_2d windowSize)
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, windowSize.y - 50.0f));
    ImGui::SetNextWindowSize(ImVec2(windowSize.x, 50.0f));
    ImGui::Begin("Info");

    ImGui::Text("Location: %.2f, %.2f", curMousePos.x, curMousePos.y);
    ImGui::End();
  }

  void EditorSystem::terminate_imgui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  }
  vertex_2d EditorSystem::getMousePos()
  {
    int x;
    int y;
    SDL_GetMouseState(&x, &y); // get pixel coords
    x = x - windowSize.x / 2; // shift
    y = -y + windowSize.y / 2;
    return vertex_2d(x / zoom, y / zoom); // zoom
  }
  //===========================================================

  /**
  void EditorSystem::render_grid()
  {
    grid.render_grid(xOffset, yOffset);
  }
  */

  //===========================================================

  void Grid::init()
  {
    points.resize((800 + 1) * 2 * 2);

    size_t i = 0;
    for (f32 x = -50; x <= 50 + GRID_SIZE / 2; x += GRID_SIZE)
    {
      points[i] = vertex_3d(x, -50, 0);
      points[i + 1] = vertex_3d(x, 50, 0);
      points[i + 2] = vertex_3d(-50, x, 0);
      points[i + 3] = vertex_3d(50, x, 0);

      i += 4;
    }

    // vertex shader
    const char* vertexShaderSource = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "uniform mat4 transform;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = transform * vec4(aPos, 1.0);\n"
      "}\0";

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // fragment shader
    const char* fragmentShaderSource = "#version 330 core\n"
      "out vec4 FragColor;\n"
      "void main()\n"
      "{\n"
      "  FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * points.size(), &points[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //bind to VAO
    glGenVertexArrays(1, &vertexArrayBuffer);
    glBindVertexArray(vertexArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, points.size() * 3 * sizeof(float), &points[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void Grid::render_grid(vertex_2d windowSize, [[maybe_unused]] vertex_2d offset, float zoom)
  {
    viewMatrix = glm::ortho(-windowSize.x / (zoom), windowSize.x / (zoom),
      -windowSize.y / (zoom), windowSize.y / (zoom));
    glClearColor(0, 0, 0, 1);

    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(shaderProgram); 

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glEnableVertexAttribArray(0);

    glLineWidth(1);
    glBindVertexArray(vertexArrayBuffer);
    glDrawArrays(GL_LINES, 0, points.size());
    glDisableVertexAttribArray(0);
  }

  Grid::~Grid()
  {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteVertexArrays(1, &vertexArrayBuffer);
  }

  


  //===========================================================
}