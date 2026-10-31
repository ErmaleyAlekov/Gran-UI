#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include <filesystem>
#include <functional>
namespace editor
{
    // Контроллер живёт дольше цикла событий. / The controller outlives the event loop.
    Controller::Controller(gran::Application& application) : app(application) {}

    // Начальная папка реальна; без аргумента открывается новый документ.
    // The initial folder is real; without an argument a new document opens.
    void Controller::start(const std::string& path)
    {
        bindActions();
        auto folder = std::filesystem::current_path();
        if (!path.empty()) folder = std::filesystem::absolute(std::filesystem::u8path(path)).parent_path();
        loadFolder(folder.u8string());
        if (path.empty())
            newDocument();
        else
            openDocument(path);
        refresh();
        app.setInterval(0.15, std::bind(&Controller::updateCursor, this));
    }

    // Команды обрабатываются после завершения текущей доставки событий.
    // Commands execute after the current event dispatch completes.
    void Controller::enqueue(const std::string& action, gran::ui::Node& node)
    {
        if (action == "openEntry")
            app.post(std::bind(&Controller::openDocument, this, node.content.option));
        else if (action == "browseFolder")
            app.post(std::bind(&Controller::loadFolder, this, node.content.option));
        else
            app.post(std::bind(&Controller::execute, this, action, nodeDocument(node)));
    }

    // Диалог подтверждения блокирует конкурирующие операции с документами.
    // The confirmation dialog blocks competing document operations.
    void Controller::execute(const std::string& action, unsigned id)
    {
        try
        {
            if (action == "confirmSave" || action == "discardClose" || action == "cancelClose")
            {
                answerClose(action);
                return;
            }
            if (pending) return;
            documentAction(action, id);
            viewAction(action);
        }
        catch (const std::exception& error)
        {
            if (pending) answerClose("cancelClose");
            setText("statusMessage", error.what());
        }
    }

    // Файлы, брошенные на окно, открываются по очереди. / Dropped files open in order.
    void Controller::dropFiles(gran::ui::Node&, const std::vector<std::string>& paths)
    {
        if (pending)
            return;
        for (const auto& path : paths)
            app.post(std::bind(&Controller::openDocument, this, path));
    }
}
