### Shader Hot Reload
- I need the shader system refactor that would make it possible to hot reload them
- If a shader source changes it should be recompiled on the background and the OpenGL handle should be replaced
- If the compilation fails then the old version should remain and an error log should be produced

### How does the shader loading work now
- All shaders are baked into the source - see shaders.h
- They have separate files, but are represented as c strings
- Then they are loaded and compiled once during the constructor of "Renderer::Render"

### What needs to change
- The shaders can't be baked into the binary anymore
- They will have to be loaded from a separate file on the runtime
- However, I want the API of Renderer to remain the same - the members should be the same
- For example, "m_solid_material" on "Renderer" has a handle to the shader
- After hot reloading the "m_solid_material" should remain there, only the underlying handle should change
- Therefore, all code that used to work with "m_solid_material" would remain unchanged

### Possible implementation
- Keep somewhere in the Renderer a list of shaders (pointers to the members) and a list of the files they were compiled from
- If any of the files changes then you recompile the shader - if succesful then the old program handle will get replaced
- If unsucessful then an error message should be produced and the old shader should be used
- You can run the loop that checks for file changes in "Renderer::Render"
  - This will make sure that shaders will be reloaded before rendering
- You will also have to change the shader files slightly
  - Now each file begins with "constexpr const char* ..."
  - That is because they are included as an header
  - That line will have to be removed

### General Instructions
- Use 10X ide search whenever possible instead of the default grep
