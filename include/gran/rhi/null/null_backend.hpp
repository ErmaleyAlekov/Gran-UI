#ifndef GRAN_RHI_NULL_BACKEND_HPP
#define GRAN_RHI_NULL_BACKEND_HPP

#include "gran/rhi/render_backend.hpp"

#include <cstdint>
#include <vector>

namespace gran::rhi
{
    // Что пустой бэкенд принял: тест видит, что кадры шли и сцена не пуста / What the empty backend accepted: a test sees frames went and the scene is not empty
    struct NullStats
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t frames = 0;
        std::uint32_t lastInstances = 0;
        std::uint32_t lastClips = 0;
        std::uint32_t liveTextures = 0;
    };

    /*
    ** Пустой бэкенд (паттерн Null Object): принимает все вызовы RenderBackend и ничего не рисует. Нужен там,
    ** где GPU нет (сборочные серверы, раннер macOS без Metal), чтобы приложение работало целиком — компоновка,
    ** сцена, ввод, таймеры — в тестах. Проверяет то же, что проверил бы настоящий бэкенд со стороны CPU:
    ** номер текстуры существует, участок обновления внутри текстуры, экземпляров не больше, чем передано.
    **
    ** An empty backend (the Null Object pattern): it accepts all RenderBackend calls and draws nothing. It is needed where
    ** there is no GPU (build servers, a macOS runner without Metal) so the application works entirely — the layout,
    ** the scene, the input, the timers — in tests. It checks what a real backend would check on the CPU side:
    ** the texture slot exists, the update region is inside the texture, the instances are not more than passed.
    */
    class NullBackend final : public RenderBackend
    {
    public:
        // ---- null_backend.cpp ----

        void resize(std::uint32_t width, std::uint32_t height) override;
        FrameStatus render(const FrameData& frame) override;
        std::vector<std::string> takeDiagnostics() override;
        AdapterInfo adapterInfo() const override;

        // Счётчики принятого / The counters of what was accepted
        const NullStats& stats() const noexcept;

        // ---- null_textures.cpp ----

        std::uint32_t createTexture(const TextureDesc& desc) override;
        void updateTexture(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels,
            std::uint32_t rowPitch) override;
        void releaseTexture(std::uint32_t slot) override;

    private:
        // Размеры текстур по номеру; освобождённая — 0×0, номер переиспользуется
        // The texture sizes by slot; a released one is 0×0, the slot is reused
        std::vector<TextureDesc> _textures;
        NullStats _stats;
    };
}

#endif
