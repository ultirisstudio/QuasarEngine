#!/usr/bin/env python3
"""Prints the source files that clang-tidy must check for the changes since a base commit.

A source file is checked when it changed or when it includes a changed file, even through other headers. The
dependencies come from clang-scan-deps, which only runs the preprocessor. Every source file is printed when the changes
can affect all of them, such as the clang-tidy settings or the compile options, or when the dependencies cannot be read.
The paths are printed as regular expressions, one per line, for run-clang-tidy.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile

SOURCES = re.compile(r"/(engine|sandbox|editor|tests)/src/")
EVERYTHING = re.compile(r"^((.*/)?\.clang-tidy|CMakePresets\.json|cmake/.*|external/.*|\.github/workflows/clang-tidy\.yml|\.github/scripts/affected_translation_units\.py)$")


def changed_files(base):
    output = subprocess.run(["git", "diff", "--name-only", base, "HEAD"], check=True, capture_output=True, text=True).stdout
    return [line for line in output.splitlines() if line]


def read_dependencies(database, jobs):
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as file:
        json.dump(database, file)
        path = file.name
    try:
        result = subprocess.run(["clang-scan-deps-18", "-compilation-database", path, "-format=make", "-j", str(jobs)], capture_output=True, text=True)
    finally:
        os.unlink(path)
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        return None

    dependencies = {}
    text = result.stdout.replace("\\\n", " ")
    for rule in text.splitlines():
        if ":" not in rule:
            continue
        files = [os.path.realpath(name) for name in rule.split(":", 1)[1].split()]
        if files:
            dependencies[files[0]] = set(files)
    return dependencies


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", required=True, help="The build folder that holds compile_commands.json")
    parser.add_argument("--base", required=True, help="The commit to compare with")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    arguments = parser.parse_args()

    with open(os.path.join(arguments.build, "compile_commands.json"), encoding="utf-8") as file:
        database = [entry for entry in json.load(file) if SOURCES.search(entry["file"])]
    sources = sorted({os.path.realpath(os.path.join(entry["directory"], entry["file"])) for entry in database})

    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], check=True, capture_output=True, text=True).stdout.strip()
    changed = changed_files(arguments.base)
    if any(EVERYTHING.match(name) for name in changed):
        selected = sources
    else:
        changed_paths = {os.path.realpath(os.path.join(root, name)) for name in changed}
        dependencies = read_dependencies(database, arguments.jobs)
        if dependencies is None:
            selected = sources
        else:
            selected = [source for source in sources if source not in dependencies or dependencies[source] & changed_paths]

    print(f"{len(selected)} of {len(sources)} source files to check", file=sys.stderr)
    for source in selected:
        print(re.escape(source) + "$")


if __name__ == "__main__":
    main()
