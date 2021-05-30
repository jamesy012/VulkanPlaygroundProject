#pragma once

class RenderPass;
struct VertexType; 

class Pipeline {
public:
   bool AddShader(std::string aPath, bool aForceReload = false);
   void SetVertexType(VertexType& aType);
   bool Create(const VkExtent2D aSize, const RenderPass* aRenderPass);
   void Destroy();

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
   std::vector<VkDynamicState> mDynamicStates;
   VertexType* mVertexType;

   VkPipeline mPipeline = VK_NULL_HANDLE;
   VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
};

