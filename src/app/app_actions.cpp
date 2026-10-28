#include "gran/app/internal/app_state.hpp"

namespace gran
{
    // Действие без обработчика молча пропускается: разметку можно писать раньше кода
    // An action without a handler is silently skipped: the markup can be written before the code
    void app::AppWindow::action(const std::string& name, ui::Node& node)
    {
        auto handler = shared.actions.find(name);

        if (handler != shared.actions.end() && handler->second)
            handler->second(node);
    }

    // Точка окна → координаты узла (с его преобразованиями) → байт текста; без рендерера — начало
    // A window point → the node coordinates (with its transforms) → a text byte; without a renderer — the start
    std::size_t app::AppWindow::offsetAt(const ui::Node& node, Vec2 point)
    {
        if (!renderer)
            return 0;
        return render::textOffsetAt(renderer->textEngine(), node, ui::windowToNode(node, point));
    }

    // Без рендерера каретка остаётся на месте / Without a renderer the caret stays in place
    std::size_t app::AppWindow::lineEdge(const ui::Node& node, std::size_t offset, bool end)
    {
        if (!renderer)
            return offset;
        return render::textLineEdge(renderer->textEngine(), node, offset, end);
    }

    // Шаг по строкам по раскладке рендерера / A line step by the renderer layout
    std::size_t app::AppWindow::lineMove(const ui::Node& node, std::size_t offset, int lines, float& goalX)
    {
        if (!renderer)
            return offset;
        return render::textLineMove(renderer->textEngine(), node, offset, lines, goalX);
    }

    // Высота страницы зависит только от компоновки / The page height depends only on layout
    int app::AppWindow::pageLines(const ui::Node& node)
    {
        return render::textPageLines(node);
    }
}
