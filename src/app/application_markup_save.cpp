#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/markup/components.hpp"
#include "gran/markup/live_document.hpp"

#include <filesystem>
#include <fstream>

namespace gran
{
    namespace
    {
        // Окно узла или главное; до загрузки разметки сохранять нечего / The node window or the main one; nothing to save before the markup is loaded
        const app::AppWindow& markupWindow(Application::State& state, const ui::Node* window)
        {
            if (window != nullptr)
                return app::windowOf(state, *window);
            if (state.windows.empty())
                throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::APP_NO_MARKUP));
            return *state.windows.front();
        }
    }

    // Текст разметки целиком, без записи на диск / The whole markup text, without writing to disk
    std::string Application::saveMarkup(const ui::Node* window) const
    {
        return markup::serializeMarkup(markupWindow(*_state, window).document);
    }

    // Путь — UTF-8 (u8path), как у loadFile; файл пишется двоично: переводы строк — как в тексте
    // The path is UTF-8 (u8path), as for loadFile; the file is written in binary: the line breaks are as in the text
    void Application::saveMarkupFile(const std::string& path, const ui::Node* window) const
    {
        std::string text = saveMarkup(window);
        std::ofstream file(std::filesystem::u8path(path), std::ios::binary | std::ios::trunc);

        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        file.close();
        if (!file)
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_WRITE_FAILED, {path}));
    }

    // Схемы строятся при загрузке документа: здесь только поиск по пространству имён / The schemas are built when the document is loaded: here is only a lookup by the namespace
    std::string Application::componentSchema(const std::string& namespaceUri, const ui::Node* window) const
    {
        for (const markup::ComponentSchema& schema : markup::componentSchemas(markupWindow(*_state, window).document))
        {
            if (schema.namespaceUri == namespaceUri)
                return schema.text;
        }
        return std::string();
    }
}
