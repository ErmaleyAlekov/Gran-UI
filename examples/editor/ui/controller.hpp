#ifndef GRAN_EDITOR_CONTROLLER_HPP
#define GRAN_EDITOR_CONTROLLER_HPP
#include "gran/app/application.hpp"
#include "../core/document.hpp"
#include <deque>
#include <vector>
namespace editor
{
    // Контроллер владеет состоянием, движок — узлами представления.
    // The controller owns state; the engine owns view nodes.
    class Controller
    {
    public:
        explicit Controller(gran::Application& application);
        void start(const std::string& path);
        void bindActions();
        void enqueue(const std::string& action, gran::ui::Node& node);
        void execute(const std::string& action, unsigned id);
        void documentAction(const std::string& action, unsigned id);
        void viewAction(const std::string& action);
        void newDocument();
        void openDocument(const std::string& path);
        bool saveDocument(unsigned id, bool saveAs);
        bool canSaveTo(const Document& item, const std::string& target);
        std::string savePath(const Document& item, bool saveAs);
        void openDialog();
        void openFolder();
        void loadFolder(const std::string& path);
        void addExplorerEntry(const std::string& path);
        void createDocument(const std::string& path, const std::string& text);
        void buildDocument(Document& document, const std::string& text);
        void select(unsigned id);
        void cycle(int direction);
        void changed(gran::ui::Node& node);
        void pointer(gran::ui::Node& node, gran::UiEvent& event);
        void requestClose(unsigned id);
        void closeAll(bool window);
        void continueClose();
        void answerClose(const std::string& action);
        void removeDocument(unsigned id);
        void reopen();
        void refresh();
        void refreshDocument(Document& document);
        void updateCursor();
        void showPanel(const std::string& title, const std::string& body);
        void toggle(const std::string& id);
        void findText();
        void findNext();
        void dropFiles(gran::ui::Node&, const std::vector<std::string>& paths);
        Document* document(unsigned id);
        unsigned nodeDocument(const gran::ui::Node& node) const;
        void setText(const std::string& id, const std::string& text);
        void show(const std::string& id, bool visible);
        gran::ui::Node& add(gran::ui::Node& parent, gran::ui::ElementKind kind,
            const std::string& style, const std::string& text);
    private:
        gran::Application& app;
        std::vector<Document> documents;
        std::deque<Document> closed;
        std::deque<unsigned> closeQueue;
        unsigned active = 0;
        unsigned serial = 0;
        unsigned pending = 0;
        bool closingWindow = false;
        std::string workspace;
    };
}
#endif
