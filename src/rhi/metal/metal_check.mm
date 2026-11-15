#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

namespace gran::rhi::metal
{
    // Провал вызова Metal → GpuException; текст NSError (если есть) идёт в сообщение как «код ошибки»
    // A failed Metal call → GpuException; the NSError text (if any) goes into the message as the "error code"
    void check(bool ok, NSError* error, const char* call, const char* file, int line)
    {
        std::string detail = "nil";
        long long code = 0;

        if (ok && error == nil)
            return;
        if (error != nil)
        {
            detail = error.localizedDescription.UTF8String;
            code = static_cast<long long>(error.code);
        }
        throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_CALL_FAILED,
            {call, detail, file, std::to_string(line)}), code);
    }

    // Неверный аргумент вызывающего кода — не сбой GPU / An invalid caller argument is not a GPU failure
    void invalid(const char* resource)
    {
        throw GpuException(ErrorCode::INVALID_ARGUMENT,
            i18n::format(i18n::Message::GPU_BAD_TEXTURE, {resource, "Metal"}), 0);
    }

    // Текстура в частной памяти GPU: данные приходят только копией из промежуточного буфера (metal_textures.mm).
    // Цель отрисовки ещё и читается шейдером (поверхность — источник размытия, уровни размытия — подложка).
    // A texture in private GPU memory: data arrives only as a copy from a staging buffer (metal_textures.mm).
    // A render target is also read by a shader (the surface is the blur source, the blur levels are the backdrop).
    void makeTexture(id<MTLDevice> device, Texture& texture, const TextureDesc& desc, bool target)
    {
        MTLPixelFormat format = MTLPixelFormatRGBA8Unorm;

        if (desc.width == 0 || desc.height == 0 || desc.width > 16384 || desc.height > 16384)
            invalid("size");
        if (desc.format == TextureFormat::R8)
            format = MTLPixelFormatR8Unorm;
        MTLTextureDescriptor* info = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
            width:desc.width height:desc.height mipmapped:NO];
        info.storageMode = MTLStorageModePrivate;
        info.usage = MTLTextureUsageShaderRead;
        if (target)
            info.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        texture.resource = [device newTextureWithDescriptor:info];
        GRAN_METAL_CHECK(texture.resource != nil, nil, "newTextureWithDescriptor");
        if (desc.debugName != nullptr)
            texture.resource.label = [NSString stringWithUTF8String:desc.debugName];
        texture.width = desc.width;
        texture.height = desc.height;
        texture.bytes = 4;
        if (desc.format == TextureFormat::R8)
            texture.bytes = 1;
    }

    // Чужой поток Metal: запись итога под мьютексом, затем освобождение кадра. Потеря устройства (снятая
    // видеокарта, отозванный доступ) — флаг lost, прочие ошибки — сообщения для takeDiagnostics.
    // A foreign Metal thread: the result is written under the mutex, then the frame is released. A device loss
    // (a removed video card, revoked access) is the lost flag, the other errors are messages for takeDiagnostics.
    void completed(SharedState& state, id<MTLCommandBuffer> buffer) noexcept
    {
        try
        {
            NSError* error = buffer.error;
            if (buffer.status == MTLCommandBufferStatusError && error != nil)
            {
                std::lock_guard<std::mutex> lock(state.mutex);
                std::string text = error.localizedDescription.UTF8String;
                if (error.code == MTLCommandBufferErrorDeviceRemoved || error.code == MTLCommandBufferErrorAccessRevoked)
                {
                    state.lostReason = text;
                    state.lost = true;
                }
                state.messages.push_back(text);
            }
        }
        catch (...)
        {
            // Нехватка памяти при записи текста не должна уйти в поток Metal: сообщение теряется, кадр отпускается
            // Running out of memory while writing the text must not reach the Metal thread: the message is lost, the frame is released
        }
        dispatch_semaphore_signal(state.inFlight);
    }
}
