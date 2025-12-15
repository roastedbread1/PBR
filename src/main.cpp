#include "vk_app.h"       
#include "utils_gltf.h"   
#include "camera.h"       
#include <glm/gtx/rotate_vector.hpp>
#include <glm/ext/matrix_clip_space.hpp>


#include <glfw/glfw3.h>


#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

GLTFContext gltf;
GLTFIntrospective gltfInspector = {
    .showAnimations = false,
    .showCameras = false,
    .showMaterials = true,
};


const bool rotateModel = false;
const glm::vec3 kInitialCameraTarget = glm::vec3(0.0f, 0.5f, 0.0f);
const glm::vec3 kInitialCameraPos = glm::vec3(0.0f, 1.0f, -1.5f);
void resizeOffscreenIfNeeded(GLTFContext& gltf, uint32_t width, uint32_t height);

int main()
{
    App app{};
    AppConfig cfg = { .initialCameraPos = kInitialCameraPos, .initialCameraTarget = kInitialCameraTarget , .showGLTFInspector = true };

    init_app(&app, &cfg);

   // generate_BRDF_LUT(app.vkDev, "D:/codes/more codes/c++/PBR/data/brdfLUT.ktx");

   
    
   gltf.inspector = &gltfInspector;
    GLTFContext_init(&gltf, &app);


    /*
    generate_prefiltered_envmap(gltf,
        "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k.hdr",
        "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k_prefilter.ktx",
        "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k_charlie.ktx",
        "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k_irradiance.ktx"); */


    
    std::string model = "CompareIor";

    
    std::string gltfPath = "D:/codes/more codes/c++/PBR/data/glTF-Sample-Assets/Models/" + model + "/glTF/" + model + ".gltf";
    std::string basePath = "D:/codes/more codes/c++/PBR/data/glTF-Sample-Assets/Models/" + model + "/glTF/";

    loadGLTF(gltf, gltfPath.c_str(), basePath.c_str());

    run_app(
        &app,
        [](App* app, uint32_t width, uint32_t height, float aspectRatio, float deltaSeconds)
        {
            VulkanRenderDevice& vkDev = app->vkDev;

            
            if (vkDev.framebufferWidth != width || vkDev.framebufferHeight != height)
            {
                vkDeviceWaitIdle(vkDev.device);
                recreate_swapchain(app);
                resizeOffscreenIfNeeded(gltf, vkDev.framebufferWidth, vkDev.framebufferHeight);
                return;
            }
            //first frame
            if (gltf.offscreenTex.image.image == VK_NULL_HANDLE) {
                vkDeviceWaitIdle(vkDev.device);
                resizeOffscreenIfNeeded(gltf, width, height);
            }


            vkWaitForFences(vkDev.device, 1, &app->inFlightFences[app->currentFrame], VK_TRUE, UINT64_MAX);
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(vkDev.device, vkDev.swapchain, UINT64_MAX,
                app->imageAvailableSemaphores[app->currentFrame], VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                vkDeviceWaitIdle(vkDev.device);
                recreate_swapchain(app);
                resizeOffscreenIfNeeded(gltf, vkDev.framebufferWidth, vkDev.framebufferHeight);
                return;
            }


            vkResetFences(vkDev.device, 1, &app->inFlightFences[imageIndex]);

            VkCommandBuffer cmd = vkDev.commandBuffers[imageIndex];
            vkResetCommandBuffer(cmd, 0);

            VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

            const glm::mat4 m1 = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 m2 = glm::rotate(glm::mat4(1.0f), rotateModel ? (float)glfwGetTime() : 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 model = m2 * m1;

            glm::mat4 view = get_view_matrix_camera(&app->camera);
            glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);
            updateCamera(gltf, model, view, proj, aspectRatio);


            renderGLTF(
                gltf,
                imageIndex,
                cmd,
                app->gridRenderPass,
                app->swapchainFramebuffers[imageIndex],
                vkDev.framebufferWidth, vkDev.framebufferHeight,  
                model, view, proj,
                false
            );
            //TODO:might not needed anymore as I added the barrier on the utils or whatever
            //UPDATE: I DO NEED THIS
            VkImageMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = 0, 
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = vkDev.swapchainImages[imageIndex],
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            VK_CHECK(vkEndCommandBuffer(cmd));


            VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
            VkSemaphore waitSemaphores[] = { app->imageAvailableSemaphores[app->currentFrame] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;
            VkSemaphore signalSemaphores[] = { app->renderFinishedSemaphores[imageIndex] };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            VK_CHECK(vkQueueSubmit(vkDev.graphicsQueue, 1, &submitInfo, app->inFlightFences[imageIndex]));

          
            VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            VkSwapchainKHR swapChains[] = { vkDev.swapchain };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            result = vkQueuePresentKHR(vkDev.graphicsQueue, &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                recreate_swapchain(app);
            }

            app->currentFrame = (app->currentFrame + 1) % app->inFlightFences.size();
        });

    vkDeviceWaitIdle(app.vkDev.device);

    GLTFContext_destroy(&gltf);
    destroy_app(&app);
   

    return 0;
}

void resizeOffscreenIfNeeded(GLTFContext& gltf, uint32_t width, uint32_t height)
{
    VulkanRenderDevice& vkDev = gltf.app->vkDev;

    
    if (gltf.offscreenTex.image.image != VK_NULL_HANDLE &&
        gltf.offscreenTex.width == width &&
        gltf.offscreenTex.height == height)
    {
        return;  
    }

    printf("Resizing offscreen: %dx%d -> %dx%d\n",
        gltf.offscreenTex.width, gltf.offscreenTex.height,
        width, height);

    
    if (gltf.offscreenTex.image.imageView) {
        gltf.textureIndexMap.erase(gltf.offscreenTex.image.imageView);
    }
    if (gltf.offscreenTex.image.image) {
        destroy_vulkan_image(vkDev.device, gltf.offscreenTex.image);
    }
    if (gltf.offscreenFbView) {
        vkDestroyImageView(vkDev.device, gltf.offscreenFbView, nullptr);
        gltf.offscreenFbView = VK_NULL_HANDLE;
    }

    gltf.offscreenTex.width = width;
    gltf.offscreenTex.height = height;
    gltf.offscreenTex.format = VK_FORMAT_B8G8R8A8_UNORM;

    uint32_t mips = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    create_image(vkDev.device, vkDev.physicalDevice, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gltf.offscreenTex.image.image, gltf.offscreenTex.image.imageMemory, 0, mips);

    create_image_view(vkDev.device, gltf.offscreenTex.image.image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
        &gltf.offscreenTex.image.imageView, VK_IMAGE_VIEW_TYPE_2D, 1, mips);
    create_image_view(vkDev.device, gltf.offscreenTex.image.image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
        &gltf.offscreenFbView, VK_IMAGE_VIEW_TYPE_2D, 1, 1);

    
    VkCommandBuffer tempCmd;
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkDev.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(vkDev.device, &allocInfo, &tempCmd);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(tempCmd, &beginInfo);

    transition_image_layout_cmd(tempCmd, gltf.offscreenTex.image.image, VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, mips);

    vkEndCommandBuffer(tempCmd);

    VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &tempCmd };
    vkQueueSubmit(vkDev.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkDev.graphicsQueue);
    vkFreeCommandBuffers(vkDev.device, vkDev.commandPool, 1, &tempCmd);

    get_texture_index(gltf, gltf.offscreenTex);
}