#ifndef GRAN_APP_APP_DRIVER_HPP
#define GRAN_APP_APP_DRIVER_HPP

#include "gran/app/application.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/rhi/null/null_backend.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran
{
    /*
    ** «Водитель» приложения для тестов интерфейса (шаг M2 и основа С.2): вместо run() тест сам шагает по тому
    ** же циклу (app::runLoopStep без сна) и подаёт события в безоконные окна, как это сделал бы пользователь.
    ** Приложение должно быть создано с AppOptions::headless = true; без видеокарты — ещё и backend = NONE.
    ** Пользоваться — только из потока, создавшего приложение (это его поток интерфейса).
    **
    **     gran::AppOptions options;
    **     options.headless = true;
    **     options.backend = gran::rhi::BackendKind::NONE;
    **     gran::Application app(options);
    **     app.loadString("<Window>…<Button id=\"ok\" onClick=\"save\">OK</Button>…</Window>");
    **     gran::AppDriver driver(app);
    **     driver.start();
    **     driver.click(*app.find("ok"));     // обработчик save уже выполнен
    **
    ** The application driver for UI tests (step M2 and the base of S.2): instead of run() a test itself steps through the same
    ** loop (app::runLoopStep without sleeping) and feeds events into the windowless windows, as a user would.
    ** The application must be created with AppOptions::headless = true; without a graphics card — also backend = NONE.
    ** Use it only from the thread that created the application (that is its UI thread).
    **
    ** Время жизни: объявляйте AppDriver после Application. Обратный порядок безопасен (замечание Т-44): время
    ** «run()» хранит само приложение, а водитель узнаёт о его разрушении по weak_ptr и бросает
    ** GranException(INVALID_ARGUMENT) на любой вызов, кроме деструктора и centerOf.
    ** Lifetime: declare AppDriver after Application. The reverse order is safe (remark T-44): the "run()" time
    ** is kept by the application itself, and the driver learns of its destruction through a weak_ptr and throws
    ** GranException(INVALID_ARGUMENT) on any call except the destructor and centerOf.
    */
    class AppDriver
    {
    public:
        // ---- app_driver_life.cpp ----

        // Приложение без headless — GranException INVALID_ARGUMENT / An application without headless — GranException INVALID_ARGUMENT
        explicit AppDriver(Application& app);
        // Снимает время «run()», если приложение ещё живо / Removes the "run()" time if the application is still alive
        ~AppDriver();
        AppDriver(const AppDriver&) = delete;
        AppDriver& operator=(const AppDriver&) = delete;

        // ---- app_driver.cpp ----

        // Окна получают графику и первый кадр; фоновые задачи будят главное окно (как в run())
        // The windows get their graphics and the first frame; background tasks wake the main window (as in run())
        void start();

        // Один шаг цикла без сна; false — приложение завершилось (закрыто главное окно)
        // One loop step without sleeping; false — the application quit (the main window was closed)
        bool step();

        // Шаги, пока кадры рисуются (не больше limit: анимация рисует всегда); возвращает число шагов
        // Steps while frames are drawn (at most limit: an animation always draws); returns the number of steps
        unsigned settle(unsigned limit = 50);

        // ---- app_driver_windows.cpp ----

        std::size_t windowCount() const;

        // Безоконное окно по номеру (0 — главное) / The windowless window by its number (0 — the main one)
        platform::HeadlessWindow& window(std::size_t index = 0);

        // Окно, в дереве которого лежит узел / The window whose tree holds the node
        platform::HeadlessWindow& windowOf(const ui::Node& node);

        // Ответ следующему диалогу файлов (пусто — «Отмена»); ответы расходуются по порядку
        // The answer to the next file dialog (empty — "Cancel"); the answers are used in order
        void answerFileDialog(const std::vector<std::string>& paths);

        // ---- app_driver_frame.cpp ----

        // Пиксели последнего кадра окна (RGBA); false — бэкенд кадр не читает (NONE) / The last frame pixels of a window (RGBA); false — the backend does not read frames (NONE)
        bool readFrame(std::size_t index, std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height);

        // Счётчики пустого бэкенда окна; nullptr — бэкенд не пустой / The empty backend counters of a window; nullptr — the backend is not empty
        const rhi::NullStats* nullStats(std::size_t index = 0) const;

        // ---- app_driver_pointer.cpp ----

        // Середина рамки узла в координатах его окна (с преобразованиями transform) / The node frame middle in its window coordinates (with the transforms)
        Vec2 centerOf(const ui::Node& node) const;

        // Мышь над серединой узла; затем settle / The mouse over the node middle; then settle
        void hover(const ui::Node& node);

        // Нажать и отпустить кнопку мыши над серединой узла; затем settle / Press and release a mouse button over the node middle; then settle
        void click(const ui::Node& node, platform::MouseButton button = platform::MouseButton::LEFT);

        // ---- app_driver_keys.cpp ----

        // Нажать и отпустить клавишу в окне index; затем settle / Press and release a key in the window index; then settle
        void press(platform::Key key, std::uint8_t modifiers = 0, std::size_t index = 0);

        // Набрать текст UTF-8 (события CHARACTER) в окне index; затем settle / Type UTF-8 text (CHARACTER events) in the window index; then settle
        void type(const std::string& text, std::size_t index = 0);

    private:
        // ---- app_driver_life.cpp ----

        // Состояние живого приложения; разрушенное — GranException APP_DRIVER_DETACHED
        // The state of a live application; a destroyed one — GranException APP_DRIVER_DETACHED
        Application::State& state() const;

        // Ссылка действительна, пока жив _alive; без проверки её не разыменовывать
        // The reference is valid while _alive lives; do not dereference it without the check
        Application& _app;
        std::weak_ptr<const bool> _alive;
    };
}

#endif
