#ifndef GRAN_RHI_ADAPTER_CHOICE_HPP
#define GRAN_RHI_ADAPTER_CHOICE_HPP

#include <string>

/*
** Явный выбор видеоадаптера переменной окружения GRAN_ADAPTER — часть имени без учёта регистра
** («intel», «nvidia», «RTX 3060»). Нужен там, где карт несколько: тестовый стенд Intel + NVIDIA,
** ноутбуки. Без переменной поведение прежнее (самый производительный адаптер). Если переменная
** задана, а подходящего адаптера нет — ошибка, а не тихий переход на WARP / lavapipe: иначе тест
** «прошёл бы» не на той карте. При forceWarp переменная не действует.
**
** Explicit video adapter choice through the GRAN_ADAPTER environment variable — a case-insensitive
** part of the name ("intel", "nvidia", "RTX 3060"). It is needed where there are several cards: the
** Intel + NVIDIA test stand, laptops. Without the variable the behavior is unchanged (the fastest
** adapter). If the variable is set but no suitable adapter exists, it is an error rather than a
** silent switch to WARP / lavapipe: otherwise a test would "pass" on the wrong card. With forceWarp
** the variable has no effect.
*/
namespace gran::rhi
{
    // Значение GRAN_ADAPTER; пустая строка — выбор не задан.
    // The GRAN_ADAPTER value; an empty string means no choice is set.
    std::string requestedAdapter();

    // Имя содержит запрошенную часть без учёта регистра ASCII; пустой запрос подходит любому имени.
    // The name contains the requested part, ignoring ASCII case; an empty request matches any name.
    bool adapterMatches(const std::string& request, const std::string& name);

    // Адаптер с этим именем разрешён текущим значением GRAN_ADAPTER.
    // The adapter with this name is allowed by the current GRAN_ADAPTER value.
    bool adapterAllowed(const std::string& name);

    // Бросает GpuException «нет адаптера GRAN_ADAPTER»; seen — имена просмотренных адаптеров.
    // Throws GpuException "no GRAN_ADAPTER adapter"; seen — the names of the adapters looked at.
    [[noreturn]] void adapterNotFound(const std::string& seen);
}

#endif
