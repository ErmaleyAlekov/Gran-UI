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
7. [Анимации и таймеры](#анимации-и-таймеры)
8. [Текст и шрифты](#текст-и-шрифты)
9. [Действия и программный интерфейс](#действия-и-программный-интерфейс)
10. [Списки из данных](#списки-из-данных)
11. [Вкладки, радиокнопки, переключатели](#вкладки-радиокнопки-переключатели)
12. [Всплывающие слои](#всплывающие-слои)
13. [Встроенный браузер: WebView](#встроенный-браузер-webview)
14. [Доступность](#доступность)
15. [Платформа: окна, файлы, иконки](#платформа-окна-файлы-иконки)
16. [Ошибки и язык сообщений](#ошибки-и-язык-сообщений)

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

Корень документа — `<Window>`: `title`, `width`, `height` (по умолчанию 800 × 600), `minWidth`, `minHeight`, `onClose`,
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
| `Icon` | Векторная иконка: путь SVG или файл `.svg`, цвет — `color` | `data`, `viewBox`, `fillRule`, `source` |
| `Image` | Картинка PNG или JPEG | `source`, `fit="fill\|contain\|cover\|none"` |
| `Button` | Кнопка с текстом или содержимым | `text`, `onClick`, `popup` (id своего меню или диалога) |
| `Input` | Однострочное поле | `value`, `placeholder`, `maxLength`, `password`, `readOnly`, `onChange`, `onSubmit` |
| `TextArea` | Многострочное поле с переносом и прокруткой | `rows`, `submitKey="ctrlEnter\|enter"`, `maxHeight`, `onSubmit` |
| `CheckBox` | Флажок | `text`, `checked`, `onChange` |
| `Slider` | Ползунок | `min`, `max`, `value`, `step`, `onChange` |
| `RadioButton` | Радиокнопка: из группы отмечена одна | `text`, `value`, `checked`, `group`, `onChange` |
| `Switch` | Переключатель (флажок другого вида) | `text`, `checked`, `onChange` |
| `ProgressBar` | Полоса прогресса | `min`, `max`, `value` |
| `Tabs` | Вкладки: полоса заголовков и выбранная страница | `value`, `onChange`, страницы `Tab` (`text`, `value`) |
| `Popup` | Всплывающий слой (скрыт, пока его не покажут) | — |
| `MenuItem` | Пункт меню в `Popup`; вложенный `Popup` — подменю | `text`, `onClick`, `value` (для `ComboBox`) |
| `Dialog` | Модальный диалог по центру окна | `closable` |
| `ComboBox` | Выпадающий список с выбранным значением | `value`, `placeholder`, `onChange`, пункты `MenuItem` или `items` |
| `WebView` | Страница системного браузера (по желанию, см. [ниже](#встроенный-браузер-webview)) | `source` или HTML в теге, `onMessage` |
| `ItemTemplate` | Вид одной записи списка внутри контейнера с `items` | ровно один элемент |

Контейнеры (`Stack`, `Box`, `Grid`, `Canvas`, `Button`, `Popup`, `Dialog`) и `ComboBox` принимают `items="модель"`
вместе с `<ItemTemplate>` — см. [Списки из данных](#списки-из-данных).

Общие атрибуты всех элементов: `id`, `style`, `visible`, `enabled`, `tooltip`, `accessibleName`, `labeledBy`
(см. [Доступность](#доступность)), атрибуты размеров, отступов и оформления. Имена атрибутов — camelCase и одинаковые у всех элементов.

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
| Анимация | `animation="pulse 0.8s ease-in-out infinite alternate"` — по ключевым кадрам `<Keyframes>` |
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

- Состояния: `Hover`, `Pressed`, `Focused`, `Checked`, `Disabled`, `DropTarget` (над зоной `onDrop` несут файлы). Смена состояния плавная, если у свойства
  есть `transition`; состояние может и запустить анимацию: `<Hover animation="wiggle 0.4s"/>`.
- `basedOn` наследует другой стиль. Тема по умолчанию задаёт стили с именами элементов (`Button`, `Input`,
  `TextArea`, `CheckBox`, `Slider`, `RadioButton`, `Switch`, `ProgressBar`, `Tabs`, `TabBar`, `TabHeader`, `Tab`,
  `Scroll`, `Popup`, `MenuItem`, `Dialog`, `ComboBox`, `Tooltip`) — свой `Style`
  с таким именем их заменяет.
- Токены темы (`<Token name="accent" value="#6d5dfc"/>`) подставляются в любое значение как `$accent`.
- `color`, `accentColor`, шрифт и кегль наследуются детьми, как в CSS.

## Анимации и таймеры

**Переходы** (`transition`) плавно ведут свойство от старого значения к новому, когда меняется состояние
или стиль. **Анимации по ключевым кадрам** играют сами — при загрузке, при появлении состояния, бесконечно, —
как `@keyframes` в CSS.

```xml
<Resources>
  <Keyframes name="pulse">
    <Frame at="0%" opacity="1" transform="scale(1)"/>
    <Frame at="100%" opacity="0.35" transform="scale(0.8)"/>
  </Keyframes>
  <Keyframes name="appear">
    <Frame at="0%" opacity="0" transform="translate(0, 16)"/>
  </Keyframes>
  <Style name="card" background="#ffffff" padding="16" animation="appear 0.6s ease-out"/>
</Resources>
…
<Box width="28" height="28" borderRadius="14" background="#e11d48"
     animation="pulse 0.8s ease-in-out infinite alternate"/>
```

- `<Keyframes name>` объявляются в `<Resources>` до стилей. В кадре `<Frame at="0%…100%">` — только анимируемые
  свойства: `background` (цвет), `borderColor`, `color`, `accentColor`, `opacity`, `transform`, `boxShadow`,
  `borderRadius`. Свойство, которого нет в кадре 0 % или 100 %, берёт там своё обычное значение (так `appear`
  выше задаёт только начало). `easing` у кадра — кривая участка до следующего кадра.
- `animation="имя длительность [кривая] [задержка] [повторы|infinite] [направление] [заполнение]"`, несколько —
  через запятую, `none` — без анимации. Направление: `normal`, `reverse`, `alternate`, `alternate-reverse`;
  заполнение: `none`, `forwards` (держать последний кадр), `backwards` (первый кадр во время задержки), `both`.
  В отличие от CSS имя всегда стоит первым.
- Анимация запускается, когда имя появляется в стиле узла (загрузка, `setStyle`, состояние вроде `Hover`),
  и снимается, когда исчезает. Закончившаяся не повторяется, пока имя остаётся в стиле; чтобы проиграть её
  снова, смените стиль узла на стиль без неё, а в следующем кадре — обратно.
- Свойства, которые ведёт анимация, переходами не анимируются. Анимации скрытых узлов (`visible="false"` у узла
  или предка: закрытый слой, другая вкладка) не будят перерисовку и при показе сразу оказываются на своём месте.
- Необъявленное имя в `animation` — ошибка загрузки с номером строки. Градиенты и преобразования разной формы
  (`rotate` → `scale`) не смешиваются: значение меняется скачком.

**Таймеры** — функции, которые цикл событий вызывает в потоке интерфейса:

```cpp
gran::TimerId tick = app.setInterval(0.05, ProgressTick{&app});   // каждые 50 мс
app.setTimeout(2.0, ShowHint{&app});                              // один раз через 2 секунды
app.clearTimer(tick);
```

- После срабатывания кадр перерисовывается; в простое поток спит до срока ближайшего таймера. Точность — кадр
  (около 16 мс при vsync) или пробуждение цикла; период не меньше 1 мс.
- Периодический таймер вызывает один и тот же объект-функцию: его поля (например, счётчик) сохраняются между
  срабатываниями. Пропущенные срабатывания (поток был занят) не догоняются пачкой.
- Таймер можно снять из его же обработчика; таймер, поставленный обработчиком, сработает не раньше следующего
  прохода цикла. Ставить и снимать таймеры можно только в потоке интерфейса — фоновый поток делает это через `post`.

Образец — `samples/animations.xml` (`gran_gallery samples/animations.xml`): вращение, пульс, смена цвета,
неопределённый прогресс, появление карточек, анимация при наведении; полоса и счётчик секунд — таймерами галереи.

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
переключателя, ползунка, выбор радиокнопки (действие у отмеченной) или вкладки (действие у `Tabs`). `onSubmit` — Enter в `Input`, клавиша `submitKey` в `TextArea`.

Класс `gran::Application` (`gran/app/application.hpp`):

| Метод | Что делает |
|---|---|
| `Application(AppOptions)` | `debugLayer` — отладочный слой D3D12, `vsync`, `exitAfter` — выход через N секунд |
| `loadFile(путь)` / `loadString(разметка)` | Загрузить и проверить разметку, построить окно |
| `onAction(имя, функция)` | Связать имя действия из разметки с функцией C++ |
| `find(id)` | Найти узел по `id` (или `nullptr`) |
| `setStyle(узел, имя)` | Сменить именованный стиль узла |
| `append(родитель, узел)` / `remove(узел)` | Вставить узел (например, копию шаблона `ui::cloneTree`) / удалить |
| `appendMarkup(родитель, "<Stack>…</Stack>")` | Вставить фрагмент разметки (та же схема, стили и тема документа) |
| `model(имя)` / `itemOf(узел)` | Модель списка `items="имя"` / запись, к которой относится узел |
| `onWebMessage`, `postWebMessage`, `navigate`, `loadHtml` | Встроенный браузер `<WebView>` |
| `focus(узел)` | Передать фокус клавиатуры |
| `showPopup(слой, якорь)` / `showPopupAt(слой, точка)` / `hidePopup(слой)` | Всплывающие слои |
| `showDialog(диалог)` / `hideDialog(диалог)` | Модальный диалог |
| `selectOption(список, значение)` | Выбрать пункт `ComboBox` из кода (без `onChange`) |
| `selectTab(вкладки, значение)` | Выбрать страницу `Tabs` из кода (без `onChange`) |
| `setValue(узел, число)` / `setChecked(узел, да/нет)` | Число `ProgressBar`/`Slider`, отметка `CheckBox`/`Switch`/`RadioButton` (без `onChange`) |
| `post(задача)` | Выполнить функцию в потоке интерфейса (безопасно из любого потока) |
| `setTimeout(секунды, функция)` / `setInterval(секунды, функция)` / `clearTimer(номер)` | Таймеры в потоке интерфейса |
| `openWindow(путь)` / `openWindowString(разметка)` / `closeWindow(узел)` | Ещё одно окно / закрыть окно узла |
| `openFileDialog(параметры, несколько)` / `saveFileDialog` / `folderDialog` | Системные диалоги файлов |
| `onFileDrop(имя, функция)` | Файлы, брошенные на элемент с `onDrop="имя"` |
| `run()` | Цикл событий до закрытия главного окна; возвращает код выхода |

Узел `gran::ui::Node` (`gran/ui/node.hpp`) хранит вид элемента, `id`, содержимое `content` (`text`, `value`,
`checked`, `placeholder`…) и вычисленный стиль. После изменения содержимого из кода узел помечается
`ui::markDirty(узел, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT)`: движок перекомпонует и перерисует только нужное.

**Шаблоны.** Повторяющийся блок одинакового вида лучше описать списком из данных (раздел ниже).
Если блок собирается сложнее (как ответ с Markdown в `examples/chat/`), его можно вставить фрагментом
разметки `appendMarkup` или скопировать скрытый образец `ui::cloneTree` и вставить `append`.

**Фоновые потоки.** Обращаться к узлам можно только из потока интерфейса. Поток сети или вычислений передаёт
результат через `app.post(задача)` — задача выполнится в цикле событий, а окно проснётся само.

## Списки из данных

Разметка описывает вид одной записи, данные живут в модели внутри движка — коду не нужно строить узлы:

```xml
<Resources>
  <Data name="contacts">                       <!-- начальные данные (можно и из кода) -->
    <Item name="Анна" mail="anna@example.org"/>
    <Item name="Борис" mail="boris@example.org"/>
  </Data>
</Resources>
...
<Stack items="contacts" spacing="8">
  <Text>Контакты</Text>                       <!-- свои дети остаются первыми -->
  <ItemTemplate>
    <Stack style="row" tooltip="{mail}" onClick="openContact">
      <Text fontWeight="600">{name}</Text>
      <Text>Почта: <Span color="#2563eb">{mail}</Span></Text>
    </Stack>
  </ItemTemplate>
</Stack>
```

```cpp
gran::data::ListModel& contacts = app.model("contacts");
contacts.append({{"name", "Вера"}, {"mail", "vera@example.org"}});   // + копия шаблона
contacts.set(0, "name", "Анна С.");                                  // меняется только этот текст
contacts.remove(1);                                                  // удаляется одна копия

// в обработчике действия openContact:
gran::ItemRef item = app.itemOf(node);              // item.model == &contacts, item.index — номер записи
```

- Подстановки `{поле}` работают в `text`, `placeholder`, `source`, `tooltip`, `accessibleName` и в тексте тегов (в том числе
  в `Span`). `{{` и `}}` — сами скобки; поле, которого нет в записи, даёт пустую строку.
- Правка модели меняет только затронутые копии: вставка и удаление — свои узлы, смена поля — только тексты
  с подстановками. Фокус, выделение и прокрутка остальных записей сохраняются.
- Запись — строки (`gran::data::Record`: имя поля → текст); формат значений решает приложение.
- `<Data>` при загрузке заменяет содержимое модели; модель можно заполнять и до загрузки разметки.
- Списки можно вкладывать (шаблон внутри шаблона), но вложенный список связан с моделью по имени — общей
  для всех записей.
- Модель меняется только в потоке интерфейса; фоновый поток передаёт данные через `app.post`.
- **Длинный список** — `virtual="true"` у `Stack` с `items` внутри `Scroll`: копии строятся только для записей около
  видимой области (плюс по 8 с каждой стороны), остальные заменяют две распорки. Список из 100 000 записей строится
  за доли миллисекунды и прокручивается за ~1 мс на кадр. Условие — записи примерно одной высоты: положение
  записи вычисляется через средний размер, у сильно разных по высоте записей полоса прокрутки немного «плывёт».
  Копии вне окна не существуют: `app.find` и `itemOf` видят только записи в окне, а состояние внутри копии
  (выделение, фокус) при уходе записи за окно теряется.
- `gran_snapshot` строит списки из `<Data>` без окна — кадр списка проверяется без программы
  (пример — `samples/lists.xml`).

Фрагмент разметки во время работы — `app.appendMarkup(родитель, "<Stack>…</Stack>")`: один корневой элемент,
проверка той же схемой, стили `style="…"` и значения темы `$имя` — из документа. Ошибка — `MarkupException`
с позицией внутри фрагмента; дерево при этом не меняется.

## Вкладки, радиокнопки, переключатели

**Радиокнопки** одной группы отмечаются по одной. Без `group` группа — соседние радиокнопки без имени; с `group`
— все радиокнопки окна с этим именем, где бы они ни стояли. Значение — `value` (без него — подпись), `onChange`
вызывается у кнопки, которую отметили. **Переключатель** `Switch` ведёт себя как `CheckBox`.

```xml
<RadioButton text="Светлая" value="light" checked="true" onChange="theme"/>
<RadioButton text="Тёмная" value="dark" onChange="theme"/>
<Switch text="Звук" checked="true" onChange="sound"/>
<ProgressBar id="progress" value="65" width="320"/>
```

**Полоса прогресса**: дорожка — фон стиля `ProgressBar`, заполнение — `accentColor`; высота и скругление — из
стиля. Значение из кода — `app.setValue(*app.find("progress"), 80)`.

**Вкладки**: страницы `Tab` с подписью `text`; движок сам строит над ними полосу заголовков (стили темы `TabBar`
и `TabHeader`, выбранный заголовок — в состоянии `Checked`). Видна только выбранная страница; `value` у `Tabs` —
значение выбранной (значение страницы — `value`, без него — `text`), без него выбрана первая.

```xml
<Tabs value="general" onChange="tabChanged">
  <Tab text="Общие" value="general"> … </Tab>
  <Tab text="Загрузка" value="download"> … </Tab>
</Tabs>
```

Клавиатура как в браузерах: Tab попадает только на отмеченную радиокнопку группы и на выбранную вкладку, стрелки
переносят выбор внутри группы (у вкладок — влево и вправо), пробел и Enter выбирают. Образец —
`gran_gallery samples/widgets.xml`.

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

**Меню без кода.** Кнопка с `popup="id"` открывает свой слой сама: щелчком, Enter, пробелом и стрелкой вниз.
С клавиатуры фокус сразу встаёт на первый пункт (так же и у меню, которое открывает код в `onClick`).
Esc закрывает меню и возвращает фокус кнопке.

**Подменю** — `Popup` внутри `MenuItem`. Оно открывается сбоку от пункта (справа, а если там не хватает места —
слева), наведением мыши, стрелкой вправо или Enter; стрелка влево и Esc закрывают только его. Пункт с подменю
помечен стрелкой и сам команду не выполняет; выбор пункта в подменю закрывает всю цепочку.

```xml
<Button text="Файл ▾" popup="fileMenu"/>
<Popup id="fileMenu">
  <MenuItem text="Открыть…" onClick="open"/>
  <MenuItem text="Экспорт">
    <Popup>
      <MenuItem text="PDF" onClick="exportPdf"/>
      <MenuItem text="PNG" onClick="exportPng"/>
    </Popup>
  </MenuItem>
</Popup>
```

**Выпадающий список** `ComboBox` показывает подпись выбранного пункта (без выбора — `placeholder`). Выбор
другого пункта меняет значение и вызывает `onChange`; значение пункта — его `value`, а без него — текст.
Пункты пишутся прямо в теге или берутся из модели — так же, как [списки из данных](#списки-из-данных).
Открытый список не уже поля, фокус в нём стоит на выбранном пункте, выбранный пункт отмечен стилем `Checked`.

```xml
<ComboBox id="theme" value="light" onChange="themeChanged">
  <MenuItem value="light" text="Светлая"/>
  <MenuItem value="dark" text="Тёмная"/>
</ComboBox>
<ComboBox id="size" placeholder="Выберите…" items="sizes" onChange="sizeChanged">
  <ItemTemplate><MenuItem value="{key}" text="{name}"/></ItemTemplate>
</ComboBox>
```

```cpp
// В обработчике themeChanged: значение — combo.content.option, подпись — combo.content.text
app.selectOption(*app.find("size"), "m");   // выбрать из кода, onChange не вызывается
```

**Диалог** `Dialog` — модальный слой: встаёт по центру окна, окно под ним затемняется (свойство стиля `scrim`,
в теме — полупрозрачный тёмный), щелчки и колесо мимо диалога никуда не попадают, Tab и Shift+Tab ходят только
по его элементам. При открытии фокус встаёт на первый элемент диалога, при закрытии возвращается туда, где был.
Esc закрывает диалог, если не задано `closable="false"` (тогда закрыть его может только код). Меню и списки
внутри диалога работают как обычно и рисуются над ним.

```xml
<Button text="Удалить…" popup="confirm"/>
<Dialog id="confirm" closable="false">
  <Text fontWeight="700">Удалить файл?</Text>
  <Stack orientation="horizontal" spacing="8" justify="end">
    <Button text="Отмена" onClick="cancel"/>
    <Button text="Удалить" onClick="remove"/>
  </Stack>
</Dialog>
```

```cpp
app.showDialog(*app.find("confirm"));   // открыть из кода
app.hideDialog(*app.find("confirm"));   // закрыть (обычно в обработчике кнопки)
```

Слои рисуются в порядке дерева: вложенный слой (меню в диалоге, подменю) — над своим, поэтому диалог, который
открывается из другого диалога, удобно объявить внутри него. Пример всего вместе — `samples/dialogs.xml`
(`gran_gallery samples/dialogs.xml`).

## Встроенный браузер: WebView

Грань UI сама браузер не содержит и для интерфейса его не использует: окно, текст, поля и списки рисует
движок. Если программе нужна веб-страница (справка, отчёт, вход через сайт, готовый веб-виджет), её показывает
элемент `<WebView>` — **гибрид**: всё окно рисует движок, а внутри прямоугольника `<WebView>` страницу
показывает браузер системы.

```xml
<WebView id="page" source="https://example.org" height="320" onMessage="fromPage"/>
<WebView id="help" source="help/index.html" grow="1"/>          <!-- файл рядом с разметкой -->
<WebView id="card" height="200"><![CDATA[<h2>HTML прямо в разметке</h2>]]></WebView>
```

```cpp
app.onWebMessage("fromPage", FromPage{&app});   // void(ui::Node& webView, const std::string& text)
app.postWebMessage(*app.find("page"), R"({"theme":"dark"})");
app.navigate(*app.find("help"), "help/faq.html");
```

В странице:

```js
window.chrome.webview.postMessage("ready");                            // → обработчик onMessage
window.chrome.webview.addEventListener("message", e => show(e.data));  // ← postWebMessage
```

Как это устроено:

- **Ленивое создание.** Браузер создаётся, только когда `<WebView>` впервые появляется на экране. Окно без
  видимых `WebView` не тратит на браузер ни памяти, ни времени запуска.
- **Отдельный процесс.** Страница живёт в процессах браузера системы, не в процессе программы; её сбой
  не роняет интерфейс.
- **Только строки.** Мост — сообщения-строки в обе стороны (формат, например JSON, выбирает программа).
  Объекты C++ в скрипт страницы не передаются; инструменты разработчика (F12) включены только
  с `AppOptions::debugLayer`.
- **Размер — из стиля.** У страницы нет собственного размера: задайте `width`/`height` или `grow`.
  Рамка и отступы элемента видны вокруг страницы; часть, ушедшая за край `Scroll` или `overflow="hidden"`,
  обрезается.
- **Ограничения.** Страница лежит поверх нарисованного движком: всплывающий слой (`Popup`, подсказка)
  над `WebView` окажется под страницей, а `borderRadius`, `opacity` и `transform` к самой странице
  не применяются. Их снимет режим композиции через DirectComposition — следующий шаг.

**Платформы и сборка.** Windows — WebView2 (Edge, Chromium); macOS (WKWebView) и Linux (WebKitGTK) — в планах.
Поддержка включается при сборке: `make WEBVIEW2_SDK=<пакет>/build/native` или
`cmake -DGRAN_WEBVIEW2_SDK=<пакет>/build/native` (пакет NuGet `Microsoft.Web.WebView2`). Без этого параметра
движок собирается как раньше, а `<WebView>` остаётся белой подложкой.

Программе с `WebView` нужны две вещи: `WebView2Loader.dll` рядом с exe (сборка кладёт её рядом с программами;
ищется только в каталоге программы и в System32) и среда WebView2 Runtime (в Windows 11 уже есть, в Windows 10
ставится отдельно). Загрузчик подключается через `LoadLibrary` при первом показе `WebView`, поэтому программы
без `WebView` о нём не знают. Если загрузчика или среды нет, `WebView` остаётся подложкой, в stderr выводится
причина, программа работает дальше. Данные браузера (кэш, cookies) хранятся в
`%LOCALAPPDATA%\GranUI\<имя программы>\WebView2`. Пример — `samples/web_view.xml`.

## Доступность

Окно отдаёт дерево элементов экранным дикторам (Экранный диктор, NVDA, JAWS) и средствам автотестов через
Microsoft UI Automation. Роль элемента берётся из его вида, имя — по порядку:

1. `accessibleName="…"` — явное имя (как `aria-label`);
2. `labeledBy="id"` — текст другого элемента, например подписи рядом с полем (как `aria-labelledby`; схема
   проверяет, что такой `id` есть);
3. свой текст; у `Input`, `TextArea` и `ComboBox` — `placeholder` (текст поля — его значение); у кнопки без
   текста — тексты детей;
4. `tooltip`.

```xml
<Text id="volumeLabel" text="Громкость"/>
<Slider labeledBy="volumeLabel" min="0" max="100"/>
<Input accessibleName="Поиск по контактам" placeholder="Найти…"/>
```

Шаблоны управления — то, что клиент может сделать с элементом сам:

| Элемент | Шаблон UI Automation | Что делает клиент |
|---|---|---|
| `Button`, `MenuItem`, любой элемент с `onClick` | Invoke | нажать (как щелчок: onClick, меню, выбор пункта) |
| `CheckBox`, `Switch` | Toggle | переключить (отметка и onChange) |
| `RadioButton`, заголовок вкладки | SelectionItem | выбрать (соседи по группе снимаются, onChange) |
| `Input`, `TextArea`, `ComboBox` | Value | прочитать текст; у поля — заменить (как «выделить всё и вставить», с `maxLength` и onChange) |
| `Slider`, `ProgressBar` | RangeValue | прочитать число, минимум, максимум и шаг; у ползунка — задать (с шагом и onChange) |

Поле `readOnly`, `ComboBox` и `ProgressBar` отвечают «только чтение». Действия клиента выполняются в цикле
приложения теми же функциями, что ввод человека: отключённый элемент (`enabled="false"`, в том числе через
предка) и элементы под открытым диалогом не нажимаются. Клиент может перевести фокус на элемент, который
принимает его с Tab. Изменения отметки, текста и числа у элемента в фокусе сообщаются дикторам событиями —
они читают новое значение сразу. Пароль не раскрывается: значение поля `password="true"` пустое.

Поток окна входит в однопоточный апартамент COM (как и для `WebView`): так UI Automation вызывает провайдер
в потоке окна, через его очередь сообщений, и дерево не читается параллельно с изменениями. Если программа
до создания окна перевела поток в многопоточный апартамент, дерево доступности окну не отдаётся.

## Платформа: окна, файлы, иконки

Образец всего раздела — `samples/platform.xml` (`gran_gallery samples/platform.xml`).

**Несколько окон.** Первое загруженное окно (`loadFile`) — главное, его закрытие завершает `run()`. Ещё одно окно —
`openWindow("окно.xml")`: у него своя тема, стили, данные и дерево, а действия (`onAction`), модели списков, таймеры
и задачи — общие. Метод возвращает корень содержимого окна; окно появляется на ближайшем проходе цикла.
`closeWindow(узел)` закрывает окно, которому принадлежит узел (после текущего прохода событий). `find(id)` ищет
во всех окнах, начиная с главного.

```xml
<Window title="Настройки" width="380" height="200" onClose="settingsClosing">…</Window>
```

`onClose` — действие на попытку закрыть окно (крестик, Alt+F4). Если оно задано, окно не закрывается само: решает
обработчик — `app.closeWindow(узел)` или ничего (например, сначала спросить о несохранённом). Без зарегистрированного
обработчика окно закрывается как обычно.

**Диалоги файлов.** Системные диалоги Windows, модальные над окном узла `owner` (по умолчанию — над главным).
Пустой результат — пользователь отказался.

```cpp
gran::FileDialogOptions options;
options.title = "Открыть картинку";
options.filters = {{"Картинки", "*.png;*.jpg"}, {"Все файлы", "*.*"}};
std::vector<std::string> files = app.openFileDialog(options, true, &node);   // несколько файлов
std::string target = app.saveFileDialog(options);                           // спросит о замене файла
std::string folder = app.folderDialog(options);
```

Поля параметров: `title`, `folder` (начальная папка), `fileName` (предлагаемое имя), `filters`, `defaultExtension`
(без точки; добавляется при сохранении, если пользователь его не написал). Пути — в UTF-8.

**Перетаскивание файлов.** Атрибут `onDrop="имя"` делает элемент зоной сброса. Пока над ней несут файлы, у неё
состояние `DropTarget` (подсветка в стиле), над остальным окном курсор показывает «нельзя».

```xml
<Box onDrop="filesDropped" borderWidth="2" borderColor="#c3c8d2" transition="background 0.15s">
  <DropTarget borderColor="#2f6fed" background="#eaf1ff"/>
  <Text>Перетащите сюда файлы</Text>
</Box>
```

```cpp
app.onFileDrop("filesDropped", DropHandler{&app});   // void(ui::Node& зона, const std::vector<std::string>& пути)
```

**Иконки.** `<Icon>` рисует векторную фигуру цветом `color` (как текст, с переходами и анимациями цвета). Размер
по умолчанию — размер `viewBox`; `width`/`height` растягивают иконку, растр строится под итоговый размер
в пикселях устройства, поэтому иконка резкая при любом масштабе и DPI.

```xml
<Icon data="M11 5h2v6h6v2h-6v6h-2v-6H5v-2h6z" color="$accent"/>      <!-- viewBox по умолчанию 0 0 24 24 -->
<Icon data="M12 2a10 10 0 1 0 0 20a10 10 0 1 0 0-20zM12 6a6 6 0 1 0 0 12a6 6 0 1 0 0-12z" fillRule="evenodd"/>
<Icon source="icons/folder.svg" width="32" height="32"/>
```

- `data` — путь SVG: все команды (`M L H V C S Q T A Z`, в обоих регистрах), неверный путь — ошибка загрузки
  разметки с позицией. `viewBox` и `fillRule` (`nonzero` или `evenodd`) — только вместе с `data`.
- `source` — файл `.svg`: учитываются `viewBox` (или `width`/`height`) и залитые `<path>`, `<rect>` (со скруглением),
  `<circle>`, `<ellipse>`, `<polygon>`, `<polyline>` внутри любых `<g>`, `fill="none"` пропускается. Обводки,
  `transform` и стили CSS не поддерживаются — для иконок с обводкой (Lucide, Feather) нужен вариант с заливкой.
- Испорченный или недоступный файл, как у `Image`, не останавливает окно: иконка просто не рисуется.

**JPEG.** `Image` понимает PNG и JPEG (формат — по содержимому файла, а не по расширению): последовательный
и прогрессивный JPEG, серый и цветной, любая субдискретизация цветности. CMYK, 12 бит и арифметическое кодирование
не поддерживаются.

## Ошибки и язык сообщений

Все ошибки движка — исключения `gran::GranException` (`gran/core/gran_exception.hpp`) с кодом `ErrorCode`:

- `MarkupException` — ошибка разметки или схемы: `getLine()`, `getColumn()`, путь к элементу в тексте;
- `GpuException` — ошибка видеокарты с исходным кодом `HRESULT`.

Тексты сообщений хранятся в каталоге (русский и английский). Язык меняется вызовом
`gran::i18n::setLanguage("en")` (`gran/core/i18n.hpp`).
