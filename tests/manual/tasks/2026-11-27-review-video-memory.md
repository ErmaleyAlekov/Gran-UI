## Т-249. Учёт видеопамяти и журнал

Сначала пройдите [общие пункты ревью кода О-1…О-8](../README.md#code-review-rules), затем переходите к шагам этой задачи.

**Зачем.** Проверить, что журнал показывает сопоставимые величины на разных адаптерах и корректно учитывает ресурсы после пересоздания устройства.

**Приоритет.** Желательно до выпуска. Оценка — 35 минут.

**Подготовка.** Откройте перечисленные исходники в редакторе. Для чтения кода сборка и запуск не требуются.

### Что читать

- [include/gran/app/internal/app_state.hpp](../../../include/gran/app/internal/app_state.hpp)
- [include/gran/app/internal/app_window.hpp](../../../include/gran/app/internal/app_window.hpp)
- [include/gran/render/renderer.hpp](../../../include/gran/render/renderer.hpp)
- [include/gran/rhi/dx11/internal/dx11_backend_impl.hpp](../../../include/gran/rhi/dx11/internal/dx11_backend_impl.hpp)
- [include/gran/rhi/dx12/internal/dx12_backend_impl.hpp](../../../include/gran/rhi/dx12/internal/dx12_backend_impl.hpp)
- [include/gran/rhi/dx12/internal/dx12_internal.hpp](../../../include/gran/rhi/dx12/internal/dx12_internal.hpp)
- [include/gran/rhi/render_backend.hpp](../../../include/gran/rhi/render_backend.hpp)
- [include/gran/rhi/vulkan/internal/vulkan_backend_impl.hpp](../../../include/gran/rhi/vulkan/internal/vulkan_backend_impl.hpp)
- [src/app/application_frame.cpp](../../../src/app/application_frame.cpp)
- [src/app/application_graphics.cpp](../../../src/app/application_graphics.cpp)
- [src/app/application_logging.cpp](../../../src/app/application_logging.cpp)
- [src/render/renderer_textures.cpp](../../../src/render/renderer_textures.cpp)
- [src/rhi/dx11/dx11_adapter.cpp](../../../src/rhi/dx11/dx11_adapter.cpp)
- [src/rhi/dx12/dx12_adapter.cpp](../../../src/rhi/dx12/dx12_adapter.cpp)
- [src/rhi/dx12/dxgi_adapter.cpp](../../../src/rhi/dx12/dxgi_adapter.cpp)
- [src/rhi/dx12/upload_ring.cpp](../../../src/rhi/dx12/upload_ring.cpp)
- [src/rhi/render_backend.cpp](../../../src/rhi/render_backend.cpp)
- [src/rhi/vulkan/vulkan_adapter.cpp](../../../src/rhi/vulkan/vulkan_adapter.cpp)
- [src/rhi/vulkan/vulkan_device.cpp](../../../src/rhi/vulkan/vulkan_device.cpp)

### Шаги и ожидаемый результат

1. В адаптерах DXGI и Vulkan проследите получение бюджета и расхода до записи в журнал. Проверьте единицы и случай, когда драйвер не предоставляет бюджет.
2. В renderer_textures.cpp сопоставьте atlasBytes с R8-зеркалом в textureBytes. Проверьте, что атлас не прибавляется повторно.
3. Проследите учёт логических слотов при отложенном удалении и ёмкости upload-буферов каждого кадра. Освобождённый слот должен исчезать из логического счётчика.
4. В application_frame.cpp, application_graphics.cpp и application_logging.cpp проследите первый замер после восстановления и интервал между записями при отрисовке. Ожидается новый исходный замер и затем минутный интервал.

**Что прислать.** В разделе ниже укажите «прошло» или «не прошло». Для каждого замечания укажите путь оригинала, строку, условие и наблюдаемое расхождение.

### Результат

1. src/rhi/vulkan/vulkan_adapter.cpp:62 при наличии нескольких куч с флагом VK_MEMORY_HEAP_DEVICE_LOCAL_BIT (например, дискретный GPU с отдельной host-visible BAR-кучей и основной VRAM-кучей или UMA-адаптер) их бюджеты и потребление суммируются в budgetBytes и usageBytes, тогда как src/rhi/dx12/dxgi_adapter.cpp:59 запрашивает один сегмент DXGI_MEMORY_SEGMENT_GROUP_LOCAL (узел 0).
2. include/gran/rhi/render_backend.hpp:132 и include/gran/rhi/vulkan/internal/vulkan_backend_impl.hpp:85 содержат определения виртуальных методов canConfirmPresentation, lastPresentation и базового pollPresentation с телом прямо в заголовочных файлах, что нарушает правило проекта О-8 об отсутствии исполняемого кода в заголовках.

### Разбор

1. В [Vulkan](../../../src/rhi/vulkan/vulkan_adapter.cpp) `videoMemoryInfo` суммирует в байтах бюджеты и расход всех куч `DEVICE_LOCAL`. В [DXGI](../../../src/rhi/dx12/dxgi_adapter.cpp) `dxgiMemory` читает локальный сегмент узла 0. Эти области определяют драйверы. Различие описано в [контракте VideoMemoryInfo](../../../include/gran/rhi/render_backend.hpp). При недоступном запросе флаг остаётся сброшен, а логические размеры вычисляются отдельно.
2. В исходной версии на указанных координатах базовые реализации трёх методов и два переопределения Vulkan были inline. Замечание подтверждено чтением обоих заголовков. Тела перенесены в [render_backend_presentation.cpp](../../../src/rhi/render_backend_presentation.cpp) и [vulkan_presentation_state.cpp](../../../src/rhi/vulkan/vulkan_presentation_state.cpp), значения и сигнатуры сохранены.

### Дополнительная проверка

- Windows/MinGW: `make -j2 bin/gran_video_memory.exe` успешно собрал и скомпоновал пример. Запуск `.\bin\gran_video_memory.exe` из корня проекта. Пример завершился с кодом 0, `textureBytes` вырос с 0 до 262144 при создании RGBA8 256×256.
- Linux/WSL: оба новых исходника скомпилированы с заголовками Vulkan и `-DGRAN_WITH_VULKAN=1`. `g++ -r` скомпоновал объектные файлы, `nm -C --defined-only` показал все пять определений. Полный запуск Vulkan на ALT остался без проверки из-за тайм-аута SSH.
- По коду [DX11](../../../src/rhi/dx11/dx11_textures.cpp) сбрасывает живой слот, [DX12](../../../src/rhi/dx12/texture_table.cpp) переводит ресурс слота в очередь отложенного удаления, [Vulkan](../../../src/rhi/vulkan/vulkan_textures.cpp) передаёт ресурс в очередь освобождений кадра и очищает слот. После вызова счётчики живых текстур исключают освобождённый слот. DX11 берёт ёмкости буферов экземпляров и отсечений и размер констант, DX12 суммирует ёмкости колец всех кадров, Vulkan суммирует размеры буферов загрузки, экземпляров и отсечений по кадрам.
- [Рендерер](../../../src/render/renderer_textures.cpp) размещает атлас в R8-текстуре и вычисляет `atlasBytes` как квадрат стороны. Бэкенд уже включает этот слот в `textureBytes`, а журнал выводит его размер отдельно.
- [Графика](../../../src/app/application_graphics.cpp) обнуляет время очередного замера при создании бэкенда, включая восстановление. [Кадр](../../../src/app/application_frame.cpp) вызывает замер после `PRESENTED`, а [журнал](../../../src/app/application_logging.cpp) допускает следующую запись через 60 секунд от предыдущей. Путь после потери устройства проверен чтением кода.
