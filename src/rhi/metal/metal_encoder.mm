#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

namespace gran::rhi::metal
{
    EncoderScope::EncoderScope(id<MTLCommandEncoder> encoder) : _encoder(encoder)
    {
    }

    // При раскрутке стека кодировщик закрывается до того, как командный буфер будет брошен
    // During stack unwinding the encoder is closed before the command buffer is abandoned
    EncoderScope::~EncoderScope()
    {
        end();
    }

    void EncoderScope::adopt(id<MTLCommandEncoder> encoder)
    {
        end();
        _encoder = encoder;
    }

    // endEncoding — сообщение Objective-C, исключений C++ не бросает / endEncoding is an Objective-C message, it throws no C++ exceptions
    void EncoderScope::end()
    {
        if (_encoder == nil)
            return;
        [_encoder endEncoding];
        _encoder = nil;
    }

    // UTF8String возвращает NULL, если строку нельзя представить в UTF-8; std::string из NULL — неопределённое поведение
    // UTF8String returns NULL if the string cannot be represented in UTF-8; std::string from NULL is undefined behavior
    std::string utf8(NSString* text) noexcept
    {
        const char* bytes = nullptr;

        if (text != nil)
            bytes = text.UTF8String;
        if (bytes == nullptr)
            return std::string();
        try { return std::string(bytes); }
        catch (...) { return std::string(); }
    }
}
