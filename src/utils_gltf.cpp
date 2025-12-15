#include <utils_gltf.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <vk_app.h>
#include <line_canvas.h>
#include <iostream>

///from dilligent engine https://gitlab.icg.tugraz.at/radl2/rtg-diligentengine/-/blob/main/DiligentTools/TextureLoader/src/KTXLoader.cpp?ref_type=heads 
#define GL_RGBA32F            0x8814
#define GL_RGBA32UI           0x8D70
#define GL_RGBA32I            0x8D82
#define GL_RGB32F             0x8815
#define GL_RGB32UI            0x8D71
#define GL_RGB32I             0x8D83
#define GL_RGBA16F            0x881A
#define GL_RGBA16             0x805B
#define GL_RGBA16UI           0x8D76
#define GL_RGBA16_SNORM       0x8F9B
#define GL_RGBA16I            0x8D88
#define GL_RG32F              0x8230
#define GL_RG32UI             0x823C
#define GL_RG32I              0x823B
#define GL_DEPTH32F_STENCIL8  0x8CAD
#define GL_RGB10_A2           0x8059
#define GL_RGB10_A2UI         0x906F
#define GL_R11F_G11F_B10F     0x8C3A
#define GL_RGBA8              0x8058
#define GL_RGBA8UI            0x8D7C
#define GL_RGBA8_SNORM        0x8F97
#define GL_RGBA8I             0x8D8E
#define GL_RG16F              0x822F
#define GL_RG16               0x822C
#define GL_RG16UI             0x823A
#define GL_RG16_SNORM         0x8F99
#define GL_RG16I              0x8239
#define GL_R32F               0x822E
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_R32UI              0x8236
#define GL_R32I               0x8235
#define GL_DEPTH24_STENCIL8   0x88F0
#define GL_RG8                0x822B
#define GL_RG8UI              0x8238
#define GL_RG8_SNORM          0x8F95
#define GL_RG8I               0x8237
#define GL_R16F               0x822D
#define GL_DEPTH_COMPONENT16  0x81A5
#define GL_R16                0x822A
#define GL_R16UI              0x8234
#define GL_R16_SNORM          0x8F98
#define GL_R16I               0x8233
#define GL_R8                 0x8229
#define GL_R8UI               0x8232
#define GL_R8_SNORM           0x8F94
#define GL_R8I                0x8231
#define GL_RGB9_E5            0x8C3D


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT        0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT       0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT       0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT       0x83F3
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT       0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#define GL_COMPRESSED_RED_RGTC1                0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1         0x8DBC
#define GL_COMPRESSED_RG_RGTC2                 0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2          0x8DBE
#define GL_COMPRESSED_RGBA_BPTC_UNORM          0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM    0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT    0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT  0x8E8F



const uint32_t kBrdfW = 256;
const  uint32_t kBrdfH = 256;
const uint32_t kNumSamples = 1024;
const uint32_t kBufferSize = 4u * sizeof(uint16_t) * kBrdfW * kBrdfH;

namespace
{

    static void update_descriptor_slot(VulkanRenderDevice& vkDev, VkDescriptorSet set, uint32_t binding, uint32_t index, VkImageView view, VkSampler sampler) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = sampler,
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = (sampler != VK_NULL_HANDLE) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &imageInfo
        };
        vkUpdateDescriptorSets(vkDev.device, 1, &write, 0, nullptr);
    }

 

    static std::string aitexture_type_to_string(aiTextureType type)
    {
        switch (type)
        {
        case aiTextureType_NONE:
            return "None";
        case aiTextureType_DIFFUSE:
            return "Diffuse";
        case aiTextureType_SPECULAR:
            return "Specular";
        case aiTextureType_AMBIENT:
            return "Ambient";
        case aiTextureType_EMISSIVE:
            return "Emissive";
        case aiTextureType_HEIGHT:
            return "Height";
        case aiTextureType_NORMALS:
            return "Normals";
        case aiTextureType_SHININESS:
            return "Shininess";
        case aiTextureType_OPACITY:
            return "Opacity";
        case aiTextureType_DISPLACEMENT:
            return "Displacement";
        case aiTextureType_LIGHTMAP:
            return "Lightmap";
        case aiTextureType_REFLECTION:
            return "Reflection";
        case aiTextureType_BASE_COLOR:
            return "Base Color";
        case aiTextureType_NORMAL_CAMERA:
            return "Normal Camera";
        case aiTextureType_EMISSION_COLOR:
            return "Emission Color";
        case aiTextureType_METALNESS:
            return "Metalness";
        case aiTextureType_DIFFUSE_ROUGHNESS:
            return "Diffuse Roughness";
        case aiTextureType_AMBIENT_OCCLUSION:
            return "Ambient Occlusion";
        case aiTextureType_UNKNOWN:
            return "Unknown";
        case aiTextureType_SHEEN:
            return "Sheen";
        case aiTextureType_CLEARCOAT:
            return "Clearcoat";
        case aiTextureType_TRANSMISSION:
            return "Transmission";
        case aiTextureType_MAYA_BASE:
            return "Maya Base";
        case aiTextureType_MAYA_SPECULAR:
            return "Maya Specular";
        case aiTextureType_MAYA_SPECULAR_COLOR:
            return "Maya Specular Color";
        case aiTextureType_MAYA_SPECULAR_ROUGHNESS:
            return "Maya Specular Roughness";
        case aiTextureType_ANISOTROPY:
            return "Anisotropy";
        case aiTextureType_GLTF_METALLIC_ROUGHNESS:
            return "Metallic Roughness";
        case _aiTextureType_Force32Bit:
            return "Force32Bit";
        default:
            return "Unknown";
        }
    }

    static void loadMaterialTexture(
        GLTFContext* gltf,
        const aiMaterial* mtlDescriptor, aiTextureType textureType, const char* assetFolder,
        VulkanTexture& textureHandle,
        bool sRGB, int index = 0)
    {
        if (mtlDescriptor->GetTextureCount(textureType) > 0) {
            aiString path;
            if (mtlDescriptor->GetTexture(textureType, index, &path) == AI_SUCCESS) {
                std::string fullPath = std::string(assetFolder) + path.C_Str();


                uint32_t w, h;
                if (create_texture_image(gltf->app->vkDev, fullPath.c_str(), textureHandle.image.image, textureHandle.image.imageMemory, &w, &h, sRGB))
                {
                    textureHandle.width = w;
                    textureHandle.height = h;
                    textureHandle.format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

                    create_image_view(gltf->app->vkDev.device, textureHandle.image.image, textureHandle.format, VK_IMAGE_ASPECT_COLOR_BIT, &textureHandle.image.imageView);


                    transition_image_layout(gltf->app->vkDev, textureHandle.image.image, textureHandle.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


                    get_texture_index(*gltf, textureHandle);
                }
                else {
                    printf("Failed to load texture: %s\n", fullPath.c_str());
                }
            }
        }
    }

    VkDeviceAddress get_buffer_address(VkDevice device, VkBuffer buffer)
    {
        if (buffer == VK_NULL_HANDLE) return 0;
        VkBufferDeviceAddressInfo info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer
        };
        return vkGetBufferDeviceAddress(device, &info);
    }

    uint32_t getNextMtxId(GLTFContext& gltf, const char* name, uint32_t& nextEmptyId, const glm::mat4& mtx)
    {
        const auto it = gltf.bonesByName.find(name);
        const uint32_t mtxId = (it == gltf.bonesByName.end()) ? nextEmptyId++ : it->second.boneId;
        if (gltf.matrices.size() <= mtxId) {
            gltf.matrices.resize(mtxId + 1);
        }
        gltf.matrices[mtxId] = mtx;
        return mtxId;
    }

    uint32_t getNodeId(GLTFContext& gltf, const char* name)
    {
        for (uint32_t i = 0; i != gltf.nodesStorage.size(); i++) {
            if (gltf.nodesStorage[i].name == name)
                return i;
        }
        return ~0;
    }



    static uint32_t get_cubemap_index(GLTFContext& gltf, const VulkanTexture& tex)
    {
        if (tex.image.imageView == VK_NULL_HANDLE) return 0;

        auto it = gltf.cubemapIndexMap.find(tex.image.imageView);
        if (it != gltf.cubemapIndexMap.end()) return it->second;

        uint32_t newIndex = gltf.nextCubemapIndex++;
        if (newIndex >= 16) {
            newIndex = 0;
        }

        gltf.cubemapIndexMap[tex.image.imageView] = newIndex;

        // Update descriptor set 2, binding 0 (kTexturesCube)
        update_descriptor_slot(gltf.app->vkDev, gltf.bindlessSet2, 0, newIndex, tex.image.imageView, VK_NULL_HANDLE);

        return newIndex;
    }

    ktxTexture1* bitmapToCube(Bitmap& bmp, bool mipmaps)
    {
        if (bmp.comp != 3 || bmp.type != eBitmapType_Cube || bmp.fmt != eBitmapFormat_Float)
        {
            printf("wrong bitmap properties\n");
            exit(255);
        }

        const int w = bmp.w;
        const int h = bmp.h;
        const uint32_t mipLevels = mipmaps ? static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1 : 1;


        ktxTextureCreateInfo createInfo =
        {
            .glInternalformat = GL_RGBA32F,
            .vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT,
            .baseWidth = static_cast<uint32_t>(w),
            .baseHeight = static_cast<uint32_t>(h),
            .baseDepth = 1u,
            .numDimensions = 2u,
            .numLevels = mipLevels,
            .numLayers = 1u,
            .numFaces = 6u,
            .generateMipmaps = KTX_FALSE,
        };

        ktxTexture1* cubemap = nullptr;
        if (ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &cubemap) != KTX_SUCCESS)
        {
            printf("failed to create KTX texture\n");
            return nullptr;
        }

        const int numFacePixels = w * h;

        for (size_t face = 0; face < 6; face++)
        {
            const glm::vec3* src = reinterpret_cast<glm::vec3*>(bmp.data.data()) + face * numFacePixels;
            size_t offset = 0;
            ktxTexture_GetImageOffset(ktxTexture(cubemap), 0, 0, face, &offset);
            float* dst = (float*)(cubemap->pData + offset);
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    const glm::vec4 rgba = glm::vec4(src[x + y * w], 1.0f);
                    memcpy(dst, &rgba, sizeof(rgba));
                    dst += 4;
                }
            }
        }


        if (mipmaps)
        {
            uint32_t prevWidth = cubemap->baseWidth;
            uint32_t prevHeight = cubemap->baseHeight;

            for (uint32_t face = 0; face < 6; face++)
            {
                printf(".");

                for (uint32_t miplevel = 1; miplevel < cubemap->numLevels; miplevel++)
                {
                    printf(":");
                    const uint32_t width = prevWidth > 1 ? prevWidth >> 1 : 1;
                    const uint32_t height = prevWidth > 1 ? prevHeight >> 1 : 1;

                    size_t prevOffset = 0;
                    ktxTexture_GetImageOffset(ktxTexture(cubemap), miplevel - 1, 0, face, &prevOffset);
                    size_t offset = 0;
                    ktxTexture_GetImageOffset(ktxTexture(cubemap), miplevel, 0, face, &offset);

                    stbir_resize_float_linear(reinterpret_cast<const float*>(cubemap->pData + prevOffset), prevWidth, prevHeight, 0, 
                        reinterpret_cast<float*>(cubemap->pData + offset), width, height, 0, STBIR_RGBA);


                    prevWidth = width;
                    prevHeight = height;

                }

                prevWidth = cubemap->baseWidth;
                prevHeight = cubemap->baseHeight;
            }
        }

        printf("\n");
        return cubemap;
    }
}
//exposing this since I need it in main
 uint32_t get_texture_index(GLTFContext& gltf, const VulkanTexture& tex)
{

    if (tex.image.imageView == VK_NULL_HANDLE) {
        return 1; //white tex at 1
    }


    auto it = gltf.textureIndexMap.find(tex.image.imageView);
    if (it != gltf.textureIndexMap.end()) {
        return it->second;
    }


    uint32_t newIndex = gltf.nextTextureIndex++;
    if (newIndex >= 1024) {
        printf("Warning: Texture descriptors wrapped around.\n");
        newIndex = 1;
        gltf.nextTextureIndex = 2;
    }

    gltf.textureIndexMap[tex.image.imageView] = newIndex;

    // Update descriptor set 0, binding 3 (kTextures2D)
    update_descriptor_slot(gltf.app->vkDev, gltf.bindlessSet0, 3, newIndex, tex.image.imageView, VK_NULL_HANDLE);

    return newIndex;
}


void prefilter_cubemap(GLTFContext* gltf, ktxTexture1* cube, const char* outputPath, VulkanTexture& srcEnvMap, uint32_t srcEnvMapBindlessIdx, Distribution distribution, uint32_t sampleCount)
{
    VulkanRenderDevice& vkDev = gltf->app->vkDev;
    printf("prefiltering... %d, %u", distribution, sampleCount);

    VkImageCreateInfo imageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent = {cube->baseWidth, cube->baseHeight, 1},
        .mipLevels = cube->numLevels,
        .arrayLayers = 6,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkImage dstImage;
    VkDeviceMemory dstMemory;
    VK_CHECK(vkCreateImage(vkDev.device, &imageInfo, nullptr, &dstImage));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkDev.device, dstImage, &memReqs);

    VkMemoryAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = find_memory_type(vkDev.physicalDevice, memReqs.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };


    VK_CHECK(vkAllocateMemory(vkDev.device, &allocInfo, nullptr, &dstMemory));
    VK_CHECK(vkBindImageMemory(vkDev.device, dstImage, dstMemory, 0));

    std::vector<VkImageView> faceViews;
    for (uint32_t mip = 0; mip < cube->numLevels; mip++)
    {
        for (uint32_t face = 0; face < 6; face++)
        {
            VkImageViewCreateInfo viewInfo= {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = dstImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = mip,
                    .levelCount = 1,
                    .baseArrayLayer = face,
                    .layerCount = 1
                }
            };
            VkImageView view;
            VK_CHECK(vkCreateImageView(vkDev.device, &viewInfo, nullptr, &view));

            faceViews.emplace_back(view);

        }
    }


    struct PushConstants
    {
        uint32_t face;
        float roughness;
        uint32_t sampleCount;
        uint32_t width;
        uint32_t height;
        uint32_t envMap;
        uint32_t distribution;
        uint32_t samplerIdx;
    };

    VkPushConstantRange pcRange =
    {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants)
    };

    std::array<VkDescriptorSetLayout, 4> layouts = {
        gltf->setLayout0, gltf->setLayout1, gltf->setLayout2, gltf->setLayout3
    };

    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 4,
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcRange
    };
    VK_CHECK(vkCreatePipelineLayout(vkDev.device, &layoutInfo, nullptr, &pipelineLayout));

    std::vector<const char*> shaderFiles = {
        "D:/codes/more codes/c++/PBR/data/prefilter/main.vert",
        "D:/codes/more codes/c++/PBR/data/prefilter/main.frag"
    };

    VkPipeline pipeline;
    if (!create_graphics_pipeline(
        vkDev,
        pipelineLayout,
        shaderFiles,
        &pipeline,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  
        VK_FORMAT_R32G32B32A32_SFLOAT,        
        VK_FORMAT_UNDEFINED,                  
        false,                                //d test
        false,                                // d write
        false,                                // blend
        true,                                 // scissor state
        VK_SAMPLE_COUNT_1_BIT,                
        cube->baseWidth,                      
        cube->baseHeight,                     
        0
    ))
    {
        printf("failed to create pipeline\n");
        return;
    }

    VkCommandBuffer  cmd = begin_single_time_commands(vkDev);
    VkDescriptorSet sets [] = { gltf->bindlessSet0, gltf->dummySet1, gltf->bindlessSet2, gltf->dummySet3 };

    VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .image = dstImage,
    .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = cube->numLevels,
        .baseArrayLayer = 0,
        .layerCount = 6
    }
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    uint32_t viewIdx = 0;
    for (uint32_t mip = 0; mip < cube->numLevels; mip++) {
        uint32_t mipWidth = std::max(1u, cube->baseWidth >> mip);
        uint32_t mipHeight = std::max(1u, cube->baseHeight >> mip);

        for (uint32_t face = 0; face < 6; face++) {
            VkRenderingAttachmentInfo colorAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = faceViews[viewIdx++],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {{1.0f, 1.0f, 1.0f, 1.0f}}}
            };

            VkRenderingInfo renderingBegin = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {{0, 0}, {mipWidth, mipHeight}},
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment
            };

            if (vkCmdBeginRenderingKHR) vkCmdBeginRenderingKHR(cmd, &renderingBegin);
            else vkCmdBeginRendering(cmd, &renderingBegin);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 4, sets, 0, nullptr);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            VkViewport viewport = { 0, (float)mipHeight, (float)mipWidth, -(float)mipHeight, 0, 1 };
            VkRect2D scissor = { {0, 0}, {mipWidth, mipHeight} };
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            PushConstants pc = {
                .face = face,
                .roughness = (float)mip / (float)(cube->numLevels - 1),
                .sampleCount = sampleCount,
                .width = srcEnvMap.width,
                .height = srcEnvMap.height,
                .envMap = srcEnvMapBindlessIdx,
                .distribution = (uint32_t)distribution,
                .samplerIdx = 0
            };
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

            vkCmdDraw(cmd, 3, 1, 0, 0);

            if (vkCmdEndRenderingKHR) vkCmdEndRenderingKHR(cmd);
            else vkCmdEndRendering(cmd);
        }
    }
    end_single_time_commands(vkDev, cmd);

    printf("reading from gpu buf");

    VkDeviceSize totalSize = 0;
    for (uint32_t mip = 0; mip < cube->numLevels; mip++)
    {
        totalSize += ktxTexture_GetImageSize(ktxTexture(cube), mip) * 6;
    }

    VkBuffer readbackBuffer;
    VkDeviceMemory readbackMemory;
    create_buffer(vkDev.device, vkDev.physicalDevice, totalSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, readbackBuffer, readbackMemory);

    VkCommandBuffer readCmd = begin_single_time_commands(vkDev);

    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    vkCmdPipelineBarrier(readCmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkDeviceSize bufferOffset = 0;

    for (uint32_t mip = 0; mip < cube->numLevels; mip++)
    {
        uint32_t mipWidth = std::max(1u, cube->baseWidth >> mip);
        uint32_t mipHeight = std::max(1u, cube->baseHeight >> mip);

        for (uint32_t face = 0; face < 6; face++) {
            VkBufferImageCopy copyRegion = 
            {
                .bufferOffset = bufferOffset,
                .imageSubresource = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = mip,
                    .baseArrayLayer = face,
                    .layerCount = 1
                },
                .imageExtent = { mipWidth, mipHeight, 1 }
            };
            vkCmdCopyImageToBuffer(readCmd, dstImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                readbackBuffer, 1, &copyRegion);

            bufferOffset += ktxTexture_GetImageSize(ktxTexture(cube), mip);
        }
    }

    end_single_time_commands(vkDev, readCmd);


    void* mapped;
    vkMapMemory(vkDev.device, readbackMemory, 0, totalSize, 0, &mapped);



    bufferOffset = 0;
    for (uint32_t mip = 0; mip < cube->numLevels; mip++)
    {
        for (uint32_t face = 0; face < 6; face++)
        {
            size_t ktxOffset;
            ktxTexture_GetImageOffset(ktxTexture(cube), mip, 0, face, &ktxOffset);
            size_t mipSize = ktxTexture_GetImageSize(ktxTexture(cube), mip);
            memcpy((uint8_t*)cube->pData + ktxOffset, (uint8_t*)mapped + bufferOffset, mipSize);
            bufferOffset += mipSize;
        }
    }

    vkUnmapMemory(vkDev.device, readbackMemory);

    printf("Saving to %s...\n", outputPath);
    ktxTexture_WriteToNamedFile(ktxTexture(cube), outputPath);

    vkDestroyBuffer(vkDev.device, readbackBuffer, nullptr);
    vkFreeMemory(vkDev.device, readbackMemory, nullptr);


    for (auto view : faceViews) vkDestroyImageView(vkDev.device, view, nullptr);
    vkDestroyPipeline(vkDev.device, pipeline, nullptr);
    vkDestroyPipelineLayout(vkDev.device, pipelineLayout, nullptr);
    vkDestroyImage(vkDev.device, dstImage, nullptr);
    vkFreeMemory(vkDev.device, dstMemory, nullptr);
}


void GLTFGlobalSamplers_init(GLTFGlobalSamplers* samplers, VulkanRenderDevice& vkDev)
{
    create_texture_sampler(vkDev.device, &samplers->clamp, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    create_texture_sampler(vkDev.device, &samplers->wrap, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    create_texture_sampler(vkDev.device, &samplers->mirror, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
}

void GLTFGlobalSamplers_destroy(GLTFGlobalSamplers* samplers, VkDevice device)
{
    vkDestroySampler(device, samplers->clamp, nullptr);
    vkDestroySampler(device, samplers->wrap, nullptr);
    vkDestroySampler(device, samplers->mirror, nullptr);
}



///TODO: CHECK IF LUT AND PREFILTER EXIST AND IF IT DOESNT CALCULATE,
/// PARSE THE FILENAME TO PASS TO EnvironmentMapTextures_init_with_paths 
/// AFTER EITHER CALCULATING OR NOT BUT WE'RE ALWAYS PARSING IT
///MOVE THE SKYBOX HERE, THIS WILL TAKE 3 PARAM (CONTEXT + LUT AND HDR PATH) OR MAYBE TWO SINCE THE LUT IS UNIVERSAL
void EnvironmentMapTextures_init(GLTFContext* gltf)
{
    EnvironmentMapTextures_init_with_paths(
        gltf, "D:/codes/more codes/c++/PBR/data/brdfLUT.ktx", "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k_prefilter.ktx",
        "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k_irradiance.ktx", "D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k_charlie.ktx");
}



void  EnvironmentMapTextures_init_with_paths(
    GLTFContext* gltf, const char* brdfLUT, const char* prefilter,
    const char* irradiance, const char* prefilterCharlie)
{

    //VulkanTexture load_texture(
      //  VulkanRenderDevice & vkDev, const char* fileName, VkImageViewType viewType, bool sRGB)

 /*   typedef enum VkImageViewType {
        VK_IMAGE_VIEW_TYPE_1D = 0,
        VK_IMAGE_VIEW_TYPE_2D = 1,
        VK_IMAGE_VIEW_TYPE_3D = 2,
        VK_IMAGE_VIEW_TYPE_CUBE = 3,
        VK_IMAGE_VIEW_TYPE_1D_ARRAY = 4,
        VK_IMAGE_VIEW_TYPE_2D_ARRAY = 5,
        VK_IMAGE_VIEW_TYPE_CUBE_ARRAY = 6,
        VK_IMAGE_VIEW_TYPE_MAX_ENUM = 0x7FFFFFFF
    } VkImageViewType;*/

    VulkanRenderDevice& vkDev = gltf->app->vkDev; 
    EnvironmentMapTextures* textures = &gltf->envMapTextures;


    textures->texBRDF_LUT = load_texture(vkDev, brdfLUT, VK_IMAGE_VIEW_TYPE_2D, false);
    if (textures->texBRDF_LUT.image.image == nullptr) { assert(0); exit(255); }
    get_texture_index(*gltf, textures->texBRDF_LUT); 

    
    textures->envMapTexture = load_texture(vkDev, prefilter, VK_IMAGE_VIEW_TYPE_CUBE, true);
    if (textures->envMapTexture.image.image == nullptr) { assert(0); exit(255); }
    get_cubemap_index(*gltf, textures->envMapTexture); 

    
    textures->envMapTextureIrradiance = load_texture(vkDev, irradiance, VK_IMAGE_VIEW_TYPE_CUBE, true);
    if (textures->envMapTextureIrradiance.image.image == nullptr) { assert(0); exit(255); }
    get_cubemap_index(*gltf, textures->envMapTextureIrradiance); 

    if (prefilterCharlie) {
        textures->envMapTextureCharlie = load_texture(vkDev, prefilterCharlie, VK_IMAGE_VIEW_TYPE_CUBE, true);
        if (textures->envMapTextureCharlie.image.image == nullptr) { assert(0); exit(255); }
        get_cubemap_index(*gltf, textures->envMapTextureCharlie); // <-- REGISTER IT
    }
}

void EnvironmentMapTextures_destroy(EnvironmentMapTextures* textures, VkDevice device) 
{
     destroy_vulkan_image(device,  textures->texBRDF_LUT.image );
     destroy_vulkan_image(device, textures->envMapTexture.image);
     destroy_vulkan_image(device, textures->envMapTextureCharlie.image);
     destroy_vulkan_image(device, textures->envMapTextureIrradiance.image);
}

glm::mat4 GLTFCamera_getProjection(const GLTFCamera* camera, float windowAspect)
{
    return camera->orthoWidth != 0.0f
        ? glm::ortho(
            -windowAspect / camera->orthoWidth, windowAspect / camera->orthoWidth, -1.0f / camera->orthoWidth,
            1.0f / camera->orthoWidth, camera->far, camera->near)
        : glm::perspective(camera->hFOV, windowAspect == 1.0f ? camera->aspect : windowAspect, camera->near, camera->far);


}


void GLTFContext_init(GLTFContext* context, App* app)
{
    context->app = app;
    context->glTFDataholder.gltfContext = context;
    VulkanRenderDevice& vkDev = app->vkDev;

    GLTFGlobalSamplers_init(&context->samplers, vkDev);

    //desc pool & layouts
    {
        const uint32_t maxTextures2D = 1024;
        const uint32_t maxSamplers = 3;
        const uint32_t maxTexturesCube = 16;

        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures2D + maxTexturesCube + 100 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 } 
        };

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 10,
            .poolSizeCount = 3,
            .pPoolSizes = poolSizes
        };
        VK_CHECK(vkCreateDescriptorPool(vkDev.device, &poolInfo, nullptr, &context->bindlessPool));

        // set 0: samplers, YUV(placeholder), Textures
        VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        VkDescriptorSetLayoutBinding bindings0[] = {
            { 0, VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures2D, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
        };
        VkDescriptorBindingFlags bindingFlags0[] = { 0, 0, bindlessFlags };
        VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlagsInfo0 = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = 3, .pBindingFlags = bindingFlags0
        };
        VkDescriptorSetLayoutCreateInfo layoutInfo0 = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = &layoutFlagsInfo0,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = 3, .pBindings = bindings0
        };
        VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo0, nullptr, &context->setLayout0));

        // set 1 : dummy (placeholder for 3D)
        VkDescriptorSetLayoutBinding binding1 = { 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT };
        VkDescriptorSetLayoutCreateInfo layoutInfo1 = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &binding1 };
        VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo1, nullptr, &context->setLayout1));

        // set 2 : cubemaps
        VkDescriptorSetLayoutBinding binding2 = { 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTexturesCube, VK_SHADER_STAGE_FRAGMENT_BIT };
        VkDescriptorBindingFlags bindingFlags2 = bindlessFlags;
        VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlagsInfo2 = {
             .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
             .bindingCount = 1, .pBindingFlags = &bindingFlags2
        };
        VkDescriptorSetLayoutCreateInfo layoutInfo2 = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = &layoutFlagsInfo2,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = 1, .pBindings = &binding2
        };
        VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo2, nullptr, &context->setLayout2));

        // set 3 : dummy
        VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo1, nullptr, &context->setLayout3)); // Re-use 1 structure

        // pipeline layout
        std::array<VkDescriptorSetLayout, 4> setLayouts = { context->setLayout0, context->setLayout1, context->setLayout2, context->setLayout3 };
        struct PushConstants {
            uint64_t draw; uint64_t materials; uint64_t environments; uint64_t lights;
            uint64_t transforms; uint64_t matrices; uint32_t envId;
            uint32_t transmissionFramebuffer; uint32_t transmissionFramebufferSampler; uint32_t lightsCount;
        };
        VkPushConstantRange pcRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };
        VkPipelineLayoutCreateInfo plInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 4, .pSetLayouts = setLayouts.data(),
            .pushConstantRangeCount = 1, .pPushConstantRanges = &pcRange
        };
        VK_CHECK(vkCreatePipelineLayout(vkDev.device, &plInfo, nullptr, &context->pipelineLayout));

        // alloc sets
        VkDescriptorSetAllocateInfo allocInfo = {
             .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
             .descriptorPool = context->bindlessPool,
             .descriptorSetCount = 1,
             .pSetLayouts = &context->setLayout0
        };
        uint32_t count0 = maxTextures2D;
        VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo0 = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
            .descriptorSetCount = 1, .pDescriptorCounts = &count0
        };
        allocInfo.pNext = &countInfo0;
        VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, &context->bindlessSet0));

        allocInfo.pSetLayouts = &context->setLayout1; allocInfo.pNext = nullptr;
        VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, &context->dummySet1));

        allocInfo.pSetLayouts = &context->setLayout2;
        uint32_t count2 = maxTexturesCube;
        VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo2 = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
            .descriptorSetCount = 1, .pDescriptorCounts = &count2
        };
        allocInfo.pNext = &countInfo2;
        VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, &context->bindlessSet2));

        allocInfo.pSetLayouts = &context->setLayout3; allocInfo.pNext = nullptr;
        VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, &context->dummySet3));

        // set 0 samplers
        VkSampler samplers[] = { context->samplers.clamp, context->samplers.wrap, context->samplers.mirror };
        VkDescriptorImageInfo samplerInfo[3] = {};
        for (int i = 0; i < 3; ++i) samplerInfo[i] = { .sampler = samplers[i] };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = context->bindlessSet0, .dstBinding = 0, .dstArrayElement = 0,
            .descriptorCount = 3, .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = samplerInfo
        };
        vkUpdateDescriptorSets(vkDev.device, 1, &write, 0, nullptr);
    }
    //white tex
    {
        const uint32_t whitePixel = 0xFFFFFFFF;
        create_texture_image_from_data(vkDev,
            context->dummyWhite.image.image,
            context->dummyWhite.image.imageMemory,
            (void*)&whitePixel, 1, 1, VK_FORMAT_R8G8B8A8_UNORM);

        create_image_view(vkDev.device, context->dummyWhite.image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &context->dummyWhite.image.imageView);
        context->dummyWhite.width = 1;
        context->dummyWhite.height = 1;

        transition_image_layout(vkDev, context->dummyWhite.image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        
        update_descriptor_slot(vkDev, context->bindlessSet0, 3, 1, context->dummyWhite.image.imageView, VK_NULL_HANDLE);
        context->textureIndexMap[context->dummyWhite.image.imageView] = 1;
        context->nextTextureIndex = 2;
    }

    EnvironmentMapTextures_init(context);
    context->canvas3d = new LineCanvas3D();
    init_line_canvas3D(context->canvas3d);

    
   /* for (int i = 0; i < 3; ++i) context->offscreenTex[i] = {};

    context->currentOffscreenTex = 0;*/
    context->doublesided = false;
}

void GLTFContext_destroy(GLTFContext* context)
{
    VkDevice device = context->app->vkDev.device;
    
    GLTFGlobalSamplers_destroy(&context->samplers, device);
    EnvironmentMapTextures_destroy(&context->envMapTextures, device);
    destroy_line_canvas3D(context->canvas3d, device);


    ///TODO: FIX ORDER ON VK DESTRUCTION
    vkDestroyDescriptorPool(device, context->bindlessPool, nullptr);
    vkDestroyDescriptorSetLayout(device, context->bindlessLayout, nullptr);

    vkDestroyDescriptorPool(device, context->bindlessPool, nullptr);
    vkDestroyPipelineLayout(device, context->pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, context->setLayout0, nullptr);
    vkDestroyDescriptorSetLayout(device, context->setLayout1, nullptr);
    vkDestroyDescriptorSetLayout(device, context->setLayout2, nullptr);
    vkDestroyDescriptorSetLayout(device, context->setLayout3, nullptr);



    auto destroyBuf = [&](VulkanBuffer& buf)
        {
            if (buf.buffer) vkDestroyBuffer(device, buf.buffer, nullptr);
            if (buf.memory) vkFreeMemory(device, buf.memory, nullptr);
            buf = {};
        };

    destroyBuf(context->envBuffer);
    destroyBuf(context->lightsBuffer);
    destroyBuf(context->perFrameBuffer);
    destroyBuf(context->transformBuffer);
    destroyBuf(context->matricesBuffer);


    //destroyBuf(context->morphStatesBuffer);
    destroyBuf(context->vertexBuffer);
    destroyBuf(context->vertexSkinningBuffer);
    //destroyBuf(context->vertexMorphingBuffer);
    destroyBuf(context->indexBuffer);
    destroyBuf(context->matBuffer);


    

    
    vkDestroyPipeline(device, context->pipelineSolid_Pass1, nullptr);
    vkDestroyPipeline(device, context->pipelineTransparent_Pass1, nullptr);
    vkDestroyPipeline(device, context->pipelineSolid_Pass2, nullptr);
    vkDestroyPipeline(device, context->pipelineTransparent_Pass2, nullptr);
    //vkDestroyPipelineLayout(device, context->pipelineLayoutTransparent, nullptr);
    //vkDestroyPipeline(device, context->pipelineComputeAnimations, nullptr);
    //vkDestroyPipelineLayout(device, context->pipelineLayoutComputeAnimations, nullptr);

    vkDestroyShaderModule(device, context->vert.shaderModule, nullptr);
    vkDestroyShaderModule(device, context->frag.shaderModule, nullptr);
    //vkDestroyShaderModule(device, context->animation.shaderModule, nullptr);

 /*   for (int i = 0; i < 3; i++) {
        destroy_vulkan_image(device, context->offscreenTex[i].image);
        vkDestroyFramebuffer(device, context->offscreenFb[i], nullptr);
    }*/

    destroy_vulkan_image(device, context->offscreenTex.image);
    vkDestroyImageView(device, context->offscreenFbView, nullptr);
   // vkDestroyRenderPass(device, context->offscreenRenderPass, nullptr);
   // vkDestroyRenderPass(device, context->transparentRenderPass, nullptr); 

    for (auto& matTextures : context->glTFDataholder.textures) {
        destroy_vulkan_image(device, matTextures.baseColorTexture.image);
        destroy_vulkan_image(device, matTextures.surfacePropertiesTexture.image);
        destroy_vulkan_image(device, matTextures.normalTexture.image);
        destroy_vulkan_image(device, matTextures.occlusionTexture.image);
        destroy_vulkan_image(device, matTextures.emissiveTexture.image);
        destroy_vulkan_image(device, matTextures.sheenColorTexture.image);
        destroy_vulkan_image(device, matTextures.sheenRoughnessTexture.image);
        destroy_vulkan_image(device, matTextures.clearCoatTexture.image);
        destroy_vulkan_image(device, matTextures.clearCoatRoughnessTexture.image);
        destroy_vulkan_image(device, matTextures.clearCoatNormalTexture.image);
        destroy_vulkan_image(device, matTextures.specularTexture.image);
        destroy_vulkan_image(device, matTextures.specularColorTexture.image);
        destroy_vulkan_image(device, matTextures.transmissionTexture.image);
        destroy_vulkan_image(device, matTextures.thicknessTexture.image);
        destroy_vulkan_image(device, matTextures.iridescenceTexture.image);
        destroy_vulkan_image(device, matTextures.iridescenceThicknessTexture.image);
        destroy_vulkan_image(device, matTextures.anisotropyTexture.image);
        destroy_vulkan_image(device, matTextures.white.image);
    }
}

bool GLTFContext_isScreenCopyRequired(const GLTFContext* context)
{
    return context->isVolumetricMaterial;
}

bool assignUVandSampler(const GLTFGlobalSamplers& samplers, const aiMaterial* mtlDescriptor, aiTextureType textureType, uint32_t& uvIndex, uint32_t& textureSampler, int index)
{
    aiString path;
    aiTextureMapMode mapmode[3] = { aiTextureMapMode_Clamp, aiTextureMapMode_Clamp, aiTextureMapMode_Clamp };
    const bool res = mtlDescriptor->GetTexture(textureType, index, &path, 0, &uvIndex, 0, 0, mapmode) == AI_SUCCESS;
    switch (mapmode[0])
    {
    case aiTextureMapMode_Clamp: textureSampler = 0; break;
    case aiTextureMapMode_Wrap: textureSampler = 1; break;
    case aiTextureMapMode_Mirror: textureSampler = 2; break;
    default: break; //maybe default to 0?
    }
    return res;
}



GLTFMaterialDataGPU setupglTFMaterialData(
    GLTFContext* gltf,
    const aiMaterial* mtlDescriptor, const char* assetFolder,
    bool& useVolumetric, bool& usedoublesided)
{
    GLTFMaterialTextures mat = {};
    uint32_t materialTypeFlags = MaterialType_Invalid;

    uint32_t whiteTexIdx = 1; 

    aiShadingMode shadingMode = aiShadingMode_NoShading;
    if (mtlDescriptor->Get(AI_MATKEY_SHADING_MODEL, shadingMode) == AI_SUCCESS) {
        if (shadingMode == aiShadingMode_Unlit) {
            materialTypeFlags = MaterialType_Unlit;
        }
    }

    
    auto load = [&](aiTextureType type, VulkanTexture& tex, bool srgb, int idx = 0) {
        loadMaterialTexture(gltf, mtlDescriptor, type, assetFolder, tex, srgb, idx);
        };

    load(aiTextureType_BASE_COLOR, mat.baseColorTexture, true);
    load(aiTextureType_METALNESS, mat.surfacePropertiesTexture, false);
    materialTypeFlags = MaterialType_MetallicRoughness;

    load(aiTextureType_LIGHTMAP, mat.occlusionTexture, false);
    load(aiTextureType_EMISSIVE, mat.emissiveTexture, true);
    load(aiTextureType_NORMALS, mat.normalTexture, false);


    load(aiTextureType_SHEEN, mat.sheenColorTexture, true, 0);
    load(aiTextureType_SHEEN, mat.sheenRoughnessTexture, false, 1);
    load(aiTextureType_CLEARCOAT, mat.clearCoatTexture, true, 0);
    load(aiTextureType_CLEARCOAT, mat.clearCoatRoughnessTexture, false, 1);
    load(aiTextureType_CLEARCOAT, mat.clearCoatNormalTexture, false, 2);
    load(aiTextureType_SPECULAR, mat.specularTexture, true, 0);
    load(aiTextureType_SPECULAR, mat.specularColorTexture, true, 1);
    load(aiTextureType_TRANSMISSION, mat.transmissionTexture, true, 0);
    load(aiTextureType_TRANSMISSION, mat.thicknessTexture, true, 1);

    
    auto idx = [&](VulkanTexture& t) {
        return (t.image.image != nullptr) ? get_texture_index(*gltf, t) : whiteTexIdx;
        };

    GLTFMaterialDataGPU res = {
        .baseColorFactor = glm::vec4(1),
        .metallicRoughnessNormalOcclusion = glm::vec4(1),
        .specularGlossiness = glm::vec4(1),
        .sheenFactors = glm::vec4(1),
        .clearcoatTransmissionThickness = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        .specularFactors = glm::vec4(1),
        .attenuation = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .emissiveFactorAlphaCutoff = glm::vec4(0,0,0,0.5f),

        .occlusionTexture = idx(mat.occlusionTexture),
        .emissiveTexture = idx(mat.emissiveTexture),
        .baseColorTexture = idx(mat.baseColorTexture),
        .surfacePropertiesTexture = idx(mat.surfacePropertiesTexture),
        .normalTexture = (mat.normalTexture.image.image) ? get_texture_index(*gltf, mat.normalTexture) : ~0u,

        .sheenColorTexture = idx(mat.sheenColorTexture),
        .sheenRoughnessTexture = idx(mat.sheenRoughnessTexture),
        .clearCoatTexture = idx(mat.clearCoatTexture),
        .clearCoatRoughnessTexture = idx(mat.clearCoatRoughnessTexture),
        .clearCoatNormalTexture = idx(mat.clearCoatNormalTexture),
        .specularTexture = idx(mat.specularTexture),
        .specularColorTexture = idx(mat.specularColorTexture),
        .transmissionTexture = idx(mat.transmissionTexture),
        .thicknessTexture = idx(mat.thicknessTexture),
        .iridescenceTexture = whiteTexIdx,           
        .iridescenceThicknessTexture = whiteTexIdx,      
        .anisotropyTexture = whiteTexIdx,
        .materialTypeFlags = materialTypeFlags,
        .ior = 1.5f
    };
    aiColor4D aiColor;
    if (mtlDescriptor->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS) {
        res.baseColorFactor = glm::vec4(aiColor.r, aiColor.g, aiColor.b, aiColor.a);
    }
    assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_DIFFUSE, res.baseColorTextureUV, res.baseColorTextureSampler);

    if (mtlDescriptor->Get(AI_MATKEY_COLOR_EMISSIVE, aiColor) == AI_SUCCESS) {
        //debug
        // mat.emissiveFactor = vec3(aiColor.r, aiColor.g, aiColor.b);
        res.emissiveFactorAlphaCutoff = glm::vec4(aiColor.r, aiColor.g, aiColor.b, 0.5f);
    }
    assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_EMISSIVE, res.emissiveTextureUV, res.emissiveTextureSampler);

    ai_real emissiveStrength = 1.0f;
    if (mtlDescriptor->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength) == AI_SUCCESS) {
        res.emissiveFactorAlphaCutoff *= glm::vec4(emissiveStrength, emissiveStrength, emissiveStrength, 1.0f);
    }

    if (materialTypeFlags & MaterialType_MetallicRoughness) {
        ai_real metallicFactor;
        if (mtlDescriptor->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS) {
            res.metallicRoughnessNormalOcclusion.x = metallicFactor;
        }
        assignUVandSampler(
           gltf->samplers, mtlDescriptor, aiTextureType_METALNESS, res.surfacePropertiesTextureUV, res.surfacePropertiesTextureSampler);

        ai_real roughnessFactor;
        if (mtlDescriptor->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS) {
            res.metallicRoughnessNormalOcclusion.y = roughnessFactor;
        }
    }
    else if (materialTypeFlags & MaterialType_SpecularGlossiness) {
        ai_real specularFactor[3];
        if (mtlDescriptor->Get(AI_MATKEY_SPECULAR_FACTOR, specularFactor) == AI_SUCCESS) {
            res.specularGlossiness.x = specularFactor[0];
            res.specularGlossiness.y = specularFactor[1];
            res.specularGlossiness.z = specularFactor[2];
        }
        assignUVandSampler(
           gltf-> samplers, mtlDescriptor, aiTextureType_SPECULAR, res.surfacePropertiesTextureUV, res.surfacePropertiesTextureSampler);

        ai_real glossinessFactor;
        if (mtlDescriptor->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossinessFactor) == AI_SUCCESS) {
            res.specularGlossiness.w = glossinessFactor;
        }
    }

    ai_real normalScale;
    if (mtlDescriptor->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), normalScale) == AI_SUCCESS) {
        res.metallicRoughnessNormalOcclusion.z = normalScale;
    }
    assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_NORMALS, res.normalTextureUV, res.normalTextureSampler);

    ai_real occlusionStrength;
    if (mtlDescriptor->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_LIGHTMAP, 0), occlusionStrength) == AI_SUCCESS) {
        res.metallicRoughnessNormalOcclusion.w = occlusionStrength;
    }
    assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_LIGHTMAP, res.occlusionTextureUV, res.occlusionTextureSampler);

    aiString alphaMode = aiString("OPAQUE");
    if (mtlDescriptor->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
        if (alphaMode == aiString("MASK")) {
            res.alphaMode = GLTFMaterialDataGPU::AlphaMode_Mask;
        }
        else if (alphaMode == aiString("BLEND")) {
            res.alphaMode = GLTFMaterialDataGPU::AlphaMode_Blend;
        }
        else {
            res.alphaMode = GLTFMaterialDataGPU::AlphaMode_Opaque;
        }
    }

    ai_real alphaCutoff;
    if (mtlDescriptor->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS) {
        res.emissiveFactorAlphaCutoff.w = alphaCutoff;
    }

    // extensions
    // sheen
    {
        bool useSheen = mat.sheenColorTexture.image.image || mat.sheenRoughnessTexture.image.image;
        aiColor4D sheenColorFactor;
        if (mtlDescriptor->Get(AI_MATKEY_SHEEN_COLOR_FACTOR, sheenColorFactor) == AI_SUCCESS) {
            res.sheenFactors = glm::vec4(sheenColorFactor.r, sheenColorFactor.g, sheenColorFactor.b, sheenColorFactor.a);
            useSheen = true;
        }
        ai_real sheenRoughnessFactor;
        if (mtlDescriptor->Get(AI_MATKEY_SHEEN_ROUGHNESS_FACTOR, sheenRoughnessFactor) == AI_SUCCESS) {
            res.sheenFactors.w = sheenRoughnessFactor;
            useSheen = true;
        }

        if (assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_SHEEN, res.sheenColorTextureUV, res.sheenColorTextureSampler, 0)) {
            useSheen = true;
        }
        if (assignUVandSampler(
            gltf->samplers, mtlDescriptor, aiTextureType_SHEEN, res.sheenRoughnessTextureUV, res.sheenRoughnessTextureSampler, 1)) {
            useSheen = true;
        }

        if (useSheen) {
            res.materialTypeFlags |= MaterialType_Sheen;
        }
    }

    // clear coat
    {
        bool useClearCoat = mat.clearCoatTexture.image.image || mat.clearCoatRoughnessTexture.image.image || mat.clearCoatNormalTexture.image.image;
        ai_real clearcoatFactor;
        if (mtlDescriptor->Get(AI_MATKEY_CLEARCOAT_FACTOR, clearcoatFactor) == AI_SUCCESS) {
            res.clearcoatTransmissionThickness.x = clearcoatFactor;
            useClearCoat = true;
        }

        ai_real clearcoatRoughnessFactor;
        if (mtlDescriptor->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, clearcoatRoughnessFactor) == AI_SUCCESS) {
            res.clearcoatTransmissionThickness.y = clearcoatRoughnessFactor;
            useClearCoat = true;
        }

        if (assignUVandSampler(
            gltf->samplers, mtlDescriptor, aiTextureType_CLEARCOAT, res.clearCoatTextureUV, res.clearCoatNormalTextureSampler, 0)) {
            useClearCoat = true;
        }

        if (assignUVandSampler(
            gltf->samplers, mtlDescriptor, aiTextureType_CLEARCOAT, res.clearCoatRoughnessTextureUV, res.clearCoatRoughnessTextureSampler, 1)) {
            useClearCoat = true;
        }

        if (assignUVandSampler(
            gltf->samplers, mtlDescriptor, aiTextureType_CLEARCOAT, res.clearCoatNormalTextureUV, res.clearCoatNormalTextureSampler, 2)) {
            useClearCoat = true;
        }

        if (useClearCoat) {
            res.materialTypeFlags |= MaterialType_ClearCoat;
        }
    }

    // specular
    {
        bool useSpecular = mat.specularColorTexture.image.image || mat.specularTexture.image.image;

        ai_real specularFactor;
        if (mtlDescriptor->Get(AI_MATKEY_SPECULAR_FACTOR, specularFactor) == AI_SUCCESS) {
            res.specularFactors.w = specularFactor;
            useSpecular = true;
        }

        assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_SPECULAR, res.specularTextureUV, res.specularTextureSampler, 0);

        aiColor4D specularColorFactor;
        if (mtlDescriptor->Get(AI_MATKEY_COLOR_SPECULAR, specularColorFactor) == AI_SUCCESS) {
            res.specularFactors = glm::vec4(specularColorFactor.r, specularColorFactor.g, specularColorFactor.b, res.specularFactors.w);
            useSpecular = true;
        }

        assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_SPECULAR, res.specularColorTextureUV, res.specularColorTextureSampler, 1);

        if (useSpecular) {
            res.materialTypeFlags |= MaterialType_Specular;
        }
    }

    // transmission
    {
        printf("\n=== MATERIAL %s TRANSMISSION DEBUG ===\n", mtlDescriptor->GetName().C_Str());

        bool useTransmission = mat.transmissionTexture.image.image;
        printf("Has transmission texture: %s\n", useTransmission ? "YES" : "NO");

        ai_real transmissionFactor = 0.0f;
        aiReturn result = mtlDescriptor->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmissionFactor);
        printf("AI_MATKEY_TRANSMISSION_FACTOR result: %d (SUCCESS=%d), value=%f\n",
            result, AI_SUCCESS, transmissionFactor);


        if (mtlDescriptor->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmissionFactor) == AI_SUCCESS) {
            res.clearcoatTransmissionThickness.z = transmissionFactor;
            useTransmission = true;
            printf("SET useTransmission = true\n");
        }

        if (useTransmission) {
            res.materialTypeFlags |= MaterialType_Transmission;
            useVolumetric = true;
            printf("SET MaterialType_Transmission flag\n");
        }

        assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_TRANSMISSION, res.transmissionTextureUV, res.transmissionTextureSampler, 0);
        printf("Final flags: 0x%X\n", res.materialTypeFlags);
    }

    {
        bool useVolume = mat.thicknessTexture.image.image;

        ai_real thicknessFactor = 0.0f;
        if (mtlDescriptor->Get(AI_MATKEY_VOLUME_THICKNESS_FACTOR, thicknessFactor) == AI_SUCCESS) {
            res.clearcoatTransmissionThickness.w = thicknessFactor;
            useVolume = true;
        }

        ai_real attenuationDistance = 0.0f;
        if (mtlDescriptor->Get(AI_MATKEY_VOLUME_ATTENUATION_DISTANCE, attenuationDistance) == AI_SUCCESS) {
            res.attenuation.w = attenuationDistance;
            useVolume = true;
        }

        aiColor4D volumeAttenuationColor;
        if (mtlDescriptor->Get(AI_MATKEY_VOLUME_ATTENUATION_COLOR, volumeAttenuationColor) == AI_SUCCESS) {
            res.attenuation.x = volumeAttenuationColor.r;
            res.attenuation.y = volumeAttenuationColor.g;
            res.attenuation.z = volumeAttenuationColor.b;
            useVolume = true;
        }

        if (useVolume) {
            res.materialTypeFlags |= MaterialType_Transmission | MaterialType_Volume;
            useVolumetric = true;
        }


    /*    if ((res.materialTypeFlags & MaterialType_Transmission) && !(res.materialTypeFlags & MaterialType_Volume)) {
            printf("Warning: Material has Transmission but no Volume, adding Volume flag\n");
            res.materialTypeFlags |= MaterialType_Volume;
        }*/

        assignUVandSampler(gltf->samplers, mtlDescriptor, aiTextureType_TRANSMISSION, res.thicknessTextureUV, res.thicknessTextureSampler, 1);
    }

    // ior
    ai_real ior;
    if (mtlDescriptor->Get(AI_MATKEY_REFRACTI, ior) == AI_SUCCESS) {
        res.ior = ior;
    }

    // doublesided
    bool ds = false;
    if (mtlDescriptor->Get(AI_MATKEY_TWOSIDED, ds) == AI_SUCCESS) {
        usedoublesided |= ds;
    }

    mat.wasLoaded = true;

   gltf->glTFDataholder.textures.push_back(std::move(mat));

    return res;
}



static uint32_t getNumVertices(const aiScene& scene)
{
    uint32_t num = 0;
    for (uint32_t i = 0; i != scene.mNumMeshes; i++) {
        num += scene.mMeshes[i]->mNumVertices;
    }
    return num;
}

static uint32_t getNumIndices(const aiScene& scene)
{
    uint32_t num = 0;
    for (uint32_t i = 0; i != scene.mNumMeshes; i++) {
        for (uint32_t j = 0; j != scene.mMeshes[i]->mNumFaces; j++) {
            CHECK(scene.mMeshes[i]->mFaces[j].mNumIndices == 3, __FILE__, __LINE__);
            num += scene.mMeshes[i]->mFaces[j].mNumIndices;
        }
    }
    return num;
}

static uint32_t getNumMorphVertices(const aiScene& scene)
{
    uint32_t num = 0;
    for (uint32_t i = 0; i != scene.mNumMeshes; i++) {
        num += scene.mMeshes[i]->mNumVertices * scene.mMeshes[i]->mNumAnimMeshes;
    }
    return num;
}



void updateLights(GLTFContext& gltf)
{
    
    for (LightDataGPU& light : gltf.lights) {
        if (light.nodeId == -1)
            continue;
        light.position = glm::vec3(gltf.matrices[light.nodeId][3]);
        light.direction = gltf.matrices[light.nodeId] * glm::vec4(light.direction, 0.0);
    }

    
    
    CHECK(gltf.lights.size() <= kMaxLights, __FILE__, __LINE__);

    upload_buffer_data(
        gltf.app->vkDev,                    
        gltf.lightsBuffer.memory,           
        0,                                  
        gltf.lights.data(),                 
        gltf.lights.size() * sizeof(LightDataGPU) 
    );
}

struct SceneReleaser {
    const aiScene* scene;
    ~SceneReleaser() { if (scene) aiReleaseImport(scene); }
};


void loadGLTF(GLTFContext& gltf, const char* gltfName, const char* glTFDataPath)
{
    const aiScene* scene = aiImportFile(gltfName, aiProcess_Triangulate);
    if (!scene || !scene->HasMeshes()) {
        printf("Unable to load %s\n", gltfName);
        exit(255);
    }
    struct SceneReleaser { const aiScene* s; ~SceneReleaser() { if (s) aiReleaseImport(s); } } releaser{ scene };

    VulkanRenderDevice& vkDev = gltf.app->vkDev;
    EnvironmentMapTextures_init(&gltf);

    std::vector<Vertex> vertices;
    std::vector<VertexBoneData> skinningData;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> startVertex;
    std::vector<uint32_t> startIndex;

    startVertex.push_back(0);
    startIndex.push_back(0);

    // meshes
    for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        gltf.meshesRemap[mesh->mName.C_Str()] = m;

        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            const aiVector3D v = mesh->mVertices[i];
            const aiVector3D n = mesh->mNormals ? mesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
            const aiColor4D c = mesh->mColors[0] ? mesh->mColors[0][i] : aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);
            const aiVector3D uv0 = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);
            const aiVector3D uv1 = mesh->mTextureCoords[1] ? mesh->mTextureCoords[1][i] : aiVector3D(0.0f, 0.0f, 0.0f);
            vertices.push_back({
                .position = glm::vec3(v.x, v.y, v.z),
                .normal = glm::vec3(n.x, n.y, n.z),
                .color = glm::vec4(c.r, c.g, c.b, c.a),
                .uv0 = glm::vec2(uv0.x, 1.0f - uv0.y),
                .uv1 = glm::vec2(uv1.x, 1.0f - uv1.y),
                });
        }

        if (skinningData.empty()) skinningData.resize(vertices.size()); // Ensure size matches

        startVertex.push_back((uint32_t)vertices.size());
        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            for (int j = 0; j != 3; j++) indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
        startIndex.push_back((uint32_t)indices.size());
    }

    // mat
    for (unsigned int mtl = 0; mtl < scene->mNumMaterials; ++mtl) {
        const aiMaterial* mtlDescriptor = scene->mMaterials[mtl];
        gltf.matPerFrame.materials[mtl] = setupglTFMaterialData(&gltf, mtlDescriptor, glTFDataPath, gltf.isVolumetricMaterial, gltf.doublesided);
        gltf.inspector->materials.push_back({
             .name = mtlDescriptor->GetName().C_Str(),
             .materialMask = gltf.matPerFrame.materials[mtl].materialTypeFlags,
             .currentMaterialMask = gltf.matPerFrame.materials[mtl].materialTypeFlags,
            });
    }

    // nodes
    uint32_t nextMtxId = 0;
    const char* rootName = scene->mRootNode->mName.C_Str();
    gltf.nodesStorage.push_back({
        .name = rootName,
        .transform = ai_mat4_to_mat4(scene->mRootNode->mTransformation),
        .modelMtxId = getNextMtxId(gltf, rootName, nextMtxId, ai_mat4_to_mat4(scene->mRootNode->mTransformation)),
        });
    gltf.root = 0;

    std::function<void(const aiNode*, GLTFNodeRef)> traverse = [&](const aiNode* aiN, GLTFNodeRef gltfN) {
        for (unsigned int m = 0; m < aiN->mNumMeshes; ++m) {
            uint32_t meshIdx = aiN->mMeshes[m];
            const aiMesh* mesh = scene->mMeshes[meshIdx];
            auto& mat = gltf.matPerFrame.materials[mesh->mMaterialIndex];

            SortingType sort = gltf.matPerFrame.materials[mesh->mMaterialIndex].alphaMode == GLTFMaterialDataGPU::AlphaMode_Blend ? SortingType_Transparent
                : gltf.matPerFrame.materials[mesh->mMaterialIndex].materialTypeFlags & MaterialType_Transmission ? SortingType_Transmission
                : SortingType_Opaque;

            printf("Mesh '%s' (mat %u): typeFlags=0x%X, alphaMode=%u\n",
                mesh->mName.C_Str(), mesh->mMaterialIndex,
                mat.materialTypeFlags, mat.alphaMode);

            if (mat.materialTypeFlags & MaterialType_Transmission) {
                sort = SortingType_Transmission;
                printf("  -> TRANSMISSION\n");
            }
            else if (mat.alphaMode == GLTFMaterialDataGPU::AlphaMode_Blend) {
                sort = SortingType_Transparent;
                printf("  -> TRANSPARENT (alpha blend)\n");
            }
            else {
                printf("  -> OPAQUE\n");
            }

            printf("Material 0 alpha: %f\n", gltf.matPerFrame.materials[0].baseColorFactor.a);

            gltf.meshesStorage.push_back({
                .vertexOffset = startVertex[meshIdx],
                .vertexCount = mesh->mNumVertices,
                .indexOffset = startIndex[meshIdx],
                .indexCount = mesh->mNumFaces * 3,
                .matIdx = mesh->mMaterialIndex,
                .sortingType = sort
                });
            gltf.nodesStorage[gltfN].meshes.push_back((uint32_t)gltf.meshesStorage.size() - 1);
        }
        for (uint32_t i = 0; i < aiN->mNumChildren; ++i) {
            const aiNode* child = aiN->mChildren[i];
            gltf.nodesStorage.push_back({
                .name = child->mName.C_Str(),
                .transform = ai_mat4_to_mat4(child->mTransformation),
                .modelMtxId = getNextMtxId(gltf, child->mName.C_Str(), nextMtxId, gltf.matrices[gltf.nodesStorage[gltfN].modelMtxId] * ai_mat4_to_mat4(child->mTransformation)),
                });
            uint32_t idx = (uint32_t)gltf.nodesStorage.size() - 1;
            gltf.nodesStorage[gltfN].children.push_back(idx);
            traverse(child, idx);
        }
        };
    traverse(scene->mRootNode, gltf.root);

    //buffers
    gltf.maxVertices = vertices.size(); 

    auto createBuf = [&](VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buf, VkDeviceMemory& mem, void* data) {
        VkBuffer staging; VkDeviceMemory stagingMem;
        create_buffer(vkDev.device, vkDev.physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);
        upload_buffer_data(vkDev, stagingMem, 0, data, size);
        create_shader_buffer(vkDev, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, mem);
        copy_buffer(vkDev, staging, buf, size);
        vkDestroyBuffer(vkDev.device, staging, nullptr); vkFreeMemory(vkDev.device, stagingMem, nullptr);
        };

    createBuf(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, gltf.vertexBuffer.buffer, gltf.vertexBuffer.memory, vertices.data());
    createBuf(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, gltf.indexBuffer.buffer, gltf.indexBuffer.memory, indices.data());

    // host visible buf
    VkBufferUsageFlags hvUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkMemoryPropertyFlags hvProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    create_buffer(vkDev.device, vkDev.physicalDevice, sizeof(MaterialsPerFrame), hvUsage, hvProps, gltf.matBuffer.buffer, gltf.matBuffer.memory);
    upload_buffer_data(vkDev, gltf.matBuffer.memory, 0, &gltf.matPerFrame, sizeof(MaterialsPerFrame));

    const EnvironmentsPerFrame envPerFrame = { .environments = {{
         .envMapTexture = get_cubemap_index(gltf, gltf.envMapTextures.envMapTexture),
         .envMapTextureIrradiance = get_cubemap_index(gltf, gltf.envMapTextures.envMapTextureIrradiance),
         .lutBRDFTexture = get_texture_index(gltf, gltf.envMapTextures.texBRDF_LUT),
         .envMapTextureCharlie = get_cubemap_index(gltf, gltf.envMapTextures.envMapTextureCharlie),
    }} };
    create_buffer(vkDev.device, vkDev.physicalDevice, sizeof(envPerFrame), hvUsage, hvProps, gltf.envBuffer.buffer, gltf.envBuffer.memory);
    upload_buffer_data(vkDev, gltf.envBuffer.memory, 0, &envPerFrame, sizeof(envPerFrame));

    create_buffer(vkDev.device, vkDev.physicalDevice, sizeof(LightDataGPU) * kMaxLights, hvUsage, hvProps, gltf.lightsBuffer.buffer, gltf.lightsBuffer.memory);
    updateLights(gltf);

    create_buffer(vkDev.device, vkDev.physicalDevice, sizeof(GLTFFrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, hvProps, gltf.perFrameBuffer.buffer, gltf.perFrameBuffer.memory);

    // pipelines
    VK_CHECK(create_shader_module(vkDev.device, &gltf.vert, "D:/codes/more codes/c++/PBR/data/gltf/main.vert"));
    VK_CHECK(create_shader_module(vkDev.device, &gltf.frag, "D:/codes/more codes/c++/PBR/data/gltf/main.frag"));

    std::vector<VkPipelineShaderStageCreateInfo> stages = {
        shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, gltf.vert, "main"),
        shader_stage_info(VK_SHADER_STAGE_FRAGMENT_BIT, gltf.frag, "main")
    };

    const VkVertexInputBindingDescription vtxBinding = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    const std::vector<VkVertexInputAttributeDescription> vtxAttribs = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) },
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv0) },
        {4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv1) },
    };
    const VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 1, &vtxBinding, (uint32_t)vtxAttribs.size(), vtxAttribs.data() };
    const VkPipelineInputAssemblyStateCreateInfo ia = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE };
    const VkPipelineViewportStateCreateInfo vp = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0, 1, nullptr, 1, nullptr };
    const VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0, VK_SAMPLE_COUNT_1_BIT };
    const VkPipelineDepthStencilStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS };
    const std::vector<VkDynamicState> dyns = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dyn = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, (uint32_t)dyns.size(), dyns.data() };

    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM; 
    VkFormat depthFormat = find_depth_format(vkDev.physicalDevice);

    const VkPipelineRenderingCreateInfo renderingInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, nullptr, 0, 1, &colorFormat, depthFormat, has_stencil_component(depthFormat) ? depthFormat : VK_FORMAT_UNDEFINED };


    // pipeline solid
    VkPipelineRasterizationStateCreateInfo rs = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, gltf.doublesided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0, 0, 0, 1.0f };
    VkPipelineColorBlendAttachmentState att = { VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0xf };
    VkPipelineColorBlendStateCreateInfo cb = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &att };

    VkGraphicsPipelineCreateInfo pi = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, &renderingInfo, 0, (uint32_t)stages.size(), stages.data(), &vi, &ia, nullptr, &vp, &rs, &ms, &ds, &cb, &dyn, gltf.pipelineLayout, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, -1 };
    VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &pi, nullptr, &gltf.pipelineSolid_Pass1));
    VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &pi, nullptr, &gltf.pipelineSolid_Pass2)); // Reuse for pass 2

    
    VkPipelineDepthStencilStateCreateInfo dsTransparent = ds;
    dsTransparent.depthWriteEnable = VK_FALSE;

    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    //alpha blend for pass 1
    VkPipelineColorBlendAttachmentState attTransparent1 = {
        VK_TRUE,
        VK_BLEND_FACTOR_SRC_ALPHA,           
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, 
        VK_BLEND_OP_ADD,                     
        VK_BLEND_FACTOR_ONE,                 
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, 
        VK_BLEND_OP_ADD,                     
        0xf
    };

    VkPipelineColorBlendStateCreateInfo cbTransparent1 = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &attTransparent1
    };

    VkGraphicsPipelineCreateInfo piTrans1 = pi;
    piTrans1.pDepthStencilState = &dsTransparent;
    piTrans1.pColorBlendState = &cbTransparent1;
    piTrans1.pRasterizationState = &rs;

    VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &piTrans1, nullptr, &gltf.pipelineTransparent_Pass1));

    // trans pipeline for Pass 2 , exact blend mode
    VkPipelineColorBlendAttachmentState attTransparent2 = {
        VK_TRUE,
        VK_BLEND_FACTOR_SRC_ALPHA,           
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, 
        VK_BLEND_OP_ADD,                     
        VK_BLEND_FACTOR_ONE,                 
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, 
        VK_BLEND_OP_ADD,                     
        0xf
    };

    VkPipelineColorBlendStateCreateInfo cbTransparent2 = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &attTransparent2
    };

    VkGraphicsPipelineCreateInfo piTrans2 = pi;
    piTrans2.pDepthStencilState = &dsTransparent;
    piTrans2.pColorBlendState = &cbTransparent2;
    piTrans2.pRasterizationState = &rs;

    VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &piTrans2, nullptr, &gltf.pipelineTransparent_Pass2));
    // Cameras
    gltf.cameras.clear();
    for (uint32_t i = 0; i < scene->mNumCameras; ++i) {
        auto* c = scene->mCameras[i];
        gltf.cameras.push_back({
            .name = c->mName.C_Str(),
            .pos = ai_vector3D_to_vec3(c->mPosition), 
            .up = ai_vector3D_to_vec3(c->mUp),
            .nodeIdx = getNodeId(gltf, c->mName.C_Str()),
            .lookAt = ai_vector3D_to_vec3(c->mLookAt), .hFOV = c->mHorizontalFOV,
            .near = c->mClipPlaneNear, .far = c->mClipPlaneFar, .aspect = c->mAspect
            });
    }


    //skybox
    {
        VK_CHECK(create_shader_module(vkDev.device, &gltf.skyboxVert, "D:/codes/more codes/c++/PBR/data/skybox/skybox.vert"));
        VK_CHECK(create_shader_module(vkDev.device, &gltf.skyboxFrag, "D:/codes/more codes/c++/PBR/data/skybox/skybox.frag"));

        std::vector<VkPipelineShaderStageCreateInfo> skyboxStages = {
            shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, gltf.skyboxVert, "main"),
            shader_stage_info(VK_SHADER_STAGE_FRAGMENT_BIT, gltf.skyboxFrag, "main")
        };

        VkPipelineVertexInputStateCreateInfo skyboxVi = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            nullptr, 0, 0, nullptr, 0, nullptr
        };

        VkPipelineInputAssemblyStateCreateInfo skyboxIa = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      nullptr, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo skyboxDs = {
             VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
             nullptr, 0,
             VK_TRUE,
             VK_FALSE,
             VK_COMPARE_OP_LESS_OR_EQUAL,
             VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f
        };

        VkPipelineRasterizationStateCreateInfo skyboxRs = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      nullptr, 0, VK_FALSE, VK_FALSE,
      VK_POLYGON_MODE_FILL,
      VK_CULL_MODE_NONE,
      VK_FRONT_FACE_COUNTER_CLOCKWISE,
      VK_FALSE, 0, 0, 0, 1.0f
        };

        VkPipelineColorBlendAttachmentState skyboxAtt = {
      VK_FALSE,
      VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
      VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
      0xf
        };

        VkPipelineColorBlendStateCreateInfo skyboxCb = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &skyboxAtt
        };


        struct SkyboxPC {
            glm::mat4 view;
            glm::mat4 proj;
            uint32_t texCube;
            uint32_t samplerIdx;
        };


        VkPushConstantRange skyboxPcRange = {
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
         0, sizeof(SkyboxPC)
        };


        std::array<VkDescriptorSetLayout, 4> skyboxLayouts = {
            gltf.setLayout0, gltf.setLayout1, gltf.setLayout2, gltf.setLayout3
        };

        VkPipelineLayoutCreateInfo skyboxLayoutInfo = {
       VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,
       4, skyboxLayouts.data(),
       1, &skyboxPcRange
        };

        VK_CHECK(vkCreatePipelineLayout(vkDev.device, &skyboxLayoutInfo, nullptr, &gltf.skyboxPipelineLayout));


        VkGraphicsPipelineCreateInfo skyboxPi = {
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                &renderingInfo,  
                0,
                (uint32_t)skyboxStages.size(), skyboxStages.data(),
                &skyboxVi,      
                &skyboxIa,      
                nullptr,
                &vp,            
                &skyboxRs,      
                &ms,            
                &skyboxDs,      
                &skyboxCb,      
                &dyn,           
                gltf.skyboxPipelineLayout,
                VK_NULL_HANDLE, 0, VK_NULL_HANDLE, -1
        };

        VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &skyboxPi, nullptr, &gltf.skyboxPipeline));


        //load skybox TODO:: MOVE THIS SOMEWHERE ELSE, THIS IS SO SHIT HOLY FUCK WHAT IS THIS CODEPATH
        gltf.envMapTextures.envMapSkybox = load_texture(vkDev,"D:/codes/more codes/c++/PBR/data/kiara_1_dawn_8k.hdr", VK_IMAGE_VIEW_TYPE_CUBE, true);
        if (gltf.envMapTextures.envMapSkybox.image.image == nullptr) { assert(0); exit(255); }


        printf("Skybox HDR loaded: %ux%u (should be ~256x256 per face for 1k HDR)\n",
            gltf.envMapTextures.envMapSkybox.width, gltf.envMapTextures.envMapSkybox.height);
        get_cubemap_index(gltf, gltf.envMapTextures.envMapSkybox);



    }

}

void renderEnvironmentCubemap(GLTFContext& gltf, VkCommandBuffer buf, const glm::mat4& proj, const glm::mat4& view)
{
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    struct SkyboxPC {
        glm::mat4 view;
        glm::mat4 proj;
        uint32_t texCube;
        uint32_t samplerIdx;
    } skyboxPc = {
        viewNoTranslation,
        proj,
        get_cubemap_index(gltf, gltf.envMapTextures.envMapSkybox),
        0  
    };

    uint32_t skyboxIdx = get_cubemap_index(gltf, gltf.envMapTextures.envMapSkybox);
   //debug
    // printf("Rendering skybox with cubemap index: %u\n", skyboxIdx);

    VkDescriptorSet sets[] = { gltf.bindlessSet0, gltf.dummySet1, gltf.bindlessSet2, gltf.dummySet3 };
    vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.skyboxPipelineLayout, 0, 4, sets, 0, nullptr);

    vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.skyboxPipeline);
    vkCmdPushConstants(buf, gltf.skyboxPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPC), &skyboxPc);

    vkCmdDraw(buf, 36, 1, 0, 0);

}

void renderGLTF(
    GLTFContext& gltf,
    uint32_t currentSwapchainImageIndex,
    VkCommandBuffer buf,
    VkRenderPass, //TODO: moved to dynamic rendering, remove this
    VkFramebuffer, //TODO : and this
    uint32_t swapchainWidth,
    uint32_t swapchainHeight,
    const glm::mat4& model,
    const glm::mat4& view,
    const glm::mat4& proj,
    bool rebuildRenderList)
{
    VulkanRenderDevice& vkDev = gltf.app->vkDev;

    if (gltf.app->depthTexture.image.image == VK_NULL_HANDLE) {
        printf("Warning: Depth texture not initialized\n");
        return;
    }


    const glm::vec4 camPos = glm::inverse(view)[3];

    if (rebuildRenderList || gltf.transforms.empty()) buildTransformsList(gltf);
    sortTransparentNodes(gltf, camPos);
    //ui updates
    if (gltf.inspector) {
        for (uint32_t m = 0; m < gltf.inspector->materials.size(); m++) {
            if (gltf.inspector->materials[m].modified) {
                gltf.inspector->materials[m].modified = false;
                gltf.matPerFrame.materials[m].materialTypeFlags = gltf.inspector->materials[m].currentMaterialMask;
            }
        }
    }



    gltf.frameData = { .model = model, .view = view, .proj = proj, .cameraPos = camPos };
    upload_buffer_data(vkDev, gltf.perFrameBuffer.memory, 0, &gltf.frameData, sizeof(GLTFFrameData));
    updateLights(gltf);
    upload_buffer_data(vkDev, gltf.matBuffer.memory, 0, &gltf.matPerFrame, sizeof(gltf.matPerFrame));

    //uint32_t currentOffscreenIdx = gltf.currentOffscreenTex;
    //VulkanTexture& offscreen = gltf.offscreenTex[currentOffscreenIdx];

    uint32_t currentOffscreenIdx = 0;
    VulkanTexture& offscreen = gltf.offscreenTex;
    uint32_t offscreenIdx = get_texture_index(gltf, offscreen); // Get bindless index
    //printf("Using offscreen texture %u, bindless index %u\n", currentOffscreenIdx, offscreenIdx);

    
    struct PushConstants { uint64_t draw, materials, environments, lights, transforms, matrices; uint32_t envId, transmissionFramebuffer, transmissionFramebufferSampler, lightsCount; };
    PushConstants pc = {
        get_buffer_address(vkDev.device, gltf.perFrameBuffer.buffer), get_buffer_address(vkDev.device, gltf.matBuffer.buffer),
        get_buffer_address(vkDev.device, gltf.envBuffer.buffer), get_buffer_address(vkDev.device, gltf.lightsBuffer.buffer),
        get_buffer_address(vkDev.device, gltf.transformBuffer.buffer), get_buffer_address(vkDev.device, gltf.matricesBuffer.buffer),
        0, offscreenIdx, 0, (uint32_t)gltf.lights.size()
    };

    VkImage swapchainImg = vkDev.swapchainImages[currentSwapchainImageIndex];
    VkImageView swapchainView = vkDev.swapchainImageViews[currentSwapchainImageIndex];
    VkImageView depthView = gltf.app->depthTexture.image.imageView;

    // pass 1 : opaque
    bool screenCopy = GLTFContext_isScreenCopyRequired(&gltf);
    
    //printf("screenCopy=%d, transmissionFramebuffer=%u\n", screenCopy, pc.transmissionFramebuffer);
    

    
    // use offscreen texture if copying, else swapchain 
    VkImageView targetView = screenCopy ? gltf.offscreenFbView : swapchainView;
    VkImage targetImage = screenCopy ? offscreen.image.image : swapchainImg;
    VkImageLayout targetInitialLayout = screenCopy ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

    
    transition_image_layout_cmd(buf, targetImage, VK_FORMAT_B8G8R8A8_UNORM, targetInitialLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);
    transition_image_layout_cmd(buf, gltf.app->depthTexture.image.image, gltf.app->depthTexture.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkClearColorValue clearColor;
    if (gltf.renderSkyboxBackground) {
        clearColor = { {0.0f, 0.0f, 0.0f, 1.0f} };  
    }
    else {
        clearColor = { {gltf.averageEnvColor.r, gltf.averageEnvColor.g, gltf.averageEnvColor.b, 1.0f} };
    }
    VkRenderingAttachmentInfoKHR colorInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR, nullptr, targetView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_RESOLVE_MODE_NONE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {.color = clearColor} };
    VkRenderingAttachmentInfoKHR depthInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR, nullptr, depthView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_RESOLVE_MODE_NONE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {.depthStencil = {1.f,0}} };
    VkRenderingInfoKHR renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO_KHR, nullptr, 0, {{0,0},{swapchainWidth, swapchainHeight}}, 1, 0, 1, &colorInfo, &depthInfo, nullptr };

    if (vkCmdBeginRenderingKHR) vkCmdBeginRenderingKHR(buf, &renderingInfo); else vkCmdBeginRendering(buf, (const VkRenderingInfo*)&renderingInfo);

    VkViewport v = { 0, (float)swapchainHeight, (float)swapchainWidth, -(float)swapchainHeight, 0, 1 }; 
    VkRect2D s = { {0,0},{swapchainWidth,swapchainHeight} };
    vkCmdSetViewport(buf, 0, 1, &v); vkCmdSetScissor(buf, 0, 1, &s);

    VkDescriptorSet sets[] = { gltf.bindlessSet0, gltf.dummySet1, gltf.bindlessSet2, gltf.dummySet3 };
    vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.pipelineLayout, 0, 4, sets, 0, nullptr);

    if (gltf.renderSkyboxBackground) {
        renderEnvironmentCubemap(gltf, buf, proj, view);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.pipelineLayout, 0, 4, sets, 0, nullptr);
    }

    VkDeviceSize off = 0; 
    vkCmdBindVertexBuffers(buf, 0, 1, &gltf.vertexBuffer.buffer, &off); 
    vkCmdBindIndexBuffer(buf, gltf.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    pc.transmissionFramebuffer = 0;
    pc.transmissionFramebufferSampler = 0;
    vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.pipelineSolid_Pass1);
    vkCmdPushConstants(buf, gltf.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

    for (auto id : gltf.opaqueNodes) {
        const auto& m = gltf.meshesStorage[gltf.transforms[id].meshRef];

        vkCmdDrawIndexed(buf, m.indexCount, 1, m.indexOffset, m.vertexOffset, id);
    }


    
    //if (!screenCopy) {
    //    
    //    //draw_grid_with_cam_pos_app(gltf.app, buf, proj * view, glm::vec3(0, -1.0f, 0), gltf.app->camera.positioner.firstPerson.cameraPosition, 1, VK_FORMAT_B8G8R8A8_UNORM);
    //}

    if (vkCmdEndRenderingKHR) vkCmdEndRenderingKHR(buf); else vkCmdEndRendering(buf);

    // pass 2: transmission/transparent 
    if (!gltf.transmissionNodes.empty() || !gltf.transparentNodes.empty())
    {
       
        
        if (screenCopy) {
            
            //debug
            /*       uint32_t verifyIdx = get_texture_index(gltf, offscreen);
            printf("Pass 2: computed offscreenIdx=%u, re-verified=%u\n", offscreenIdx, verifyIdx);

            if (verifyIdx != offscreenIdx) {
                printf("ERROR: Texture index changed!\n");
            }*/

            uint32_t mips = static_cast<uint32_t>(std::floor(std::log2(std::max(offscreen.width, offscreen.height)))) + 1;


            
            //color attachment optimal from pass 1 -> transfer src optimal (read then copy to swapchain)          
            transition_image_layout_cmd(buf, offscreen.image.image, VK_FORMAT_B8G8R8A8_UNORM,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 1);

           
            // (irrelevant frame) undefined -> transfer dst
            transition_image_layout_cmd(buf, swapchainImg, VK_FORMAT_B8G8R8A8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);

           //blit to swapchain for the opaque bg for the transparent pass
            VkImageBlit blit = {
                .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .srcOffsets = { {0,0,0}, {(int32_t)swapchainWidth, (int32_t)swapchainHeight, 1} },
                .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .dstOffsets = { {0,0,0}, {(int32_t)swapchainWidth, (int32_t)swapchainHeight, 1} }
            };
            vkCmdBlitImage(buf, offscreen.image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                swapchainImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_NEAREST);

            // === Step D: Prepare Swapchain for Rendering ===
            // Swapchain goes TRANSFER_DST -> COLOR_ATTACHMENT
            transition_image_layout_cmd(buf, swapchainImg, VK_FORMAT_B8G8R8A8_UNORM,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);

            // === Step E: Generate Mips for Offscreen ===
            // Offscreen Mip 0 is currently in TRANSFER_SRC_OPTIMAL (from Step A).
            // We pass TRANSFER_SRC_OPTIMAL to generate_mipmaps so it knows the starting state.
            generate_mipmaps(vkDev.physicalDevice, buf, offscreen.image.image, VK_FORMAT_B8G8R8A8_UNORM,
                swapchainWidth, swapchainHeight, mips,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // <--- Current Layout
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); // <--- Final Layout

            update_descriptor_slot(gltf.app->vkDev, gltf.bindlessSet0, 3, offscreenIdx,
                offscreen.image.imageView, VK_NULL_HANDLE);


            
            pc.transmissionFramebuffer = offscreenIdx;
            pc.transmissionFramebufferSampler = 0;
        }

        // Begin Pass 2 (Load existing background)
        colorInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorInfo.imageView = swapchainView; // Always target swapchain now
        depthInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        if (vkCmdBeginRenderingKHR) vkCmdBeginRenderingKHR(buf, &renderingInfo); else vkCmdBeginRendering(buf, (const VkRenderingInfo*)&renderingInfo);

        VkDescriptorSet sets[] = { gltf.bindlessSet0, gltf.dummySet1, gltf.bindlessSet2, gltf.dummySet3 };
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.pipelineLayout, 0, 4, sets, 0, nullptr);

        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(buf, 0, 1, &gltf.vertexBuffer.buffer, &off);
        vkCmdBindIndexBuffer(buf, gltf.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        VkViewport v = { 0, (float)swapchainHeight, (float)swapchainWidth, -(float)swapchainHeight, 0, 1 };
        VkRect2D s = { {0,0},{swapchainWidth,swapchainHeight} };
        vkCmdSetViewport(buf, 0, 1, &v);
        vkCmdSetScissor(buf, 0, 1, &s);

        vkCmdPushConstants(buf, gltf.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

        // Transmission Opaque (Solid Pipeline)
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.pipelineSolid_Pass2);
        for (auto id : gltf.transmissionNodes) {
            const auto& m = gltf.meshesStorage[gltf.transforms[id].meshRef];
            vkCmdDrawIndexed(buf, m.indexCount, 1, m.indexOffset, m.vertexOffset, id);
        }

        // Transparent (Blend Pipeline)
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gltf.pipelineTransparent_Pass2);
        for (auto id : gltf.transparentNodes) {
            const auto& m = gltf.meshesStorage[gltf.transforms[id].meshRef];
            vkCmdDrawIndexed(buf, m.indexCount, 1, m.indexOffset, m.vertexOffset, id);
        }

        if (screenCopy) {
           // draw_grid_with_cam_pos_app(gltf.app, buf, proj * view, glm::vec3(0, -1.0f, 0), gltf.app->camera.positioner.firstPerson.cameraPosition, 1, VK_FORMAT_B8G8R8A8_UNORM);
        }

        if (vkCmdEndRenderingKHR) vkCmdEndRenderingKHR(buf); else vkCmdEndRendering(buf);
    }

    //might be better if we're doing it also in the opaque when we're about to blit
    {
        
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        
        if (gltf.inspector) {
            if (gltf.app->cfg.showGLTFInspector) {
                draw_GTF_inspector_app(gltf.app, *gltf.inspector);
            }
        }
        draw_fps(gltf.app);


        drawSelector(&gltf);
        drawGizmo(gltf);

        ImGui::Render();

        
        VkRenderingAttachmentInfo colorAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = vkDev.swapchainImageViews[currentSwapchainImageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };

        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { {0, 0}, {swapchainWidth, swapchainHeight} },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        if (vkCmdBeginRenderingKHR)
            vkCmdBeginRenderingKHR(buf, &renderingInfo);
        else
            vkCmdBeginRendering(buf, &renderingInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buf);

        if (vkCmdEndRenderingKHR)
            vkCmdEndRenderingKHR(buf);
        else
            vkCmdEndRendering(buf);
    }

   // gltf.currentOffscreenTex = (gltf.currentOffscreenTex + 1) % 3;
}

//MaterialType detectMaterialType(const aiMaterial* mtl) {
//    aiShadingMode mode;
//    if (mtl->Get(AI_MATKEY_SHADING_MODEL, mode) == AI_SUCCESS && mode == aiShadingMode_Unlit) return MaterialType_Unlit;
//    return MaterialType_MetallicRoughness;
//}



void updateCamera(GLTFContext& gltf, const glm::mat4& model, glm::mat4& view, glm::mat4& proj, float aspectRatio)
{

    if (gltf.inspector->activeCamera == ~0u || gltf.inspector->activeCamera >= gltf.cameras.size())
        return;

    const GLTFCamera& cam = gltf.cameras[gltf.inspector->activeCamera];
    if (cam.nodeIdx == ~0u)
        return;

    view = glm::inverse(model * gltf.matrices[cam.nodeIdx]);
    proj = GLTFCamera_getProjection(&cam, aspectRatio);
}


void buildTransformsList(GLTFContext& gltf)
{
    gltf.transforms.clear();
    gltf.opaqueNodes.clear();
    gltf.transmissionNodes.clear();
    gltf.transparentNodes.clear();

    std::function<void(GLTFNodeRef gltfNode)> traverseTree = [&](GLTFNodeRef nodeRef) {
        GLTFNode& node = gltf.nodesStorage[nodeRef];
        for (GLTFNodeRef meshId : node.meshes) {
            const GLTFMesh& mesh = gltf.meshesStorage[meshId];
            gltf.transforms.push_back({
                .modelMtxId = node.modelMtxId,
                .matId = mesh.matIdx,
                .nodeRef = nodeRef,
                .meshRef = meshId,
                .sortingType = mesh.sortingType,
                });
            if (mesh.sortingType == SortingType_Transparent) {
                gltf.transparentNodes.push_back(gltf.transforms.size() - 1);
                printf("Node %s mesh %u -> TRANSPARENT list\n", node.name.c_str(), meshId);
            }
            else if (mesh.sortingType == SortingType_Transmission) {
                gltf.transmissionNodes.push_back(gltf.transforms.size() - 1);
                printf("Node %s mesh %u -> TRANSMISSION list\n", node.name.c_str(), meshId);
            }
            else {
                gltf.opaqueNodes.push_back(gltf.transforms.size() - 1);
                printf("Node %s mesh %u -> OPAQUE list\n", node.name.c_str(), meshId);
            }
        }
        for (GLTFNodeRef child : node.children) {
            traverseTree(child);
        }
        };

    traverseTree(gltf.root);

    VulkanRenderDevice& vkDev = gltf.app->vkDev;
    VkDevice device = vkDev.device;


    if (gltf.transformBuffer.buffer) vkDestroyBuffer(device, gltf.transformBuffer.buffer, nullptr);
    if (gltf.transformBuffer.memory) vkFreeMemory(device, gltf.transformBuffer.memory, nullptr);
    if (gltf.matricesBuffer.buffer) vkDestroyBuffer(device, gltf.matricesBuffer.buffer, nullptr);
    if (gltf.matricesBuffer.memory) vkFreeMemory(device, gltf.matricesBuffer.memory, nullptr);

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    //  transforms buffer
    size_t transformsSize = gltf.transforms.size() * sizeof(GLTFTransforms);
    create_buffer(device, vkDev.physicalDevice, transformsSize, usage, props, gltf.transformBuffer.buffer, gltf.transformBuffer.memory);
    upload_buffer_data(vkDev, gltf.transformBuffer.memory, 0, gltf.transforms.data(), transformsSize);
    gltf.transformBuffer.size = transformsSize;

    // matrices buffer
    size_t matricesSize = gltf.matrices.size() * sizeof(glm::mat4);
    create_buffer(device, vkDev.physicalDevice, matricesSize, usage, props, gltf.matricesBuffer.buffer, gltf.matricesBuffer.memory);
    upload_buffer_data(vkDev, gltf.matricesBuffer.memory, 0, gltf.matrices.data(), matricesSize);
    gltf.matricesBuffer.size = matricesSize;
}

void sortTransparentNodes(GLTFContext& gltf, const glm::vec3& cameraPos)
{
    // glTF spec expects to sort based on pivot positions (not sure correct way though)
    std::sort(gltf.transparentNodes.begin(), gltf.transparentNodes.end(), [&](uint32_t a, uint32_t b) {
        float sqrDistA = glm::length2(cameraPos - glm::vec3(gltf.matrices[gltf.transforms[a].modelMtxId][3]));
        float sqrDistB = glm::length2(cameraPos - glm::vec3(gltf.matrices[gltf.transforms[b].modelMtxId][3]));
        return sqrDistA < sqrDistB;
        });
}


MaterialType detectMaterialType(const aiMaterial* mtl)
{
    aiShadingMode shadingMode = aiShadingMode_NoShading;

    if (mtl->Get(AI_MATKEY_SHADING_MODEL, shadingMode) == AI_SUCCESS) {
        if (shadingMode == aiShadingMode_Unlit) {
            return MaterialType_Unlit;
        }
    }

    if (shadingMode == aiShadingMode_PBR_BRDF) {
        ai_real factor = 0;
        if (mtl->Get(AI_MATKEY_GLOSSINESS_FACTOR, factor) == AI_SUCCESS) {
            return MaterialType_SpecularGlossiness;
        }
        else if (mtl->Get(AI_MATKEY_METALLIC_FACTOR, factor) == AI_SUCCESS) {
            return MaterialType_MetallicRoughness;
        }
    }

    printf("Unknown material type\n");

    return MaterialType_Invalid;
}

void printPrefix(int ofs)
{
    for (int i = 0; i < ofs; i++)
        printf("\t");
}

void printMat4(const aiMatrix4x4& m)
{
    if (!m.IsIdentity()) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                printf("%f ;", m[i][j]);
    }
    else {
        printf(" Identity");
    }
}


//std::vector<std::string> animationsGLTF(GLTFContext& gltf)
//{
//    std::vector<std::string> names;
//    names.reserve(gltf.animations.size());
//
//    for (auto c : gltf.animations) {
//        names.push_back(c.name);
//    }
//
//    return names;
//}

//void animateGLTF(GLTFContext& gltf, AnimationState& anim, float dt)
//{
//    if (gltf.transforms.empty())
//        return;
//
//    if (gltf.pipelineComputeAnimations.empty()) {
//        gltf.pipelineComputeAnimations = gltf.app->ctx->createComputePipeline({
//            .smComp = gltf.animation,
//            });
//    }
//
//    // we support only one single animation at this time
//    anim.active = anim.animId != ~0;
//    gltf.animated = anim.active;
//    if (anim.active) {
//        updateAnimation(gltf, anim, dt);
//    }
//}

//void animateBlendingGLTF(GLTFContext& gltf, AnimationState& anim1, AnimationState& anim2, float weight, float dt)
//{
//    if (gltf.transforms.empty())
//        return;
//
//    if (gltf.pipelineComputeAnimations.empty()) {
//        gltf.pipelineComputeAnimations = gltf.app->ctx->createComputePipeline({
//            .smComp = gltf.animation,
//            });
//    }
//
//    anim1.active = anim1.animId != ~0;
//    anim2.active = anim2.animId != ~0;
//    gltf.animated = anim1.active || anim2.active;
//    if (anim1.active && anim2.active) {
//        updateAnimationBlending(gltf, anim1, anim2, weight, dt);
//    }
//    else if (anim1.active) {
//        updateAnimation(gltf, anim1, dt);
//    }
//    else if (anim2.active) {
//        updateAnimation(gltf, anim2, dt);
//    }
//}

std::vector<std::string> camerasGLTF(GLTFContext& gltf)
{
    std::vector<std::string> names;
    names.reserve(gltf.cameras.size() + 1);

    for (auto c : gltf.cameras) {
        names.push_back(c.name);
    }
    names.push_back("Application cam");

    return names;
}

void generate_BRDF_LUT(VulkanRenderDevice& vkDev, const char* outputPath)
{

    ShaderModule compShader;
    VK_CHECK(create_shader_module(vkDev.device, &compShader, "D:/codes/more codes/c++/PBR/data/LUT/main.comp"));

    VkSpecializationMapEntry specEntry =
    {
        .constantID = 0,
        .offset = 0,
        .size = sizeof(kNumSamples),
    };

    VkSpecializationInfo specInfo =
    {
        .mapEntryCount = 1,
        .pMapEntries = &specEntry,
        .dataSize = sizeof(kNumSamples),
        .pData = &kNumSamples
    };

    VkPipelineShaderStageCreateInfo shaderStage =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compShader.shaderModule,
        .pName = "main",
        .pSpecializationInfo = &specInfo,
    };

    struct PushConstants
    {
        uint32_t w;
        uint32_t h;
        uint64_t dataAddr;

    };

    VkPushConstantRange pcRange =
    {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    VkPipelineLayoutCreateInfo layoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcRange,
    };

    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(vkDev.device, &layoutInfo, nullptr, &pipelineLayout));

    VkComputePipelineCreateInfo pipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shaderStage,
        .layout = pipelineLayout,
    };

    VkPipeline pipeline;
    VK_CHECK(vkCreateComputePipelines(vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    VkBuffer dstBuffer;

    VkDeviceMemory dstMemory;
    create_buffer(vkDev.device, vkDev.physicalDevice, kBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        dstBuffer, dstMemory);

    VkBufferDeviceAddressInfo addressInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = dstBuffer,

    };

    uint64_t bufferAddress = vkGetBufferDeviceAddress(vkDev.device, &addressInfo);

    VkCommandBuffer cmd = begin_single_time_commands(vkDev);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    
    PushConstants pc =
    {
        .w = kBrdfW,
        .h = kBrdfH,
        .dataAddr = bufferAddress
    };

    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pc);

    vkCmdDispatch(cmd, kBrdfW / 16, kBrdfH / 16, 1);

    VkMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
    };

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,0, 1, &barrier, 0, nullptr, 0, nullptr);

    end_single_time_commands(vkDev, cmd);

    void* mapped;
    vkMapMemory(vkDev.device, dstMemory, 0, kBufferSize, 0, &mapped);
    
    ktxTextureCreateInfo createInfo =
    {
        .glInternalformat = GL_RGBA16,
        .vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
        .baseWidth = kBrdfW,
        .baseHeight = kBrdfH,
        .baseDepth = 1,
        .numDimensions = 2,
        .numLevels  = 1,  
        .numLayers = 1, 
        .numFaces = 1, 
        .generateMipmaps = KTX_FALSE,
    };

    ktxTexture1* lutTexture = nullptr;

    if (ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &lutTexture) != KTX_SUCCESS)
    {
        printf("failed to create ktx texture\n");
        vkUnmapMemory(vkDev.device, dstMemory);
        return;
    }


    memcpy(lutTexture->pData, mapped, kBufferSize);
    vkUnmapMemory(vkDev.device, dstMemory);

    printf("saving LUT to %s...\n", outputPath);
    ktxTexture_WriteToNamedFile(ktxTexture(lutTexture), outputPath);


    ktxTexture_Destroy(ktxTexture(lutTexture));
    vkDestroyBuffer(vkDev.device, dstBuffer, nullptr);
    vkFreeMemory(vkDev.device, dstMemory, nullptr);
    vkDestroyPipeline(vkDev.device, pipeline, nullptr);
    vkDestroyPipelineLayout(vkDev.device, pipelineLayout, nullptr);
    vkDestroyShaderModule(vkDev.device, compShader.shaderModule, nullptr);

    printf("BRDF LUT generated");
}

void generate_prefiltered_envmap(GLTFContext& gltf, const char* inputHDR, const char* outputGGX, const char* outputCharlie, const char* outputIrradiance)
{
    VulkanRenderDevice& vkDev = gltf.app->vkDev;
    

    int w, h;
    const float* img = stbi_loadf(inputHDR, &w, &h, nullptr, 3);
    if (!img)
    {
        printf("error: failed to load HDR");
        return;
    }
    SCOPE_EXIT{ stbi_image_free((void*)img); };


    Bitmap in;
    init_bitmap_from_data(&in, w, h, 3, eBitmapFormat_Float, img);
    Bitmap verticalCross = convert_equirectangular_map_to_vertial_cross(in);
    Bitmap cubemap = convert_vertical_cross_to_cubemap_faces(verticalCross);

    printf("dimension: %dx%d", cubemap.w, cubemap.h);

    ktxTexture1* specularCube = bitmapToCube(cubemap, true);
    if (!specularCube) return;

    VulkanTexture  srcEnvMap = {};
    srcEnvMap.width = cubemap.w;
    srcEnvMap.height= cubemap.h;
    srcEnvMap.depth= 6;
    srcEnvMap.format = VK_FORMAT_R32G32B32A32_SFLOAT;


    VkImageCreateInfo srcImageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent = { specularCube->baseWidth, specularCube->baseHeight, 1 },
        .mipLevels = specularCube->numLevels,
        .arrayLayers = 6,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK(vkCreateImage(vkDev.device, &srcImageInfo, nullptr, &srcEnvMap.image.image));

    VkMemoryRequirements srcMemReqs;
    vkGetImageMemoryRequirements(vkDev.device, srcEnvMap.image.image, &srcMemReqs);

    VkMemoryAllocateInfo srcAllocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = srcMemReqs.size,
        .memoryTypeIndex = find_memory_type(vkDev.physicalDevice,  srcMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_CHECK(vkAllocateMemory(vkDev.device, &srcAllocInfo, nullptr, &srcEnvMap.image.imageMemory));
    VK_CHECK(vkBindImageMemory(vkDev.device, srcEnvMap.image.image, srcEnvMap.image.imageMemory, 0));

    VkDeviceSize ktxDataSize = 0;
    for (uint32_t mip = 0; mip < specularCube->numLevels; mip++) {
        for (uint32_t face = 0; face < 6; face++) {
            size_t offset;
            ktxTexture_GetImageOffset(ktxTexture(specularCube), mip, 0, face, &offset);
            ktxDataSize = std::max(ktxDataSize, (VkDeviceSize)(offset + ktxTexture_GetImageSize(ktxTexture(specularCube), mip)));
        }
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    create_buffer(vkDev.device, vkDev.physicalDevice, ktxDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    upload_buffer_data(vkDev, stagingMemory, 0, specularCube->pData, ktxDataSize);
    VkCommandBuffer uploadCmd = begin_single_time_commands(vkDev);
    transition_image_layout_cmd(uploadCmd, srcEnvMap.image.image, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, specularCube->numLevels);

    std::vector<VkBufferImageCopy> copyRegions;
    for (uint32_t mip = 0; mip < specularCube->numLevels; mip++) {
        for (uint32_t face = 0; face < 6; face++) {
            size_t offset;
            ktxTexture_GetImageOffset(ktxTexture(specularCube), mip, 0, face, &offset);

            VkBufferImageCopy region = {
                .bufferOffset = offset,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = mip,
                    .baseArrayLayer = face,
                    .layerCount = 1
                },
                .imageExtent = {
                    std::max(1u, specularCube->baseWidth >> mip),
                    std::max(1u, specularCube->baseHeight >> mip),
                    1
                }
            };
            copyRegions.push_back(region);
        }
    }

    vkCmdCopyBufferToImage(uploadCmd, stagingBuffer, srcEnvMap.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copyRegions.size(), copyRegions.data());

    transition_image_layout_cmd(uploadCmd, srcEnvMap.image.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, specularCube->numLevels);

    end_single_time_commands(vkDev, uploadCmd);


    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingMemory, nullptr);

    VkImageViewCreateInfo srcViewInfo = {
     .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
     .image = srcEnvMap.image.image,
     .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
     .format = VK_FORMAT_R32G32B32A32_SFLOAT,
     .subresourceRange = {
         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
         .baseMipLevel = 0,
         .levelCount = specularCube->numLevels,
         .baseArrayLayer = 0,
         .layerCount = 6
     }
    };

    VK_CHECK(vkCreateImageView(vkDev.device, &srcViewInfo, nullptr, &srcEnvMap.image.imageView));
    uint32_t srcBindlessIdx = get_cubemap_index(gltf, srcEnvMap);

    printf("\nPrefiltering GGX...\n");
    prefilter_cubemap(&gltf, specularCube, outputGGX, srcEnvMap, srcBindlessIdx, Distribution_GGX, 1024);

    printf("\nPrefiltering Charlie...\n");
    prefilter_cubemap(&gltf, specularCube, outputCharlie, srcEnvMap, srcBindlessIdx, Distribution_Charlie, 1024);


    ktxTextureCreateInfo irradianceInfo = {
        .glInternalformat = GL_RGBA32F,
        .vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT,
        .baseWidth = 64,
        .baseHeight = 64,
        .baseDepth = 1,
        .numDimensions = 2,
        .numLevels = 1,
        .numLayers = 1,
        .numFaces = 6,
        .generateMipmaps = KTX_FALSE
    };
    ktxTexture1* irradianceCube = nullptr;
    ktxTexture1_Create(&irradianceInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &irradianceCube);

    printf("\nPrefiltering Lambertian (Irradiance)...\n");
    prefilter_cubemap(&gltf, irradianceCube, outputIrradiance, srcEnvMap, srcBindlessIdx, Distribution_Lambertian, 2048);

    vkDestroyImageView(vkDev.device, srcEnvMap.image.imageView, nullptr);
    vkDestroyImage(vkDev.device, srcEnvMap.image.image, nullptr);
    vkFreeMemory(vkDev.device, srcEnvMap.image.imageMemory, nullptr);
    ktxTexture_Destroy(ktxTexture(specularCube));
    ktxTexture_Destroy(ktxTexture(irradianceCube));

    printf("env map prefiltered done");
}


void drawGizmo(GLTFContext& gltf)
{
    if (gltf.selectedNodeIdx < 0 || gltf.selectedNodeIdx >= (int32_t)gltf.nodesStorage.size())
        return;

    GLTFNode& node = gltf.nodesStorage[gltf.selectedNodeIdx];

    if (node.modelMtxId >= gltf.matrices.size()) {
        gltf.selectedNodeIdx = -1;
        return;
    }

    ImGuizmo::BeginFrame();

    // Get the world matrix - but also apply the model transform from renderGLTF
    glm::mat4 worldMatrix = gltf.frameData.model * gltf.matrices[node.modelMtxId];

    ImGuizmo::SetOrthographic(false);

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    static ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE currentMode = ImGuizmo::WORLD;

    ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Transform", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::RadioButton("Translate", currentOp == ImGuizmo::TRANSLATE))
            currentOp = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", currentOp == ImGuizmo::ROTATE))
            currentOp = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", currentOp == ImGuizmo::SCALE))
            currentOp = ImGuizmo::SCALE;

        ImGui::Separator();
        ImGui::Text("Node: %s", node.name.c_str());

        if (ImGui::Button("Deselect")) {
            gltf.selectedNodeIdx = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Transform")) {
            gltf.matrices[node.modelMtxId] = glm::mat4(1.0f);
            upload_buffer_data(
                gltf.app->vkDev,
                gltf.matricesBuffer.memory,
                node.modelMtxId * sizeof(glm::mat4),
                &gltf.matrices[node.modelMtxId],
                sizeof(glm::mat4)
            );
        }
    }
    ImGui::End();

    if (ImGuizmo::Manipulate(
        glm::value_ptr(gltf.frameData.view),
        glm::value_ptr(gltf.frameData.proj),
        currentOp,
        currentMode,
        glm::value_ptr(worldMatrix),
        nullptr))
    {
        // Validate
        bool valid = true;
        for (int i = 0; i < 4 && valid; i++) {
            for (int j = 0; j < 4 && valid; j++) {
                if (std::isnan(worldMatrix[i][j]) || std::isinf(worldMatrix[i][j])) {
                    valid = false;
                }
            }
        }

        if (valid) {
            // Remove the model transform to get back to local space
            glm::mat4 localMatrix = glm::inverse(gltf.frameData.model) * worldMatrix;

            gltf.matrices[node.modelMtxId] = localMatrix;

            upload_buffer_data(
                gltf.app->vkDev,
                gltf.matricesBuffer.memory,
                node.modelMtxId * sizeof(glm::mat4),
                &localMatrix,
                sizeof(glm::mat4)
            );
        }
    }
}


void drawSelector(GLTFContext* gltf)
{
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse)) {

        
        ImGui::TextDisabled("Nodes: %zu", gltf->nodesStorage.size());
        ImGui::Separator();

        
        if (ImGui::BeginChild("NodesList", ImVec2(0, 0), true)) {
            for (uint32_t i = 0; i < gltf->nodesStorage.size(); i++) {
                const auto& node = gltf->nodesStorage[i];
                bool isSelected = (gltf->selectedNodeIdx == (int32_t)i);

                std::string label = node.name.empty() ?
                    "Node " + std::to_string(i) : node.name;

                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    gltf->selectedNodeIdx = i;
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}