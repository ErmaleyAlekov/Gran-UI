"""Проверка публичного C ABI / Public C ABI check."""
import re
import subprocess
from pathlib import Path

HEADER = "include/gran/capi/gran.h"


def declarations(source):
    clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
    functions = re.findall(r"\bGRAN_API\s+([^;]+?)\b(gran_\w+)\s*\(([^;]*)\)\s*;", clean)
    callbacks = re.findall(r"typedef\s+([^;{}]+?)\(\s*\*\s*(gran_\w+)\s*\)\s*\(([^;]*)\);", clean)
    structs = re.findall(r"typedef struct (gran_\w+)\s*\{([^}]+)\}", clean)
    types = {name: f"{result.strip()} (*)({args})" for result, name, args in functions + callbacks}
    for scalar, name in re.findall(r"typedef\s+(\w+)\s+(gran_\w+)\s*;", clean):
        types[name] = scalar
    return types, dict(structs), {name for _, name, _ in functions}, clean


def exports(library):
    if library.suffix.lower() == ".dll":
        listing = subprocess.check_output(["objdump", "-p", library], text=True)
        table = listing.split("[Ordinal/Name Pointer] Table", 1)[1].split("\n\n", 1)[0]
        return set(re.findall(r"\b(gran_\w+)\s*$", table, re.M))
    listing = subprocess.check_output(["nm", "-D", "--defined-only", library], text=True)
    return set(re.findall(r"\b(gran_\w+)(?:@@?\S+)?$", listing, re.M))


def check_structs(old, new, strict, lines):
    for name, body in old.items():
        lines.append(f"struct baseline_{name} {{{body}}};")
        lines.append(f"static_assert(alignof({name}) == alignof(baseline_{name}));")
        extendible = not strict and name == "gran_input_event"
        if not extendible:
            lines.append(f"static_assert(sizeof({name}) == sizeof(baseline_{name}));")
        fields = re.findall(r"(\w+)\s*;", body)
        current = re.findall(r"(\w+)\s*;", new[name])
        if (current[:len(fields)] if extendible else current) != fields:
            raise ValueError(f"ABI fields: {name}")
        for field in fields:
            lines.append(f"static_assert(offsetof({name}, {field}) == offsetof(baseline_{name}, {field}));")
            lines.append(f"static_assert(std::is_same_v<decltype({name}::{field}), decltype(baseline_{name}::{field})>);")


def constants(tag, old, new, strict):
    pattern = r"\b(GRAN_\w+)\s*=\s*([^,}]+)"
    expected = dict(re.findall(pattern, old))
    if strict and expected.keys() != dict(re.findall(pattern, new)).keys():
        raise ValueError("ABI enum declarations changed")
    version = subprocess.check_output(["git", "show", f"{tag}:include/gran/gran_version.h"], encoding="utf-8")
    expected["GRAN_ABI_VERSION"] = re.search(r"^#define GRAN_ABI_VERSION\s+(\d+)$", version, re.M)[1]
    return [f"static_assert({name} == ({value}));" for name, value in expected.items()]


def check_abi(tag, strict, library, directory):
    previous = subprocess.check_output(["git", "show", f"{tag}:{HEADER}"], encoding="utf-8")
    old, structs, symbols, clean = declarations(previous)
    new, current, public, current_clean = declarations(Path(HEADER).read_text(encoding="utf-8"))
    actual = exports(library)
    if public != actual:
        raise ValueError(f"ABI exports: {sorted(public ^ actual)}")
    for before, after in [(set(old), set(new)), (set(structs), set(current)), (symbols, actual)]:
        difference = before ^ after if strict else before - after
        if difference:
            raise ValueError(f"ABI declarations: {sorted(difference)}")
    lines = ['#include "gran/capi/gran.h"', '#include <type_traits>']
    for name, signature in old.items():
        target = f"decltype(&{name})" if name in symbols else name
        lines.append(f"static_assert(std::is_same_v<{target}, {signature}>, \"{name}\");")
    lines.extend(constants(tag, clean, current_clean, strict))
    check_structs(structs, current, strict, lines)
    probe = directory / "abi.cpp"
    probe.write_text("\n".join(lines), encoding="utf-8")
    subprocess.run(["g++", "-std=c++17", "-fsyntax-only", "-Iinclude", probe, "src/capi/layout.cpp"], check=True)
