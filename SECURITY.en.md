# Security

[Русский](SECURITY.md) | English

An internal note. Gran UI loads markup, fonts and images from files and works directly with GPU memory,
so security bugs are fixed first — before new features.

## What counts as a vulnerability

- A crash, an out-of-bounds access, a leak or undefined behavior on any markup, PNG file or TrueType/TTC font.
- Bypassing the limits: image and inflate output sizes, markup depth, composite glyph nesting.
- Loading external XML resources (DTD, XInclude) while parsing markup.
- Running anything from markup other than the actions the program itself bound to C++ functions.
- Passing program objects into a `<WebView>` page script around the string bridge.
- Writing past a GPU buffer, a shader reading past its data, using a resource after its release.

XML parsing and schema bugs belong to Beresta.

## How to report

Only to the rights holder, privately: [ermaley.alekov@yandex.ru](mailto:ermaley.alekov@yandex.ru), subject "Security: Gran UI".
Give the commit, OS, compiler, GPU and driver, a minimal example (markup, font, image) and what happens.
If a vulnerability affects programs already delivered to clients, they are told together with the fix.
