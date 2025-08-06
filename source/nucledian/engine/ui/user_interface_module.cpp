#include <engine/ui/user_interface_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/engine.h>

#include <engine/player/player.h>
#include <engine/player/thing_system.h>

#include <stb_image/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    ui_elements.clear();

    init_shaders();

    vec2 vertices[] = { vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1)};

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    load_texture();

    return true;
  }

  UserInterfaceSystem::~UserInterfaceSystem()
  {
    /*glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(shader_program);*/
  }

  void UserInterfaceSystem::init_shaders()
  {
    const char* vertex_shader_source =
      "#version 430 core\n"
      "in vec2 position;\n"     
      "out vec2 textureCoords;\n"
      "uniform mat4 transformationMatrix;\n"
      "uniform int digit;\n"
      "void main(void) {\n"
      "  gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);\n"
      "  textureCoords = vec2((position.x + 1.0) / 2.0, 1 - (position.y + 1.0) / 2.0);\n"
      "  float texX = mod(digit, 8);\n"
      "  float texY = digit / 8;\n"
      "  textureCoords.x = textureCoords.x / 8.0 + texX / 8.0; \n"
      "  textureCoords.y = textureCoords.y / 16.0 + texY / 16.0; \n"
      "}\0";


    const char* fragment_shader_source = 
      "#version 430 core\n"
      "in vec2 textureCoords; \n"
      "out vec4 FragColor; \n"
      "uniform sampler2D guiTexture; \n"
      "void main(void) {\n" 
      "   FragColor = texture(guiTexture, textureCoords);\n" 
      "}\0";

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

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
      glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
  }

  void UserInterfaceSystem::gather_player_info()
  {
    display_health = get_engine().get_module<ThingSystem>().get_player()->get_health();
  }

  void UserInterfaceSystem::draw()
  {
    int health = display_health;
    bool first = true;

    glUseProgram(shader_program);
    //glUniform1i(glGetUniformLocation(shader_program, "guiTexture"), 0);
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < ui_elements.size(); i++)
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, ui_elements[i].get_texture());

      glm::mat4 trans_mat = glm::mat4(1.0f);
      vec2 translate = ui_elements[i].get_position();
      trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
      vec2 scale = ui_elements[i].get_scale();
      trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));     

      unsigned int transformLoc = glGetUniformLocation(shader_program, "transformationMatrix");
      glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans_mat));

      int digit = health % 10;
      digit += 48;

      if (!first && health == 0)
      {
        digit = 0;
      }

      health = health / 10;

      unsigned int digitLoc = glGetUniformLocation(shader_program, "digit");
      glUniform1i(digitLoc, digit);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      first = false;
    }

    glDisable(GL_BLEND);
     
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
  }

  void UserInterfaceSystem::load_texture()
  {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nr_channels;
    unsigned char* data = stbi_load("content/ui/font.png", &width, &height, &nr_channels, 0);

    if (data)
    {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);

      glBindTexture(GL_TEXTURE_2D, 0);
    }

    //GuiTexture element = GuiTexture(texture, vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
    GuiTexture element = GuiTexture(texture, vec2(-0.8f, -0.8f), vec2(0.075f, 0.1f));
    GuiTexture element_2 = GuiTexture(texture, vec2(-0.65f, -0.8f), vec2(0.075f, 0.1f));
    GuiTexture element_3 = GuiTexture(texture, vec2(-0.5f, -0.8f), vec2(0.075f, 0.1f));
    ui_elements.push_back(element_3);
    ui_elements.push_back(element_2);
    ui_elements.push_back(element);

    stbi_image_free(data);
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
      //draw();
      break;
    case ModuleEventType::cleanup:
      break;
    case ModuleEventType::terminate:
      glDeleteBuffers(1, &VAO);
      glDeleteBuffers(1, &VBO);
      glDeleteShader(vertex_shader);
      glDeleteShader(fragment_shader);
      glDeleteProgram(shader_program);
      break;
    default:
      break;
    }
  }

}


