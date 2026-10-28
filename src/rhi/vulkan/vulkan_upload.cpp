#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    /*
    ** Загрузки кадра: новые текстуры очищаются, ждущие области одним memcpy уходят в буфер загрузки
    ** кадра (его fence уже пройден в beginFrame) и копируются в изображения только своими прямоугольниками.
    ** Затем все затронутые изображения переходят в SHADER_READ_ONLY.
    **
    ** The frame uploads: new textures are cleared, the pending regions go into the frame upload buffer with one memcpy
    ** (its fence was already passed in beginFrame) and are copied into the images by their own rectangles only.
    ** Then all the touched images move to SHADER_READ_ONLY.
    */
    void VulkanBackend::uploadTextures()
    {
        Frame& frame = _frames[_current];

        for (Image& image : _textures)
            if (image.handle.get() != VK_NULL_HANDLE && !image.cleared) clearTexture(image);
        if (!_uploads.empty())
        {
            ensureBuffer(frame.upload, _uploadBytes.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            writeBuffer(frame.upload, _uploadBytes.data(), _uploadBytes.size());
            for (const PendingUpload& upload : _uploads) copyRegion(upload);
            _uploads.clear();
            _uploadBytes.clear();
        }
        for (Image& image : _textures)
            if (image.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) barrier(image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // Барьер перед каждой копией: области одной текстуры могут перекрываться (запись после записи).
    // A barrier before every copy: the regions of one texture may overlap (write after write).
    void VulkanBackend::copyRegion(const PendingUpload& upload)
    {
        Image& image = _textures[upload.slot];
        VkBufferImageCopy copy{};

        barrier(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy.bufferOffset = upload.offset;
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageOffset = {static_cast<int>(upload.region.x), static_cast<int>(upload.region.y), 0};
        copy.imageExtent = {upload.region.width, upload.region.height, 1};
        vkCmdCopyBufferToImage(_frames[_current].commands, _frames[_current].upload.handle.get(), image.handle.get(),
            image.layout, 1, &copy);
    }
}
