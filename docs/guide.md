# Грань UI — руководство

**Русский** · [English](guide.en.md)

← [К описанию](../README.md)

## Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Окно и ресурсы](#окно-и-ресурсы)
3. [Элементы](#элементы)
4. [Компоновка](#компоновка)
5. [Значения стилей](#значения-стилей)
6. [Стили, тема и состояния](#стили-тема-и-состояния)
7. [Текст и шрифты](#текст-и-шрифты)
8. [Действия и программный интерфейс](#действия-и-программный-интерфейс)
9. [Всплывающие слои](#всплывающие-слои)
10. [Ошибки и язык сообщений](#ошибки-и-язык-сообщений)

## Быстрый старт

`hello.xml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Window title="Привет" width="420" height="220" background="#0f1117" color="#e6e8ee" fontSize="14">
  <Stack padding="24" spacing="12">
    <Text fontSize="20" fontWeight="600">Привет, Грань UI</Text>
    <Input id="name" placeholder="Как вас зовут?"/>
    <Stack orientation="horizontal" spacing="12" align="center">
      <Button text="Поздороваться" onClick="greet"/>
      <Text id="answer" color="#8b93a7"/>
    </Stack>
  </Stack>
</Window>
```

`main.cpp`:

```cpp
#include "gran/app/application.hpp"
#include <iostream>

// Действие кнопки: прочитать поле и показать ответ
struct GreetAction
{
    gran::Application* app;

    void operator()(gran::ui::Node& button) const
    {
        gran::ui::Node* name = app->find("name");
        gran::ui::Node* answer = app->find("answer");
        (void)button;
        if (name == nullptr || answer == nullptr)
            return;
        answer->content.text = "Привет, " + name->content.text + "!";
        gran::ui::markDirty(*answer, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }
};

int main()
{
    try
    {
        gran::Application app;
        app.loadFile("hello.xml");
        app.onAction("greet", GreetAction{&app});
        return app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
```

Как собрать — в [docs/building.md](building.md#подключение-к-своей-программе). Готовые разметки для опытов —
в `samples/`, их открывает `bin/gran_gallery <файл.xml>`.

## Окно и ресурсы

Корень документа — `<Window>`: `title`, `width`, `height` (по умолчанию 800 × 600), `minWidth`, `minHeight`,
а также любые атрибуты оформления, которые наследуют дети (`background`, `color`, `fontSize`, `fontFamily`).
Внутри — необязательный `<Resources>` и ровно один корневой элемент интерфейса.

```xml
<Window title="Окно" width="960" height="620">
  <Resources>
    <Theme>
      <Token name="accent" value="#6d5dfc"/>
      <Token name="radius" value="16"/>
    </Theme>
    <Font family="Inter" source="fonts/Inter-Regular.ttf" weight="400"/>
    <Style name="card" background="#171a23" borderRadius="$radius" padding="20"/>
  </Resources>
  <Stack style="card">…</Stack>
</Window>
```

Разметка проверяется встроенной схемой XSD 1.1 (`schemas/gran-ui.xsd`) прямо во время разбора. Неизвестный
атрибут, неверный цвет, ссылка на необъявленный стиль, повторный `id`, `minWidth` больше `maxWidth` или номер
строки `Grid` за пределами дорожек — ошибка с номером строки и путём ещё до создания окна.

## Элементы

| Элемент | Назначение | Свои атрибуты |
|---|---|---|
| `Stack` | Ряд или столбец детей (подмножество flexbox) | `orientation`, `spacing`, `justify`, `align`, `wrap` |
| `Grid` | Сетка строк и столбцов | `rows`, `columns`, `rowSpacing`, `columnSpacing` |
| `Canvas` | Абсолютное размещение детей | у детей `left`, `top`, `right`, `bottom` |
| `Box` | Пустой прямоугольник (фон, рамка, тень) | — |
| `Scroll` | Прокрутка ровно одного ребёнка | `direction="vertical\|horizontal\|both"` |
| `Text` | Текст, может содержать участки `Span` | `textWrap`, `textOverflow`, `maxLines`, `textAlign` |
| `Span` | Участок оформленного текста внутри `Text` | шрифт, `color`, `background`, `textDecoration`, `onClick` (ссылка) |
| `Image` | Картинка PNG | `source`, `fit="fill\|contain\|cover\|none"` |
| `Button` | Кнопка с текстом или содержимым | `text`, `onClick` |
| `Input` | Однострочное поле | `value`, `placeholder`, `maxLength`, `password`, `readOnly`, `onChange`, `onSubmit` |
| `TextArea` | Многострочное поле с переносом и прокруткой | `rows`, `submitKey="ctrlEnter\|enter"`, `maxHeight`, `onSubmit` |
| `CheckBox` | Флажок | `text`, `checked`, `onChange` |
| `Slider` | Ползунок | `min`, `max`, `value`, `step`, `onChange` |
| `Popup` | Всплывающий слой (скрыт, пока его не покажут) | — |
| `MenuItem` | Пункт меню в `Popup` | `text`, `onClick` |

Общие атрибуты всех элементов: `id`, `style`, `visible`, `enabled`, `tooltip`, атрибуты размеров, отступов и
оформления. Имена атрибутов — camelCase и одинаковые у всех элементов.

## Компоновка

Модель блока: `margin` (без схлопывания) → `borderWidth` → `padding` → содержимое. `width` и `height` — `auto`,
пиксели или проценты; `minWidth`, `maxWidth`, `minHeight`, `maxHeight` соблюдаются всегда. Отступы пишутся как в
CSS: `padding="8"`, `"8 16"`, `"4 8 12 16"`.

**Stack.** `orientation="vertical|horizontal"`, `spacing` — зазор между детьми. `justify` распределяет детей
по главной оси (`start`, `center`, `end`, `space-between`, `space-around`, `space-evenly`), `align` — по
поперечной (`start`, `center`, `end`, `stretch`). У детей: `grow`, `shrink`, `alignSelf`. `wrap="true"` переносит
детей на новую строку.

**Grid.** Дорожки перечисляются через запятую: число (пиксели), `auto`, `1fr`, `minmax(100, 1fr)`.

```xml
<Grid rows="auto, 1fr" columns="1fr, 1fr, 1fr" rowSpacing="20" columnSpacing="20">
  <Text gridColumnSpan="3">Заголовок во всю ширину</Text>
  <Box gridRow="1" gridColumn="0"/>
  <Box gridRow="1" gridColumn="1" gridColumnSpan="2"/>
</Grid>
```

Номера строк и столбцов — с нуля.

**Canvas.** Дети ставятся по `left`/`top`/`right`/`bottom`; `zIndex` меняет порядок наложения.

**Scroll.** Ребёнок больше `Scroll` — появляется полоса прокрутки, работают колесо и перетаскивание ползунка.

## Значения стилей

| Что | Примеры |
|---|---|
| Цвет | `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(…)`, `rgba(…)`, `hsl(…)`, `transparent`, имена CSS, `$токен` |
| Заливка (`background`) | цвет; `linear-gradient(135deg, #6d5dfc, #b05cff)`; `radial-gradient(…)`; `conic-gradient(…)` |
| Скругление | `borderRadius="12"` или по углам `"12 12 0 0"` |
| Тень | `boxShadow="0 10 30 rgba(0, 0, 0, 0.45)"`, несколько через запятую, `inset` — внутренняя |
| Преобразование | `transform="rotate(12deg) scale(1.05) translate(4, 0)"`, `transformOrigin` |
| Прозрачность и стекло | `opacity="0.8"`, `backdropBlur="24"` — размытие того, что под элементом |
| Переход | `transition="background 0.15s ease, transform 0.1s cubic-bezier(0.2, 0, 0, 1)"` |
| Курсор | `cursor="pointer\|text\|default"` |

Длины — в логических пикселях: на экране с масштабом 150 % движок сам умножает их на 1,5.
Цвета и смешивание — в sRGB с предумноженной альфой, как в браузерах.

## Стили, тема и состояния

Каскад, от слабого к сильному: умолчания движка ← встроенная тема (`schemas/default_theme.xml`) ← стиль
`style="имя"` ← атрибуты элемента ← активные состояния.

```xml
<Style name="primary" background="linear-gradient(135deg, #6d5dfc, #b05cff)" color="#ffffff">
  <Hover boxShadow="0 6 18 rgba(109, 93, 252, 0.45)"/>
  <Pressed transform="scale(0.98)"/>
  <Disabled opacity="0.5"/>
</Style>
<Style name="ghost" basedOn="primary" background="transparent" borderWidth="1"/>
```

- Состояния: `Hover`, `Pressed`, `Focused`, `Checked`, `Disabled`. Смена состояния плавная, если у свойства
  есть `transition`.
- `basedOn` наследует другой стиль. Тема по умолчанию задаёт стили с именами элементов (`Button`, `Input`,
  `TextArea`, `CheckBox`, `Slider`, `Scroll`, `Popup`, `MenuItem`, `Tooltip`) — свой `Style` с таким именем их
  заменяет.
- Токены темы (`<Token name="accent" value="#6d5dfc"/>`) подставляются в любое значение как `$accent`.
- `color`, `accentColor`, шрифт и кегль наследуются детьми, как в CSS.

## Текст и шрифты

- Шрифт: `fontFamily`, `fontSize`, `fontWeight` (100–900), `fontStyle="italic"`, `letterSpacing`, `lineHeight`.
  Системные шрифты Windows находятся по имени семейства; свой файл подключается в ресурсах
  `<Font family="…" source="путь.ttf" weight="…" italic="…"/>`. Глифов нет в шрифте (китайский, японский) —
  движок сам берёт запасной системный шрифт.
- `textWrap="wrap|nowrap"`, `textAlign="start|center|end|justify"`, `textOverflow="ellipsis"`, `maxLines`.
- Оформленный текст — участки `Span` внутри `Text`; пробелы схлопываются через границы участков, перенос идёт
  сквозь них:

```xml
<Text lineHeight="1.6">Обычный, <Span fontWeight="700">жирный</Span>,
  <Span fontStyle="italic">курсив</Span> и <Span style="link" onClick="open">ссылка</Span>.</Text>
```

- Текст `Text` и полей выделяется мышью (двойной щелчок — слово) и клавиатурой (Shift+стрелки, Ctrl+A),
  копируется Ctrl+C и через контекстное меню по правому щелчку. Поле `password="true"` показывает точки и не даёт
  копировать.

## Действия и программный интерфейс

Атрибуты `onClick`, `onChange`, `onSubmit` содержат имя действия. Приложение связывает имя с функцией:

```cpp
app.onAction("save", SaveAction{&app});   // функция получает узел, вызвавший действие
```

`onClick` — щелчок или Enter/Пробел на кнопке, пункте меню, ссылке. `onChange` — изменение поля, флажка,
ползунка. `onSubmit` — Enter в `Input`, клавиша `submitKey` в `TextArea`.

Класс `gran::Application` (`gran/app/application.hpp`):

| Метод | Что делает |
|---|---|
| `Application(AppOptions)` | `debugLayer` — отладочный слой D3D12, `vsync`, `exitAfter` — выход через N секунд |
| `loadFile(путь)` / `loadString(разметка)` | Загрузить и проверить разметку, построить окно |
| `onAction(имя, функция)` | Связать имя действия из разметки с функцией C++ |
| `find(id)` | Найти узел по `id` (или `nullptr`) |
| `setStyle(узел, имя)` | Сменить именованный стиль узла |
| `append(родитель, узел)` / `remove(узел)` | Вставить узел (например, копию шаблона `ui::cloneTree`) / удалить |
| `focus(узел)` | Передать фокус клавиатуры |
| `showPopup(слой, якорь)` / `showPopupAt(слой, точка)` / `hidePopup(слой)` | Всплывающие слои |
| `post(задача)` | Выполнить функцию в потоке интерфейса (безопасно из любого потока) |
| `run()` | Цикл событий до закрытия окна; возвращает код выхода |

Узел `gran::ui::Node` (`gran/ui/node.hpp`) хранит вид элемента, `id`, содержимое `content` (`text`, `value`,
`checked`, `placeholder`…) и вычисленный стиль. После изменения содержимого из кода узел помечается
`ui::markDirty(узел, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT)`: движок перекомпонует и перерисует только нужное.

**Шаблоны.** Повторяющийся блок (сообщение чата, строка списка) описывается в разметке скрытым образцом с
`visible="false"`; код копирует его `ui::cloneTree`, правит текст в копии (`ui::findNode`) и вставляет
`append`. Так устроен пример `examples/chat/`.

**Фоновые потоки.** Обращаться к узлам можно только из потока интерфейса. Поток сети или вычислений передаёт
результат через `app.post(задача)` — задача выполнится в цикле событий, а окно проснётся само.

## Всплывающие слои

```xml
<Button id="more" text="Файл ▾" onClick="more"/>
<Popup id="moreMenu">
  <MenuItem text="Открыть…" onClick="open"/>
  <MenuItem text="Экспорт" enabled="false"/>
</Popup>
```

```cpp
// В действии "more": показать меню под кнопкой
app.showPopup(*app.find("moreMenu"), button);
```

`Popup` вынут из потока компоновки и рисуется поверх всего. Он встаёт под якорем (над ним, если внизу не хватает
места), `showPopupAt` — у точки с отражением внутрь окна. Esc и щелчок мимо закрывают слой, стрелки ходят по
пунктам, Enter выбирает. Атрибут `tooltip` у любого элемента показывает подсказку через 0,6 с.

## Ошибки и язык сообщений

Все ошибки движка — исключения `gran::GranException` (`gran/core/gran_exception.hpp`) с кодом `ErrorCode`:

- `MarkupException` — ошибка разметки или схемы: `getLine()`, `getColumn()`, путь к элементу в тексте;
- `GpuException` — ошибка видеокарты с исходным кодом `HRESULT`.

Тексты сообщений хранятся в каталоге (русский и английский). Язык меняется вызовом
`gran::i18n::setLanguage("en")` (`gran/core/i18n.hpp`).
