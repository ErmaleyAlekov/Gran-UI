#include "gran/render/renderer.hpp"

namespace gran::render
{
    // Ресурсы создаются по требованию, в первом кадре / The resources are created on demand, in the first frame
    Renderer::Renderer(rhi::RenderBackend& backend, text::FontLibrary& fonts, const std::string& imageDirectory)
        : _backend(backend), _text(fonts), _images(backend, imageDirectory), _sizer(_text, _images)
    {
    }

    // Текстуры возвращаются бэкенду (он освободит их после кадров, которые их читают)
    // The textures are returned to the backend (it releases them after the frames that read them)
    Renderer::~Renderer()
    {
        if (_atlasTexture.slot != NO_TEXTURE)
            _backend.releaseTexture(_atlasTexture.slot);
        if (_rampTexture.slot != NO_TEXTURE)
            _backend.releaseTexture(_rampTexture.slot);
    }

    // Текстовый движок (раскладки, кэш) / The text engine (the layouts, the cache)
    TextEngine& Renderer::textEngine() noexcept
    {
        return _text;
    }

    // Измеритель для компоновки / The measurer for layout
    layout::ContentMeasurer& Renderer::measurer() noexcept
    {
        return _sizer;
    }

    /*
    ** Сцена строится не больше двух раз: если за первую попытку атлас или рампа сменили
    ** поколение, ранние экземпляры ссылаются на старые места текстуры. Вторая попытка
    ** идёт с уже заполненным атласом и в обычной жизни поколение не меняет.
    **
    ** The scene is built at most twice: if during the first attempt the atlas or the ramp changed
    ** its generation, the early instances refer to old texture places. The second attempt
    ** runs with the already filled atlas and in normal life does not change the generation.
    */
    rhi::FrameStatus Renderer::render(const ui::Node& root, const style::ComputedStyle& window, float scale,
        const FrameMarks& marks)
    {
        SceneResources resources{_text, _atlas, _ramps, _images, marks.caret, marks.selection, marks.mnemonics};
        rhi::FrameData frame;

        for (int attempt = 0; attempt < 2; ++attempt)
        {
            std::uint32_t atlasGeneration = _atlas.generation();
            std::uint32_t rampGeneration = _ramps.generation();
            buildScene(root, window, scale, resources, _scene);
            if (atlasGeneration == _atlas.generation() && rampGeneration == _ramps.generation())
                break;
        }
        syncAtlas();
        syncRamps();
        frame.instances = _scene.instances.data();
        frame.instanceCount = static_cast<std::uint32_t>(_scene.instances.size());
        frame.clips = _scene.clips.data();
        frame.clipCount = static_cast<std::uint32_t>(_scene.clips.size());
        frame.segments = _scene.segments.data();
        frame.segmentCount = static_cast<std::uint32_t>(_scene.segments.size());
        Color clear = _scene.clearColor;
        frame.clearColor[0] = clear.r * clear.a;
        frame.clearColor[1] = clear.g * clear.a;
        frame.clearColor[2] = clear.b * clear.a;
        frame.clearColor[3] = clear.a;
        frame.rampRows = _ramps.capacity();
        frame.atlasSlot = _atlasTexture.slot;
        frame.rampSlot = _rampTexture.slot;
        _text.endFrame();
        return _backend.render(frame);
    }
}
