"""Контроль выравнивания C ABI / C ABI alignment regression."""
import subprocess
import tempfile
import unittest
from pathlib import Path

from compat_abi import HEADER, check_structs, declarations


class AbiAlignmentTest(unittest.TestCase):
    def test_packed_struct_with_unchanged_size_and_offsets(self):
        source = Path(HEADER).read_text(encoding="utf-8")
        _, baseline, _, _ = declarations(source)
        packed = source.replace("typedef struct gran_update {", "#pragma pack(push, 1)\ntypedef struct gran_update {")
        packed = packed.replace("} gran_update;", "} gran_update;\n#pragma pack(pop)")
        self.assertNotEqual(source, packed)

        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            header = directory / HEADER
            header.parent.mkdir(parents=True)
            probe = directory / "probe.cpp"
            command = ["g++", "-std=c++17", "-fsyntax-only", f"-I{directory / 'include'}", "-Iinclude"]

            for current, expected in [(source, True), (packed, False)]:
                with self.subTest(aligned=expected):
                    header.write_text(current, encoding="utf-8")
                    _, structs, _, _ = declarations(current)
                    lines = ['#include "gran/capi/gran.h"', "#include <type_traits>"]
                    check_structs(baseline, structs, True, lines)
                    probe.write_text("\n".join(lines), encoding="utf-8")
                    for unit in ["src/capi/layout.cpp", probe]:
                        result = subprocess.run([*command, unit], capture_output=True, text=True, check=False)
                        self.assertEqual(result.returncode == 0, expected, result.stderr)
                        if not expected:
                            self.assertIn("(1 == 8)", result.stderr)


if __name__ == "__main__":
    unittest.main()
