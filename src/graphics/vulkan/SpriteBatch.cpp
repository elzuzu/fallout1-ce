#include "SpriteBatch.hpp"
#include "MemoryAllocator.hpp"
#include <cstring>
#include <vk_mem_alloc.h>

SpriteBatch::SpriteBatch() = default;
SpriteBatch::~SpriteBatch()
{
    destroy();
}

bool SpriteBatch::init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamily)
{
    m_device = device;
    // Create static quad vertex buffer
    struct Vert { glm::vec2 pos; glm::vec2 uv; } verts[4] = {
        { {0.f, 0.f}, {0.f, 0.f} },
        { {1.f, 0.f}, {1.f, 0.f} },
        { {1.f, 1.f}, {1.f, 1.f} },
        { {0.f, 1.f}, {0.f, 1.f} },
    };

    VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufInfo.size = sizeof(verts);
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    m_vertexAlloc = MemoryAllocator::createVertexBuffer(sizeof(verts), m_vertexBuffer);

    bufInfo.size = 1024 * sizeof(Sprite);
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    m_instanceAlloc = MemoryAllocator::createBuffer(bufInfo, m_instanceBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // upload vertex data
    void* data{};
    vmaMapMemory(MemoryAllocator::handle(), m_vertexAlloc, &data);
    memcpy(data, verts, sizeof(verts));
    vmaUnmapMemory(MemoryAllocator::handle(), m_vertexAlloc);

    return true;
}

void SpriteBatch::destroy()
{
    if (m_vertexBuffer)
        vmaDestroyBuffer(MemoryAllocator::handle(), m_vertexBuffer, m_vertexAlloc);
    if (m_instanceBuffer)
        vmaDestroyBuffer(MemoryAllocator::handle(), m_instanceBuffer, m_instanceAlloc);
    m_vertexBuffer = VK_NULL_HANDLE;
    m_instanceBuffer = VK_NULL_HANDLE;
    m_vertexAlloc = nullptr;
    m_instanceAlloc = nullptr;
    m_sprites.clear();
}

void SpriteBatch::draw(const Sprite& s)
{
    m_sprites.push_back(s);
}

void SpriteBatch::flush(VkCommandBuffer cb)
{
    if (m_sprites.empty())
        return;

    if (cb == VK_NULL_HANDLE) {
        m_sprites.clear();
        return;
    }

    // Copy instance data to GPU
    void* data{};
    vmaMapMemory(MemoryAllocator::handle(), m_instanceAlloc, &data);
    memcpy(data, m_sprites.data(), m_sprites.size() * sizeof(Sprite));
    vmaUnmapMemory(MemoryAllocator::handle(), m_instanceAlloc);

    VkDeviceSize offsets[] = {0};
    VkBuffer bufs[] = {m_vertexBuffer};
    vkCmdBindVertexBuffers(cb, 0, 1, bufs, offsets);
    VkBuffer instBufs[] = {m_instanceBuffer};
    vkCmdBindVertexBuffers(cb, 1, 1, instBufs, offsets);
    vkCmdDraw(cb, 4, static_cast<uint32_t>(m_sprites.size()), 0, 0);

    m_sprites.clear();
}
