#pragma once

class RenderPass;
struct VertexType; 

struct ShaderMacroArguments {
   enum Args {
      POSITION_ONLY,
      SIMPLE_SCENE,
   };
   std::vector<Args> mMacros;
   static const std::string ArgsToString(Args aArg) {
      switch (aArg) {
         case POSITION_ONLY: return "POSITION_ONLY";
         case SIMPLE_SCENE: return "SIMPLE_SCENE";
      }
      ASSERT("No string conversion of shader macro");
      return "";
   }
private:

};

class Pipeline {
public:
   void SetShaderMacroArguments(ShaderMacroArguments& aArguments) {
      mShaderMacroArguments = aArguments;
   }
   bool AddShader(std::string aPath, bool aForceReload = false);
   void SetVertexType(VertexType& aType);
   void AddDescriptorSetLayout(VkDescriptorSetLayout aSetLayout);
   bool Create(const VkExtent2D aSize, const RenderPass* aRenderPass);
   void Destroy();

   void SetCullMode(VkCullModeFlags aCullMode) {
      mCullMode = aCullMode;
   }
   void SetBlendingEnabled(bool aBlending) {
      mBlending = aBlending;
   }

   const VkPipeline GetPipeline() const {
      return mPipeline;
   }

   const VkPipelineLayout GetPipelineLayout() const {
      return mPipelineLayout;
   }
private:
   struct Shader {
   public:
      VkPipelineShaderStageCreateInfo mInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
   };
   std::vector<Shader> mShaders;
   ShaderMacroArguments mShaderMacroArguments;
   std::vector<VkDynamicState> mDynamicStates;
   std::vector<VkDescriptorSetLayout> mDescriptorSets;
   VertexType* mVertexType;
   VkCullModeFlags mCullMode = VK_CULL_MODE_BACK_BIT;
   bool mBlending = false;

   VkPipeline mPipeline = VK_NULL_HANDLE;
   VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
};

