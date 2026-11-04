# Gran UI — Versions, Release Branches, and Compatibility Guarantees

[Русский](versioning.md) · **English**

← [Back to the overview](../README.en.md)

## Table of Contents

1. [Version Numbering](#version-numbering)
2. [Branches](#branches)
3. [Guarantees on Release/X.Y Branches](#guarantees-on-releasexy-branches)
4. [What Is Not Covered by Guarantees](#what-is-not-covered-by-guarantees)
5. [Rules Before Version 1.0](#rules-before-version-10)
6. [Deprecation and Removal](#deprecation-and-removal)
7. [Version Checking from the Program](#version-checking-from-the-program)
8. [Release Procedure](#release-procedure)
9. [Release Support](#release-support)

## Version Numbering

Gran UI is numbered according to [Semantic Versioning 2.0.0](https://semver.org/):
**MAJOR.MINOR.PATCH**, for example `0.1.0`, `1.4.2`, `2.0.0-rc.1`.

| Part | When It Increases | Example |
|---|---|---|
| MAJOR | Incompatible changes to the public interface, markup, or C-ABI | `1.4.2` → `2.0.0` |
| MINOR | New features without breaking compatibility | `1.4.2` → `1.5.0` |
| PATCH | Only bug fixes | `1.4.2` → `1.4.3` |
| Suffix | Pre-releases: `-dev` (main branch), `-rc.N` (release candidate) | `1.5.0-rc.1` |

The number is recorded in one place — [`include/gran/gran_version.h`](../include/gran/gran_version.h).
It is taken by CMake (`project(VERSION …)`), functions `gran_version_number` / `gran_version_string`
and test `version string matches numbers`. The git tag (`vX.Y.Z`) and section in [CHANGELOG.md](../CHANGELOG.en.md) must match
the header — this is checked before each release.

Separately from the engine version, the **C-ABI version** — `GRAN_ABI_VERSION` (`gran_abi_version()`) — is maintained. It increases
only with incompatible changes to the binary interface `gran.h`: a function is removed, a signature is changed,
a structure layout or an enumeration value is changed. New ABI functions do not change it. Python, C#, and Java
wrappers compare it and refuse to work with foreign versions.

## Branches

```
main ──●──●──●──●──●──●──●──●──●──●──●──►   development, version 0.(N+1).0-dev
        \                \
         release/0.1      release/0.2       only bug fixes
          ●─v0.1.0─●─v0.1.1    ●─v0.2.0─●─v0.2.1
```

- **`main`** — development. New features appear here, and here (and only here) incompatible changes are allowed.
  The number in the header is the next minor version with the `-dev` suffix. `main` has one
  guarantee: each commit is built and tested on Windows and Linux.
- **`release/X.Y`** — release branch. Created from `main` when the minor version is ready; tags
  `vX.Y.0`, `vX.Y.1`, … are set on it. Guarantees below apply to this branch.
- Fixes are first made in `main` (if the error is there), then ported with `git cherry-pick -x` to
  supported branches `release/X.Y`. Changes only for the release branch are an exception and are marked in CHANGELOG.
- Task branches (`claude/…`, `feature/…`, `fix/…`) live until merging into `main` and are deleted afterward.

## Guarantees on Release/X.Y Branches

This is briefly recorded in the header of [`include/gran/gran_export.h`](../include/gran/gran_export.h).
If a guarantee is violated — it is a release error, and it is fixed in the next patch.

1. **Binary Compatibility of the C Interface.** A program or wrapper built with `gran_c` version X.Y.0
   works with any `gran_c` X.Y.Z without recompilation: just replace `gran_c.dll` / `libgran_c.so`.
   Functions are not removed and do not change their signature, structure layouts and enumeration values and `gran_status` codes
   do not change, `GRAN_ABI_VERSION` does not change. New functions are not added to the patch release.
2. **C++ Source Compatibility.** Declarations in `include/gran/` (except `internal/` folders) are not removed
   and not renamed, parameter types do not change. Behavior changes only where it was an error,
   and such a change is described in CHANGELOG. There is no binary compatibility of C++: `libgran` is a static
   library, after updating, the program is recompiled.
3. **Markup Compatibility.** A document that passes the `schemas/gran-ui.xsd` version X.Y.0 schema also passes it
   in X.Y.Z and builds the same element tree. Formats of configuration files (`gran-settings.xsd`)
   and resource manifest (`gran-resources.xsd`) do not change, the default theme changes only to fix errors.
4. **Messages and Error Codes.** Message catalog identifiers and error codes are not removed and do not change
   their meaning (message texts can be clarified).
5. **Requirements Do Not Increase.** Minimum versions on the branch do not increase: Windows 10 1909, MinGW-w64 GCC 11 or
   MSVC 2022 on Windows, GCC 8 and glibc 2.27 on Linux (Astra Linux SE 1.7), Vulkan 1.0, Direct3D feature level 11_0.
6. **Patch Release Composition:** bug fixes, vulnerabilities, builds on new compiler and OS versions,
   documentation. Without new features and without refactoring "along the way".
7. **Each Tag Is Checked:** `make test` on Windows and Linux, `make test-render` (D3D12, D3D11, Vulkan
   on software device), wrapper tests, build through CMake.

## What Is Not Covered by Guarantees

- Everything in the `include/gran/**/internal/` and `src/` folders — internal implementation.
- Declarations marked with `GRAN_EXPERIMENTAL` — new parts that may change in any minor version.
- Pixel-perfect frame matching between releases: fixing smoothing or font may shift pixels
  (comparison of snapshots — with tolerance, see [rendering tests](render_testing.en.md)).
- Generated files (`src/rhi/shaders_gen/`, `gen/`), examples (`examples/`, `samples/`) and development tools.
- Behavior that contradicts the documentation is an error, and its correction is not considered a break.

## Rules Before Version 1.0

While the major version is `0`, the engine is young, and according to SemVer, the minor version may contain incompatible changes.
To be able to use this, Gran UI has stricter rules:

- Inside the `release/0.Y` branch, **all** guarantees above apply — patch releases `0.Y.Z` are safe.
- Between minor versions (`0.1` → `0.2`) C++ API and markup **may** change incompatibly, but each such
  change is listed in the "Incompatible Changes" section of CHANGELOG with instructions on how to update the code.
- **C-ABI breaks only with an increase in `GRAN_ABI_VERSION`**, even before 1.0. Wrappers will see a mismatch immediately
  when loading the library, not crashing in the middle of work.
- Where possible, the old is first marked `GRAN_DEPRECATED` and deleted no earlier than the next minor version.

From version **1.0.0** incompatible changes — only in major versions.

## Deprecation and Removal

1. Replacement is added to `main`, the old declaration is marked `GRAN_DEPRECATED("use …")`
   (for markup attributes — a warning in the log at load and a mark in the documentation).
2. In CHANGELOG — a "Deprecated" section with the replacement.
3. Removal — no earlier than the next minor version before 1.0 and no earlier than the next major after 1.0;
   only in `main`, never on the release branch.

## Version Checking from the Program

At compile time — header macros:

```cpp
#include "gran/gran_version.h"

#if !GRAN_VERSION_AT_LEAST(0, 2, 0)
#error "Gran UI 0.2.0 or newer is required"
#endif
```

At runtime (for wrappers and programs that load `gran_c` as a DLL):

```c
#include "gran/capi/gran.h"

if (gran_abi_version() != GRAN_ABI_VERSION) { /* library is incompatible */ }
printf("Gran UI %s\n", gran_version_string());   /* version of the compiled DLL */
```

`GRAN_VERSION_NUMBER` and `gran_version_number()` encode the version as a number `MAJOR·1 000 000 + MINOR·1000 + PATCH`
(`1.2.3` → `1002003`), so they can be compared with usual `<`/`>`.

## Release Procedure

Release is an external action: the tag and release are published only by the repository owner.

**New Minor Version X.Y.0**

1. In `main`, tasks for the version from the [roadmap](roadmap.en.md) are completed, there are no open issues marked "blocks release".
2. Create a `release/X.Y` branch from `main`; in `gran_version.h` remove the `-dev` suffix (`X.Y.0`).
3. In `CHANGELOG.md` and `CHANGELOG.en.md` a `[X.Y.0]` section with a date: "Added", "Changed", "Incompatible Changes",
   "Deprecated", "Fixed".
4. Checks from guarantee 7. If necessary — candidate `X.Y.0-rc.1` for a week.
5. Commit, tag `vX.Y.0` on the release branch, send the branch and tag to GitHub and Gitverse.
6. In `main`, increase the number to `X.(Y+1).0-dev`.

**Patch Release X.Y.Z**

1. The fix is merged into `main` and ported to `release/X.Y` (`git cherry-pick -x`).
2. `GRAN_VERSION_PATCH` + 1, `[X.Y.Z]` section in CHANGELOG (only "Fixed" and "Security").
3. Checks, tag `vX.Y.Z`, send to both servers.

## Release Support

| Period | Which Branches Receive Fixes |
|---|---|
| Before 1.0 | Only the latest minor branch `release/0.Y` |
| After 1.0 | The two latest minor branches; security fixes — also the latest minor branch of the previous major version for 12 months after the release of the new major |

Release schedules and version compositions — in the [roadmap](roadmap.en.md#releases). About vulnerabilities — [SECURITY.md](../SECURITY.en.md).
