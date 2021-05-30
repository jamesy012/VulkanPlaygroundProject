#include "stdafx.h"
#include "Pipeline.h"

#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>

bool Pipeline::AddShader(std::string aPath) {

   std::ifstream fileStream(aPath);
   std::stringstream dataStream;
   dataStream << fileStream.rdbuf();

   shaderc::Compiler compiler;
   shaderc::CompileOptions options;

   shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(dataStream.str(), shaderc_shader_kind::shaderc_glsl_vertex_shader, aPath.c_str(), options);

   if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
      ASSERT_RET_FALSE("Failed shader compilation");
   }
   std::string preProcessResult = { result.cbegin(), result.cend() };

   result = compiler.CompileGlslToSpvAssembly(dataStream.str(), shaderc_shader_kind::shaderc_glsl_vertex_shader, aPath.c_str(), options);

   if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
      ASSERT_RET_FALSE("Failed shader compilation");
   }
   std::string compileResult = { result.cbegin(), result.cend() };

   shaderc::SpvCompilationResult result2 = compiler.CompileGlslToSpv(dataStream.str(), shaderc_shader_kind::shaderc_glsl_vertex_shader, aPath.c_str(), options);
   if (result2.GetCompilationStatus() != shaderc_compilation_status_success) {
      ASSERT_RET_FALSE("Failed shader compilation");
   }
   std::vector<uint32_t> spvResult = { result2.cbegin(), result2.cend() };

   return true;
}
