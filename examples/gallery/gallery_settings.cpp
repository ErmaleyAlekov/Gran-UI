#include "gallery_actions.hpp"

#include <string>

namespace gallery
{
    namespace
    {
        // «Сменить ширину»: атрибут width полосы с persist — 160 ↔ 320 через живую разметку / "Change the width": the width attribute of the persist bar — 160 ↔ 320 through the live markup
        struct SettingsBar
        {
            gran::Application* application;

            void operator()(gran::ui::Node&) const
            {
                gran::ui::Node* bar = application->find("settingsBar");
                std::string width = "320";

                if (bar == nullptr)
                    return;
                if (application->markupAttribute(*bar, "width").value_or("") == "320")
                    width = "160";
                application->setMarkupAttribute(*bar, "width", width);
            }
        };
    }

    /*
    ** Кнопка полосы и счётчик запусков: значение программы в настройках (launches) растёт при каждом
    ** запуске галереи и сохраняется в конце run() вместе с раскладкой окна.
    **
    ** The bar button and the launch counter: a program value in the settings (launches) grows on every
    ** gallery start and is saved at the end of run() together with the window layout.
    */
    void registerSettings(gran::Application& application)
    {
        gran::settings::Settings& settings = application.settings();
        double launches = settings.getNumber("launches") + 1.0;
        gran::ui::Node* info = application.find("settingsInfo");

        application.onAction("settingsBar", SettingsBar{&application});
        settings.setNumber("launches", launches);
        if (info == nullptr)
            return;
        info->content.text += " Запусков галереи: " + std::to_string(static_cast<long long>(launches))
            + ". Файл: " + settings.path();
        gran::ui::markDirty(*info, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }
}
