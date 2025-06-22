#include "Shader.h"

#include "AndroidOut.h"
#include "Model.h"

Shader* Shader::loadShader(const std::string& vertexSource, const std::string& fragmentSource,
                           const std::string& positionAttributeName, const std::string& uvAttributeName,
                           const std::string& toClipFromObjectUniformName) {
  aout << "Loading shader with vertex source:\n" << vertexSource << "\nand fragment source:\n" << fragmentSource << std::endl;
  Shader* shader = nullptr;

  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
  if (!vertexShader) {
    aout << "Failed to load vertex shader." << std::endl;
    return nullptr;
  }

  GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
  if (!fragmentShader) {
    glDeleteShader(vertexShader);
    aout << "Failed to load fragment shader." << std::endl;
    return nullptr;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint logLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

      // If we fail to link the shader program, log the result for debugging
      if (logLength) {
        std::vector<GLchar> log(logLength + 1, 0);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        aout << "Failed to link program with:\n" << log.data() << std::endl;
      }

      glDeleteProgram(program);
    } else {
      // Get the attribute and uniform locations by name. You may also choose to hardcode
      // indices with layout= in your shader, but it is not done in this sample
      GLint positionAttribute = glGetAttribLocation(program, positionAttributeName.c_str());
      GLint uvAttribute = glGetAttribLocation(program, uvAttributeName.c_str());
      GLint toClipFromObjectUniform = glGetUniformLocation(program, toClipFromObjectUniformName.c_str());

      // Only create a new shader if all the attributes are found.
      if (positionAttribute != -1 && uvAttribute != -1 && toClipFromObjectUniform != -1) {
        shader = new Shader(program, positionAttribute, uvAttribute, toClipFromObjectUniform);
        aout << "Shader loaded successfully with program ID: " << program << std::endl;
      } else {
        aout << "Failed to find all attributes or uniforms in shader program." << std::endl;
        glDeleteProgram(program);
      }
    }
  }

  // The shaders are no longer needed once the program is linked. Release their memory.
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return shader;
}

GLuint Shader::loadShader(GLenum shaderType, const std::string& shaderSource) {
  GLuint shader = glCreateShader(shaderType);
  if (shader) {
    auto* shaderRawString = (GLchar*)shaderSource.c_str();
    GLint shaderLength = shaderSource.length();
    glShaderSource(shader, 1, &shaderRawString, &shaderLength);
    glCompileShader(shader);

    GLint shaderCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);

    // If the shader doesn't compile, log the result to the terminal for debugging
    if (!shaderCompiled) {
      GLint infoLength = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

      if (infoLength) {
        std::vector<GLchar> infoLog(infoLength + 1, 0);
        glGetShaderInfoLog(shader, infoLength, nullptr, infoLog.data());
        aout << "Failed to compile with:\n" << infoLog.data() << std::endl;
      }

      glDeleteShader(shader);
      shader = 0;
    }
  }
  return shader;
}

void Shader::activate() const {
  glUseProgram(program_);
}

void Shader::deactivate() const {
  glUseProgram(0);
}

void Shader::drawModel(const Model& model) const {
  // The position attribute is 3 floats
  glVertexAttribPointer(position_, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), model.getVertexData());
  glEnableVertexAttribArray(position_);

  // The uv attribute is 2 floats
  glVertexAttribPointer(uv_, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((uint8_t*)model.getVertexData()) + sizeof(r3::Vec3f));
  glEnableVertexAttribArray(uv_);

  // Setup the texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, model.getTexture().getTextureID());

  // Draw as indexed triangles
  glDrawElements(GL_TRIANGLES, model.getIndexCount(), GL_UNSIGNED_SHORT, model.getIndexData());

  glDisableVertexAttribArray(uv_);
  glDisableVertexAttribArray(position_);
}

void Shader::setToClipFromObject(const r3::Matrix4f& toClipFromObject) const {
  glUniformMatrix4fv(toClipFromObject_, 1, false, toClipFromObject.data());
}