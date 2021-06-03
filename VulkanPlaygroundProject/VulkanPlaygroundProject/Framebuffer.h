#pragma once

class RenderPass;

class Framebuffer {
public: 
   bool Create(VkDevice aDevice, VkExtent2D aSize, RenderPass* aRenderPass, std::vector<VkImageView>& aView);
   void Destroy(VkDevice aDevice);

   const VkFramebuffer GetFramebuffer() const {
      return mFramebuffer;
   }
private:
   VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
};

