#include "render_tests.hpp"
#include "gran/app/application.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace render_tests
{
    namespace
    {
        // Одна запись и закрытие имитируют сохранение редактором.
        // One write and close simulate an editor save.
        void write(const std::string& path, const char* text)
        {
            std::ofstream output(path);
            output << text;
        }

        // Копии функтора разделяют фазу; указатели на удалённые узлы не сохраняются.
        // Functor copies share the phase; pointers to removed nodes are never retained.
        struct Probe
        {
            gran::Application* app;
            std::string path;
            std::shared_ptr<unsigned> phase;
            bool* passed;

            // Проверяем отложенную замену и полный цикл ошибки/исправления watcher.
            // Check deferred replacement and the complete watcher error/recovery cycle.
            void operator()() const
            {
                if (*phase == 0)
                {
                    app->loadString("<Window><Input id=\"new\" value=\"new\"/></Window>");
                    *passed = *passed && app->find("old") != nullptr;
                }
                else if (*phase == 1)
                {
                    *passed = *passed && app->find("new") != nullptr;
                    app->focus(app->find("new"));
                    app->loadFile(path);
                }
                else if (*phase == 2) write(path, "<Window>");
                else if (*phase == 3)
                {
                    if (app->reloadError().empty()) return;
                    *passed = *passed && app->find("old") != nullptr;
                    write(path, "<Window><Text id=\"recovered\">recovered content</Text></Window>");
                }
                else if (*phase == 4 && app->find("recovered") == nullptr) return;
                else if (*phase == 4) *passed = *passed && app->reloadError().empty();
                ++*phase;
            }
        };
    }

    // Настоящее окно и GPU: загрузка из таймера, ошибка XML, автоматическое восстановление.
    // A real window and GPU: timer loading, XML failure, automatic recovery.
    bool runReload(const Options& options)
    {
        gran::AppOptions settings;
        settings.backend = options.backend;
        settings.forceWarp = options.warp;
        settings.debugLayer = true;
        settings.watchMarkup = true;
        settings.exitAfter = 1.6;
        gran::Application app(settings);
        bool passed = true;
        auto phase = std::make_shared<unsigned>(0);
        std::string path = options.output + "/reload.xml";
        write(path, "<Window><Input id=\"old\" value=\"old\"/></Window>");
        app.loadFile(path);
        app.focus(app.find("old"));
        app.setInterval(0.08, Probe{&app, path, phase, &passed});
        app.run();
        passed = passed && *phase >= 5 && app.find("recovered") != nullptr;
        std::filesystem::remove(path);
        if (passed) std::cout << "[ OK ] live reload\n";
        else std::cout << "[FAIL] live reload phase=" << *phase << '\n';
        return passed;
    }
}
