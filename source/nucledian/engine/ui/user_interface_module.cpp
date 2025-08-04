#include <engine/ui/user_interface_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/engine.h>

#include <engine/player/player.h>
#include <engine/player/thing_system.h>

namespace nc
{
  EngineModuleId nc::UserInterfaceSystem::get_module_id()
  {
    return EngineModule::user_interface_system;
  }

  UserInterfaceSystem& UserInterfaceSystem::get()
  {
    return get_engine().get_module<UserInterfaceSystem>();
  }

  bool UserInterfaceSystem::init()
  {
    init_shaders();

    vec2 vertices[] = { vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1)};

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
  }

  UserInterfaceSystem::~UserInterfaceSystem()
  {
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader_program);
  }

  void UserInterfaceSystem::init_shaders()
  {
    const char* vertex_shader_source =
      "#version 330 \n"
      "in vec2 position;\n"
      "out vec2 textureCoords;\n"
      "uniform mat4 transformationMatrix;\n"
      "void main(void) {\n"
      "  gl_Position = vec4(position, 0.0, 1.0);\n"
      "  textureCoords = vec2((position.x + 1.0) / 2.0, 1 - (position.y + 1.0) / 2.0);\n"
      "}\0";


    const char* fragment_shader_source = 
      "#version 330 \n"
      "in vec2 textureCoords; \n"
      "out vec4 out_Color; \n"
      "uniform sampler2D guiTexture; \n"
      "void main(void) {\n" 
      "   out_Color = texture(guiTexture, textureCoords);\n" 
      "}\0";

    unsigned int vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    int  success;
    char infoLog[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
      glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
      glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // deleting shaders as they are linked
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  }

  void UserInterfaceSystem::gather_player_info()
  {
    display_health = get_engine().get_module<ThingSystem>().get_player()->get_health();
  }

  void UserInterfaceSystem::draw()
  {
  }

  void UserInterfaceSystem::on_event(ModuleEvent& event)
  {
    switch (event.type)
    {
    case ModuleEventType::post_init:
      break;
    case ModuleEventType::game_update:
      gather_player_info();
      break;
    case ModuleEventType::render:
      draw();
      break;
    case ModuleEventType::cleanup:
      break;
    default:
      break;
    }
  }

}


