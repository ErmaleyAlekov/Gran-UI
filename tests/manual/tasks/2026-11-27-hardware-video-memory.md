# Т-252. Показания видеопамяти на физическом GPU

1. На Windows из корня проекта выполните `make -j2 bin/gran_video_memory.exe`, затем `.\bin\gran_video_memory.exe`. В настроенной сборке CMake для Linux соберите цель `gran_video_memory` и запустите одноимённый исполняемый файл. Запишите адаптер, драйвер, ОС и обе строки вывода. При создании RGBA8 256×256 прирост `textureBytes` должен составить 262144 байта. Бюджет и занятость драйвера меняются независимо от этого счётчика.
2. Запустите приложение с настроенным `AppOptions::appName` и текстом в окне. После первого показанного кадра найдите запись `gpu` с `atlasBytes`, `textureBytes` и `bufferBytes`. Размер атласа входит в общий размер текстур. При доступном бюджете присутствуют `budgetBytes` и `usageBytes`.
3. Продолжайте отрисовку не менее минуты (например, прокручивайте содержимое). Приложите две записи журнала с отметками времени. Если драйвер не предоставляет бюджет, проверьте, что логические счётчики всё равно доступны.

## Результат
    1.  budgetAvailable=true budgetBytes=16168480768 usageBytes=12455936 textureBytes=0 bufferBytes=0
        budgetAvailable=true budgetBytes=16168480768 usageBytes=12718080 textureBytes=262144 bufferBytes=0

### Автоматическая проверка на стенде, 2026-11-27

Прошло. ALT Workstation 11.2, Vulkan, NVIDIA GeForce RTX 3060, драйвер NVIDIA 595.84 по vulkaninfo. Пример зафиксировал рост textureBytes с 0 до 262144 при создании RGBA8 256×256.

Галерея открыла samples/animations.xml с параметрами `--vulkan --exit-after 66` и завершилась с кодом 0. Первый замер появился через 0,301 с после записи о запуске. Атлас занимает 1048576 байт при общем объёме текстур 1081344 байта. Следующий замер появился через 60,007 с. Драйвер предоставил бюджет в обоих замерах.

```text
2026-11-27 20:35:24.842 | INFO    | 6bf4 | gpu | atlasBytes=1048576 textureBytes=1081344 bufferBytes=1212416 budgetBytes=8127447040 usageBytes=8585216
2026-11-27 20:36:24.849 | INFO    | 6bf4 | gpu | atlasBytes=1048576 textureBytes=1081344 bufferBytes=1409024 budgetBytes=8125349888 usageBytes=8585216
```

Журнал на gran-alt: `~/gran-stand/t252.JoiMM7/check.NXOMzL/GranGallery/logs/GranGallery.log`. Задача закрыта.
