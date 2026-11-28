"""П.3/П.4: запуск из корня Gran / Run from the Gran project root."""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path
from xml.etree import ElementTree as ET

from compat_abi import check_abi
from compat_corpus import SCHEMA, validate_corpus
from compat_schema import compare


def baseline(branch):
    if not branch:
        raise ValueError("Missing target branch")
    release = re.fullmatch(r"release/(\d+\.\d+)", branch)
    if release:
        return f"v{release[1]}.0", True
    if branch.startswith("release/"):
        raise ValueError(f"Unknown release branch: {branch}")
    tags = subprocess.check_output(["git", "tag", "--merged", "HEAD", "--sort=-v:refname"], text=True)
    for tag in tags.splitlines():
        if re.fullmatch(r"v\d+\.\d+\.0", tag):
            return tag, False
    raise ValueError("No reachable release baseline; fetch the release tags and history")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--library", required=True, type=Path)
    parser.add_argument("--beresta", required=True, type=Path)
    args = parser.parse_args()
    tag, strict = baseline(args.branch)
    print(f"Compatibility: {args.branch} / {tag}", flush=True)
    previous = subprocess.check_output(["git", "show", f"{tag}:{SCHEMA}"])
    compare(ET.fromstring(previous), ET.parse(SCHEMA).getroot(), strict, tag == "v0.2.0" and not strict)
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        check_abi(tag, strict, args.library, directory)
        validate_corpus(tag, directory, args.beresta)


if __name__ == "__main__":
    main()
