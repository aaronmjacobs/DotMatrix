#pragma once

#include "Core/Log.h"
#include "Core/Pointers.h"

#include <glad/gl.h>

#include <array>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef GLM

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
#define RAW_VALUE(x) glm::value_ptr(x)

#else

using vec2 = std::array<float, 2>;
using vec3 = std::array<float, 3>;
using vec4 = std::array<float, 4>;
using mat4 = std::array<float, 16>;
#define RAW_VALUE(x) (x).data()

#endif

namespace ShaderAttribute
{

enum Enum : GLuint
{
   kPosition = 0,
   kNormal = 1,
   kTexCoord = 2,
   kColor = 3,
};

} // namespace ShaderAttribute

union UniformData
{
   bool boolVal;
   int intVal;
   float floatVal;
   vec2 vec2Val;
   vec3 vec3Val;
   vec4 vec4Val;
   mat4 mat4Val;

   UniformData()
   {
      memset(this, 0, sizeof(UniformData));
   }
};

class Uniform
{
public:
   Uniform(const GLint location, const GLenum type, const std::string &name);

   void commit();

   GLenum getType() const
   {
      return type;
   }

   const std::string& getName() const
   {
      return name;
   }

   const UniformData& getActiveData() const
   {
      return activeData;
   }

   UniformData& getPendingData()
   {
      dirty = true;
      return pendingData;
   }

   const UniformData& getPendingData() const
   {
      return pendingData;
   }

   void setValue(bool value);
   void setValue(int value);
   void setValue(GLenum value);
   void setValue(float value);
   void setValue(const vec2 &value);
   void setValue(const vec3 &value);
   void setValue(const vec4 &value);
   void setValue(const mat4 &value);

protected:
   const GLint location;
   const GLenum type;
   const std::string name;
   UniformData activeData;
   UniformData pendingData;
   bool dirty;
};

class Shader;
using UniformMap = std::unordered_map<std::string, SPtr<Uniform>>;

class ShaderProgram
{
public:
   ShaderProgram();
   ShaderProgram(ShaderProgram&& other);
   ~ShaderProgram();

   /**
    * Gets the shader program's handle
    */
   GLuint getID() const
   {
      return id;
   }

   /**
    * Attaches the given shader to the program
    */
   void attach(const SPtr<Shader>& shader);

   /**
    * Links the shader program
    */
   bool link();

   /**
    * Returns whether the program has a uniform with the given name
    */
   bool hasUniform(const std::string& name) const;

   /**
    * Gets the uniform with the given name
    */
   SPtr<Uniform> getUniform(const std::string& name) const;

   /**
    * Commits the values of all uniforms in the shader program
    */
   void commit();

   /**
    * Sets the value of the uniform with the given name
    */
   template<typename T>
   void setUniformValue(const std::string& name, const T& value, bool ignoreFailure = false)
   {
      UniformMap::iterator itr = uniforms.find(name);
      if (itr == uniforms.end())
      {
         if (!ignoreFailure)
         {
            LOG_WARNING("Uniform with given name doesn't exist: " << name);
         }

         return;
      }

      itr->second->setValue(value);
   }

protected:
   /**
   * The shader program's handle
   */
   GLuint id;

   /**
   * All shaders attached to the program
   */
   std::vector<SPtr<Shader>> shaders;

   /**
   * All uniforms in the shader program
   */
   UniformMap uniforms;

   /**
   * Sets the program as the active program
   */
   void use() const;
};
