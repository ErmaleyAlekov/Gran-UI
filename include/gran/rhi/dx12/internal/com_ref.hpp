#ifndef GRAN_RHI_DX12_INTERNAL_COM_REF_HPP
#define GRAN_RHI_DX12_INTERNAL_COM_REF_HPP

#include <utility>

namespace gran::rhi::dx12
{
    /*
    ** Владеющая ссылка на объект COM (D3D12, DXGI). Только перемещение: копия означала бы
    ** второй Release и разрушение объекта, который ещё читает GPU. Деструктор и reset()
    ** вызывают Release ровно один раз и обнуляют указатель (повторный reset безопасен).
    ** Шаблон, поэтому код в заголовке (правило 3 это допускает).
    **
    ** An owning reference to a COM object (D3D12, DXGI). Move only: a copy would mean
    ** a second Release and destroying an object the GPU still reads. The destructor and reset()
    ** call Release exactly once and null the pointer (a repeated reset is safe).
    ** A template, so the code is in the header (rule 3 allows it).
    */
    template <typename T>
    class ComRef
    {
    public:
        ComRef() noexcept = default;

        // Release при разрушении / Release on destruction
        ~ComRef()
        {
            reset();
        }

        ComRef(const ComRef&) = delete;
        ComRef& operator=(const ComRef&) = delete;

        // Перемещение передаёт владение, источник становится пустым / A move passes the ownership, the source becomes empty
        ComRef(ComRef&& other) noexcept
            : _pointer(std::exchange(other._pointer, nullptr))
        {
        }

        // Присваивание перемещением освобождает прежний объект / A move assignment releases the former object
        ComRef& operator=(ComRef&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                _pointer = std::exchange(other._pointer, nullptr);
            }
            return *this;
        }

        // Невладеющий доступ / Non-owning access
        T* get() const noexcept
        {
            return _pointer;
        }

        // Доступ к методам объекта / Access to the object methods
        T* operator->() const noexcept
        {
            return _pointer;
        }

        // Адрес для выходного параметра Create*: прежний объект освобождается заранее
        // The address for a Create* output parameter: the former object is released beforehand
        T** put() noexcept
        {
            reset();
            return &_pointer;
        }

        // Освобождение (идемпотентно) / Releasing (idempotent)
        void reset() noexcept
        {
            if (_pointer != nullptr)
            {
                _pointer->Release();
                _pointer = nullptr;
            }
        }

        // Есть ли объект / Whether there is an object
        explicit operator bool() const noexcept
        {
            return _pointer != nullptr;
        }

    private:
        T* _pointer = nullptr;
    };
}

#endif
