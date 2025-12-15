#include <volk.h>
#include <glfw/glfw3.h>
#include <memory>
#include<iostream>
#include <utils_cubemap.h>
#include <string.h>
#include <string>
#include <malloc.h>
#include <cassert>
#include "Utils.h"

#include <stb_image.h>
#include <ktx.h>

#include <ktxvulkan.h>
    

#include <unordered_map>

std::unordered_map<uint32_t, std::string> debugGLSLSourceCode;

bool ends_with(const char* s, const char* part)
{
	const size_t sLength = strlen(s);
	const size_t partLength = strlen(part);
	return sLength < partLength ? false : strcmp(s + sLength - partLength, part) == 0;
}

std::string read_shader_file(const char* fileName)
{
    FILE* file = fopen(fileName, "r");

    if (!file) {
        printf("I/O error. Cannot open shader file '%s'\n", fileName);
        return std::string();
    }

    fseek(file, 0L, SEEK_END);
    const size_t bytesinfile = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char* buffer = (char*)alloca(bytesinfile + 1);
    const size_t bytesread = fread(buffer, 1, bytesinfile, file);
    fclose(file);

    buffer[bytesread] = 0;

    static constexpr unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };

    if (bytesread > 3) {
        if (!memcmp(buffer, BOM, 3))
            memset(buffer, ' ', 3);
    }

    std::string code(buffer);

    while (code.find("#include ") != code.npos) {
        const auto pos = code.find("#include ");
        const auto p1 = code.find('<', pos);
        const auto p2 = code.find('>', pos);
        if (p1 == code.npos || p2 == code.npos || p2 <= p1) {
            printf("Error while loading shader program: %s\n", code.c_str());
            return std::string();
        }
        const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
        const std::string include = read_shader_file(name.c_str());
        code.replace(pos, p2 - pos + 1, include.c_str());
    }

    return code;
}


VkShaderStageFlagBits shader_stage_from_file_name(const char* fileName)
{
    if (ends_with(fileName, ".vert"))
        return VK_SHADER_STAGE_VERTEX_BIT;

    if (ends_with(fileName, ".frag"))
        return VK_SHADER_STAGE_FRAGMENT_BIT;

    if (ends_with(fileName, ".geom"))
        return VK_SHADER_STAGE_GEOMETRY_BIT;

    if (ends_with(fileName, ".comp"))
        return VK_SHADER_STAGE_COMPUTE_BIT;

    if (ends_with(fileName, ".tesc"))
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

    if (ends_with(fileName, ".tese"))
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    return VK_SHADER_STAGE_VERTEX_BIT;
}




VulkanTexture load_texture(VulkanRenderDevice& vkDev, const char* fileName, VkImageViewType viewType, bool sRGB)
{
    const bool isKTX = ends_with(fileName, ".ktx") || ends_with(fileName, ".KTX");
    VulkanTexture texture = {};

    if (isKTX) {
        ktxTexture1* ktxTex = nullptr;
        if (ktxTexture1_CreateFromNamedFile(fileName, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex) != KTX_SUCCESS) {
            printf("Failed to load KTX: %s\n", fileName);
            return {};
        }
        SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

        texture.width = ktxTex->baseWidth;
        texture.height = ktxTex->baseHeight;
        texture.depth = 1;

 
        switch (ktxTex->glInternalformat) {
        case 0x8E8F: texture.format = VK_FORMAT_BC6H_UFLOAT_BLOCK; break;       // GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
        case 0x8E8C: texture.format = VK_FORMAT_BC7_UNORM_BLOCK; break;         // GL_COMPRESSED_RGBA_BPTC_UNORM
        case 0x881A: texture.format = VK_FORMAT_R16G16B16A16_SFLOAT; break;     // GL_RGBA16F
        case 0x8058: texture.format = VK_FORMAT_R8G8B8A8_UNORM; break;          // GL_RGBA8
        default:
            texture.format = ktxTexture1_GetVkFormat(ktxTex);
            if (texture.format == VK_FORMAT_UNDEFINED) {
                printf("WARNING: Unknown KTX Format 0x%X. Using RGBA16F.\n", ktxTex->glInternalformat);
                texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            }
            break;
        }

        const uint32_t layerCount = ktxTex->isCubemap ? 6 : 1;
        const VkImageCreateFlags flags = ktxTex->isCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        const VkImageViewType ktxViewType = ktxTex->isCubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

        
        create_image(vkDev.device, vkDev.physicalDevice, texture.width, texture.height, texture.format,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image.image, texture.image.imageMemory, flags, ktxTex->numLevels);

        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        create_buffer(vkDev.device, vkDev.physicalDevice, ktxTex->dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingMemory);
        upload_buffer_data(vkDev, stagingMemory, 0, ktxTex->pData, ktxTex->dataSize);

        
        std::vector<VkBufferImageCopy> regions;
        for (uint32_t level = 0; level < ktxTex->numLevels; ++level) {
            for (uint32_t layer = 0; layer < layerCount; ++layer) {
                ktx_size_t offset;
                ktxTexture_GetImageOffset(ktxTexture(ktxTex), level, 0, layer, &offset);

                VkBufferImageCopy region = {};
                region.bufferOffset = offset;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = level;
                region.imageSubresource.baseArrayLayer = layer;
                region.imageSubresource.layerCount = 1;
                region.imageExtent.width = std::max(1u, ktxTex->baseWidth >> level);
                region.imageExtent.height = std::max(1u, ktxTex->baseHeight >> level);
                region.imageExtent.depth = 1;
                regions.push_back(region);
            }
        }

        
        VkCommandBuffer cmd = begin_single_time_commands(vkDev);
        transition_image_layout_cmd(cmd, texture.image.image, texture.format,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            layerCount, ktxTex->numLevels);
        vkCmdCopyBufferToImage(cmd, stagingBuffer, texture.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)regions.size(), regions.data());
        transition_image_layout_cmd(cmd, texture.image.image, texture.format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            layerCount, ktxTex->numLevels);
        end_single_time_commands(vkDev, cmd);

        vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
        vkFreeMemory(vkDev.device, stagingMemory, nullptr);

        create_image_view(vkDev.device, texture.image.image, texture.format,
            VK_IMAGE_ASPECT_COLOR_BIT, &texture.image.imageView, ktxViewType, layerCount, ktxTex->numLevels);
    }
    else {
        // for HDR
        int w, h;
        const float* img = stbi_loadf(fileName, &w, &h, nullptr, 4);
        if (!img) {
            printf("Failed to load HDR: %s\n", fileName);
            return {};
        }
        SCOPE_EXIT{ stbi_image_free((void*)img); };

        Bitmap in;
        init_bitmap_from_data(&in, w, h, 4, eBitmapFormat_Float, img);

        Bitmap verticalCross = convert_equirectangular_map_to_vertial_cross(in);
        Bitmap cubemap = convert_vertical_cross_to_cubemap_faces(verticalCross);

        
        texture.width = cubemap.w;   
        texture.height = cubemap.h;  
        texture.depth = 6;           
        texture.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,  
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .extent = { (uint32_t)cubemap.w, (uint32_t)cubemap.h, 1 },  
            .mipLevels = 1,
            .arrayLayers = 6,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        VK_CHECK(vkCreateImage(vkDev.device, &imageInfo, nullptr, &texture.image.image));

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(vkDev.device, texture.image.image, &memReqs);

        VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs.size,
            .memoryTypeIndex = find_memory_type(vkDev.physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_CHECK(vkAllocateMemory(vkDev.device, &allocInfo, nullptr, &texture.image.imageMemory));
        VK_CHECK(vkBindImageMemory(vkDev.device, texture.image.image, texture.image.imageMemory, 0));

        VkDeviceSize imageSize = cubemap.data.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        create_buffer(vkDev.device, vkDev.physicalDevice, imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingMemory);

        upload_buffer_data(vkDev, stagingMemory, 0, cubemap.data.data(), imageSize);

        VkCommandBuffer cmdBuf = begin_single_time_commands(vkDev);

        transition_image_layout_cmd(cmdBuf, texture.image.image, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, 1);

        std::vector<VkBufferImageCopy> regions;
        VkDeviceSize faceSize = imageSize / 6;
        for (uint32_t face = 0; face < 6; face++) {
            VkBufferImageCopy region = {
                .bufferOffset = face * faceSize,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = face,
                    .layerCount = 1
                },
                .imageOffset = {0, 0, 0},
                .imageExtent = {(uint32_t)cubemap.w, (uint32_t)cubemap.h, 1}  
            };
            regions.push_back(region);
        }

        vkCmdCopyBufferToImage(cmdBuf, stagingBuffer, texture.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions.data());

        transition_image_layout_cmd(cmdBuf, texture.image.image, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, 1);

        end_single_time_commands(vkDev, cmdBuf);

        vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
        vkFreeMemory(vkDev.device, stagingMemory, nullptr);

        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture.image.image,
            .viewType = VK_IMAGE_VIEW_TYPE_CUBE,  
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 6
            }
        };

        VK_CHECK(vkCreateImageView(vkDev.device, &viewInfo, nullptr, &texture.image.imageView));
    }

    create_texture_sampler(vkDev.device, &texture.sampler);
    texture.desiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    setVkImageName(vkDev, texture.image.image, fileName);


    return texture;
}


void save_string_list(FILE* f, const std::vector<std::string>& lines)
{
    uint32_t sz = (uint32_t)lines.size();
    fwrite(&sz, sizeof(uint32_t), 1, f);
    for (const std::string& s : lines) {
        sz = (uint32_t)s.length();
        fwrite(&sz, sizeof(uint32_t), 1, f);
        fwrite(s.c_str(), sz + 1, 1, f);
    }
}

void load_string_list(FILE* f, std::vector<std::string>& lines)
{
    {
        uint32_t sz = 0;
        fread(&sz, sizeof(uint32_t), 1, f);
        lines.resize(sz);
    }
    std::vector<char> inBytes;
    for (std::string& s : lines) {
        uint32_t sz = 0;
        fread(&sz, sizeof(uint32_t), 1, f);
        inBytes.resize(sz + 1);
        fread(inBytes.data(), sz + 1, 1, f);
        s = std::string(inBytes.data());
    }
}

int add_unique(std::vector<std::string>& files, const std::string& file)
{
    if (file.empty())
        return -1;

    const auto i = std::find(std::begin(files), std::end(files), file);

    if (i != files.end())
        return (int)std::distance(files.begin(), i);

    files.push_back(file);
    return (int)files.size() - 1;
}

std::string replace_all(const std::string& str, const std::string& oldSubStr, const std::string& newSubStr)
{
    std::string result = str;

    for (size_t p = result.find(oldSubStr); p != std::string::npos; p = result.find(oldSubStr))
        result.replace(p, oldSubStr.length(), newSubStr);

    return result;
}

// Convert 8-bit ASCII string to upper case
std::string lowercase_string(const std::string& s)
{
    std::string out(s.length(), ' ');
    std::transform(s.begin(), s.end(), out.begin(), tolower);
    return out;
}