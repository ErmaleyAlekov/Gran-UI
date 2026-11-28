"""Исторические документы для проверки XSD / Historical documents for XSD validation."""
import re
import subprocess
from pathlib import Path
from xml.etree import ElementTree as ET

SCHEMA = "schemas/gran-ui.xsd"


def documents(tag, directory):
    paths = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", tag, "samples", "tests", "docs"],
                                    encoding="utf-8").splitlines()
    counts = {"samples": 0, "tests": 0, "docs": 0}
    for path in paths:
        section = path.split("/")[0]
        suffix = ".md" if section == "docs" else ".xml"
        if not path.endswith(suffix):
            continue
        raw = subprocess.check_output(["git", "show", f"{tag}:{path}"])
        if section == "docs":
            examples = doc_examples(raw.decode("utf-8-sig"))
        elif ET.fromstring(raw).tag in ("Window", "Library"):
            examples = [(0, raw)]
        else:
            continue
        for line, data in examples:
            file = directory / (path.replace("/", "_") + f"-{line}.xml")
            file.write_bytes(data)
            counts[section] += 1
            yield file
    if not all(counts.values()):
        raise ValueError(f"Empty historical corpus: {counts}")
    print(f"{tag}: {counts}", flush=True)


def doc_examples(markdown):
    for match in re.finditer(r"^\x60{3}xml[^\S\n]*\n(.*?)^\x60{3}\s*$", markdown, re.M | re.S):
        snippet = match[1].strip()
        line = markdown.count("\n", 0, match.start()) + 1
        content = re.sub(r"^<\?xml[^?]*\?>", "", snippet).strip()
        try:
            wrapper = ET.fromstring("<Fragment>" + content + "</Fragment>")
        except ET.ParseError:
            continue
        if len(wrapper) == 1 and wrapper[0].tag in ("Window", "Library"):
            yield line, snippet.encode("utf-8")
            continue
        for label, opening, closing in [("window", "<Window>", "</Window>"),
                                       ("stack", "<Window><Stack>", "</Stack></Window>"),
                                       ("resource", "<Window><Resources>", "</Resources></Window>")]:
            yield f"{line}-{label}", (opening + content + closing).encode("utf-8")


def validate_corpus(tag, directory, beresta):
    checker = directory / "validate.exe"
    subprocess.run(["g++", "-std=c++17", "-I", beresta / "include", "tools/ci/compat_validate.cpp",
                    beresta / "bin/libberesta.a", "-o", checker], check=True)
    tagged_schema = directory / "tagged.xsd"
    tagged_schema.write_bytes(subprocess.check_output(["git", "show", f"{tag}:{SCHEMA}"]))
    files = list(documents(tag, directory))
    docs = [file for file in files if file.name.startswith("docs_")]
    files = [file for file in files if file not in docs]
    accepted = []
    for offset in range(0, len(docs), 32):
        batch = docs[offset:offset + 32]
        indices = subprocess.check_output([checker, "--select", tagged_schema, *batch], text=True)
        accepted.extend(batch[int(index)] for index in indices.splitlines())
    if not accepted:
        raise ValueError("No standalone examples validated against the tagged schema")
    print(f"{tag}: {len(accepted)} standalone documentation examples", flush=True)
    files.extend(accepted)
    for offset in range(0, len(files), 32):
        subprocess.run([checker, "--validate", SCHEMA, *files[offset:offset + 32]], check=True, stdout=subprocess.DEVNULL)
