# Security Policy

[Русский](SECURITY.md) | English

Gran UI loads markup, fonts and images from files and works directly with GPU memory, so security bugs matter
to us most of all.

## Supported Versions

There are no releases yet: the engine is under development, fixes go into the `main` branch.

| Version | Support |
|---|---|
| `main` | ✅ security patches |

## How to Report a Vulnerability

**Do not open a public issue or post in discussions** — this will make the vulnerability known before it is fixed.

Report privately through GitHub: [Security → Report a vulnerability](https://github.com/ErmaleyAlekov/Gran-UI/security/advisories/new). If you do not have access to GitHub — by email to [ermaley.alekov@yandex.ru](mailto:ermaley.alekov@yandex.ru) with the subject "Security: Gran UI".

In your message, specify:

- the engine commit or version, OS, compiler, GPU and driver version;
- the minimal markup, font or image on which the bug shows up, and what happens (a crash, a hang, memory growth, reading a foreign file, a device loss, a D3D12 debug layer error);
- if available, an assessment of the consequences.

## What Happens Next

- Acknowledgment of receipt — within 7 days.
- Assessment and a fix plan — within 30 days; we will notify you of the timeline in advance.
- The fix is released on GitHub and Gitverse; the reporter is credited in the GitHub Security Advisory (if they wish).

## What is Considered a Vulnerability

- A crash, out-of-bounds memory access, leak or undefined behavior on any markup, PNG file or TrueType/TTC font.
- Bypassing the limits: image and inflate output sizes, markup depth, composite glyph nesting.
- Loading external XML resources (DTD, XInclude) while parsing markup.
- Running anything from markup other than the actions the program itself has bound to C++ functions.
- Passing program objects into a `<WebView>` page script around the string bridge.
- Writing past a GPU buffer, a shader reading past its data, using a resource after it was released.

Not a vulnerability: rendering and layout bugs with no security impact, refusing to load markup that fails the schema — report these in regular issues. XML parsing and schema bugs belong to [Beresta](https://github.com/ErmaleyAlekov/beresta/blob/main/SECURITY.en.md).
