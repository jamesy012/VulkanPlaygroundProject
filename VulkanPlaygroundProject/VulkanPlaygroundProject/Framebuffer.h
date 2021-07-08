#pragma once

class RenderPass;

class Framebuffer {
public: 
   bool Create(VkDevice aDevice, VkExtent2D aSize, const RenderPass* aRenderPass, std::vector<VkImageView>& aView);
   void Destroy(VkDevice aDevice);

   const VkFramebuffer GetFramebuffer() const {
      return mFramebuffer;
   }

   void SetName(std::string aName);
private:
   VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
};

