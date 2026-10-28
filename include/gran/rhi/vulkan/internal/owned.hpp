#ifndef GRAN_RHI_VULKAN_OWNED_HPP
#define GRAN_RHI_VULKAN_OWNED_HPP

#include <functional>
#include <utility>

namespace gran::rhi::vk
{
    // Дескриптор с контекстным удалением; перемещение передаёт единственное владение.
    // A handle with contextual destruction; moving transfers sole ownership.
    template<class Handle>
    class Owned
    {
    public:
        Owned() = default;
        Owned(const Owned&) = delete;
        Owned& operator=(const Owned&) = delete;
        Owned(Owned&& other) noexcept : _handle(other._handle), _destroy(std::move(other._destroy))
        { other._handle = {}; }
        Owned& operator=(Owned&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                _handle = other._handle;
                _destroy = std::move(other._destroy);
                other._handle = {};
            }
            return *this;
        }
        ~Owned() { reset(); }
        Handle get() const noexcept { return _handle; }
        void reset() noexcept
        {
            if (_handle != Handle{}) _destroy(_handle);
            _handle = {};
        }
        Handle* put(std::function<void(Handle)> destroy)
        {
            reset();
            _destroy = std::move(destroy);
            return &_handle;
        }
    private:
        Handle _handle{};
        std::function<void(Handle)> _destroy;
    };
}
#endif
