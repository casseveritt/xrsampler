#!/usr/bin/python
import os
import re
import sys

def match_case(word, template):
    if template.isupper():
        return word.upper()
    elif template.islower():
        return word.lower()
    elif template[0].isupper():
        return word.capitalize()
    else:
        return word

def replace_in_file(filepath, pattern, replacement):
    changed = False
    output_lines = []
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    new_lines = []
    for lineno, line in enumerate(lines, 1):
        def repl(match):
            return match_case(replacement, match.group())
        new_line, n = pattern.subn(repl, line)
        if n > 0:
            changed = True
            output_lines.append(f"{filepath}:{lineno}: {new_line.rstrip()}")
        new_lines.append(new_line)
    if changed:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        for out in output_lines:
            print(out)

def rename_path(path, pattern, replacement):
    dirname = os.path.dirname(path)
    basename = os.path.basename(path)
    m = pattern.search(basename)
    if m:
        new_basename = pattern.sub(lambda m: match_case(replacement, m.group()), basename)
        new_path = os.path.join(dirname, new_basename)
        print(f"Renaming: {path} -> {new_path}")
        os.rename(path, new_path)
        return new_path
    return path

def recursive_replace_and_rename(root, search, replacement):
    pattern = re.compile(re.escape(search), re.IGNORECASE)
    # Walk bottom-up so we rename children before parents
    for dirpath, dirnames, filenames in os.walk(root, topdown=False):
        # Rename files
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            replace_in_file(filepath, pattern, replacement)
            rename_path(filepath, pattern, replacement)
        # Rename directories
        for dirname in dirnames:
            dir_fullpath = os.path.join(dirpath, dirname)
            rename_path(dir_fullpath, pattern, replacement)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python replace_recursive.py <directory> <search> <replacement>")
        sys.exit(1)
    recursive_replace_and_rename(sys.argv[1], sys.argv[2], sys.argv[3])
