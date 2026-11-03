#include "gran/res/internal/registry.hpp"

namespace gran::res
{
    namespace internal
    {
        // Реестр живёт до конца процесса: на его байты ссылаются шрифты и потоки / The registry lives until the process end: the fonts and the streams refer to its bytes
        Registry& registry() noexcept
        {
            static Registry state;

            return state;
        }
    }

    // Пути пакета уже нормализованы генератором; нормализация повторяется для пакетов, собранных вручную / The bundle paths are already normalized by the generator; the normalization repeats for hand-made bundles
    void mount(const Bundle& bundle)
    {
        internal::Registry& state = internal::registry();
        std::lock_guard<std::mutex> lock(state.mutex);

        for (std::size_t index = 0; index < bundle.count; ++index)
        {
            const Entry& entry = bundle.entries[index];
            std::string path = normalize(entry.path);
            if (!path.empty())
                state.entries[path] = ResourceView{entry.data, entry.size};
        }
    }

    // Блоки освобождаются вместе с видами на них / The blocks are freed together with the views on them
    void unmountAll() noexcept
    {
        internal::Registry& state = internal::registry();
        std::lock_guard<std::mutex> lock(state.mutex);

        state.entries.clear();
        state.blobs.clear();
    }

    // «res:» и косая любого вида / "res:" and a slash of any kind
    bool isResourcePath(std::string_view path) noexcept
    {
        return path.size() > 4 && path.compare(0, 4, "res:") == 0 && (path[4] == '/' || path[4] == '\\');
    }

    // Не ресурсный путь — сразу false (обычный файл) / A non-resource path — false at once (an ordinary file)
    bool find(std::string_view path, ResourceView& view)
    {
        internal::Registry& state = internal::registry();

        if (!isResourcePath(path))
            return false;
        std::string key = normalize(path);
        std::lock_guard<std::mutex> lock(state.mutex);
        auto found = state.entries.find(key);
        if (key.empty() || found == state.entries.end())
            return false;
        view = found->second;
        return true;
    }
}
