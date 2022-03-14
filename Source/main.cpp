//#include <iostream>
//
//#include <glm/glm.hpp>
//
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//
//#define VMA_IMPLEMENTATION
//#include <vk_mem_alloc.h>
//
//#include <vulkan/vulkan.h>
//
//int main(int, char**) {
//
//    printf("Loading Vulkan sdk: ");
//    uint32_t version = VK_HEADER_VERSION_COMPLETE;
//    printf("Version: %i.%i.%i\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
//
//    glm::vec3 pos = glm::vec3(1, 2, 3) * glm::vec3(5);
//
//    int x, y, mode;
//    void* data = stbi_load("D:/stuff/Libraries/Pictures/lzrecq0bzpp21.jpg", &x, &y, &mode, STBI_rgb_alpha);
//
//    std::cout << "cmake" << std::endl;
//    printf("%f,%f,%f\n", pos.x, pos.y, pos.z);
//    printf("%i,%i\n", x, y);
//
//    stbi_image_free(data);
//}