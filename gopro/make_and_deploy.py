#!/usr/bin/env python3
"""
copy_and_patch.py

A command-line utility that:
  1) Copies files from a source directory to a target directory (with a default target).
  2) Optionally updates only the [patch] section of a TOML-like file using another input TOML-like file.
"""
import argparse
import os
import re
import shutil
import subprocess
from typing import Optional, Tuple

DEFAULT_SOURCE_DIR = os.path.join(os.getcwd(), "patches")
DEFAULT_TARGET_DIR = os.path.join(os.getcwd(), "../../gopro-sdk-quik-engine/sxbundle/bundle/build_scripts/patches/ffmpeg")
DEFAULT_TARGET_TOML = os.path.join(os.getcwd(), "../../gopro-sdk-quik-engine/sxbundle/bundle/recipes/ffmpeg/ffmpeg.toml")

def clean_target_dir(target_dir: str) -> None:
    """Remove all contents from target directory."""
    if os.path.exists(target_dir):
        for item in os.listdir(target_dir):
            item_path = os.path.join(target_dir, item)
            if os.path.isfile(item_path):
                os.remove(item_path)
                print(f"Removed: {item_path}")
            elif os.path.isdir(item_path):
                shutil.rmtree(item_path)
                print(f"Removed directory: {item_path}")

def copy_files_flat(source_dir: str, target_dir: str) -> int:
    """Copy only files from the top level of source_dir to target_dir."""
    count = 0
    for item in os.listdir(source_dir):
        s_path = os.path.join(source_dir, item)
        t_path = os.path.join(target_dir, item)
        if os.path.isfile(s_path):
            shutil.copy2(s_path, t_path)
            print(f"Copied: {s_path} -> {t_path}")
            count += 1
    return count

def copy_files_recursive(source_dir: str, target_dir: str) -> int:
    """Recursively copy files from source_dir to target_dir, preserving directory structure."""
    count = 0
    for root, dirs, files in os.walk(source_dir):
        rel_root = os.path.relpath(root, source_dir)
        dest_root = target_dir if rel_root == '.' else os.path.join(target_dir, rel_root)
        os.makedirs(dest_root, exist_ok=True)
        for fname in files:
            s_path = os.path.join(root, fname)
            t_path = os.path.join(dest_root, fname)
            shutil.copy2(s_path, t_path)
            print(f"Copied: {s_path} -> {t_path}")
            count += 1
    return count

# --- TOML [[patches]] section utilities ---
PATCH_HEADER_PATTERN = re.compile(r"^\s*\[\[patches\]\]\s*$", re.MULTILINE)
ANY_HEADER_PATTERN   = re.compile(r"^\s*\[.+?\]\s*$", re.MULTILINE)

def extract_all_patch_sections(content: str) -> list:
    """Extract all [[patches]] sections from content, each with its header."""
    patches = []
    pos = 0
    while True:
        m = PATCH_HEADER_PATTERN.search(content, pos)
        if not m:
            break
        start = m.start()
        # Find next header or EOF
        next_m = ANY_HEADER_PATTERN.search(content, m.end())
        end = next_m.start() if next_m else len(content)
        patch_section = content[start:end].rstrip()
        patches.append(patch_section)
        print(f"Extracted patch section:\n{patch_section}\n")
        pos = end
    return patches

def remove_all_patch_sections(target_text: str) -> str:
    """Remove all [[patches]] sections from target_text."""
    pos = 0
    result = target_text
    while True:
        m = PATCH_HEADER_PATTERN.search(result, pos)
        if not m:
            break
        start = m.start()
        # Find next header or EOF
        next_m = ANY_HEADER_PATTERN.search(result, m.end())
        end = next_m.start() if next_m else len(result)
        # Remove the patch section and any trailing newlines
        result = result[:start] + result[end:].lstrip("\n")
    return result.rstrip() + "\n"

def append_patch_sections(target_text: str, patches: list) -> str:
    """Append all patch sections to the end of target_text."""
    if not patches:
        return target_text
    text = target_text.rstrip() + "\n"
    for patch in patches:
        text += patch + "\n"
    return text

def patch_toml(input_text: str, target_toml_path: str) -> None:
    """Extract all [[patches]] from input TOML and replace all patches in target TOML."""
    if not os.path.exists(target_toml_path):
        raise FileNotFoundError(f"Patch target file not found: {target_toml_path}")

    with open(target_toml_path, 'r', encoding='utf-8') as f:
        target_text = f.read()

    # Extract all patch sections from input
    patches = extract_all_patch_sections(input_text)
    if not patches:
        print("Warning: No [[patches]] sections found in input")
        return

    # Remove all existing patches from target
    target_text_cleaned = remove_all_patch_sections(target_text)

    # Append all extracted patches to target
    updated_text = append_patch_sections(target_text_cleaned, patches)

    with open(target_toml_path, 'w', encoding='utf-8') as f:
        f.write(updated_text)

    print(f"Updated {len(patches)} patch section(s) in: {target_toml_path}")



def run_make(target=None):
    """
    Run `make` command from Python.
    :param target: Optional make target (e.g., 'build', 'clean')
    """
    command = ["make"]
    if target:
        command.append(target)

    try:
        result = subprocess.run(command, check=True, text=True, capture_output=True)
        print("Output:\n", result.stdout)
        if result.stderr:
            print("Errors:\n", result.stderr)
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Make failed with exit code {e.returncode}")
        print("Output:\n", e.stdout)
        print("Errors:\n", e.stderr)

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Copy files from a source directory to a target directory (default: ./copied_files). "
            "Optionally update the [patch] section of a TOML-like file using another input TOML-like file."
        )
    )
    parser.add_argument("source", nargs="?", default=DEFAULT_SOURCE_DIR, help="Path to the source directory whose files will be copied")
    parser.add_argument("target", nargs="?", default=DEFAULT_TARGET_DIR,
                        help=f"Path to the target directory (default: {DEFAULT_TARGET_DIR})")
    parser.add_argument("--recursive", action="store_true",
                        help="Copy files recursively, preserving directory structure")
    parser.add_argument("--patch-input",  dest="patch_input", nargs="?", default=None,
                        help="Path to the input TOML-like file containing the [patch] section")
    parser.add_argument("--patch-target", dest="patch_target", nargs="?", default=DEFAULT_TARGET_TOML,
                        help="Path to the target TOML-like file whose [patch] section will be replaced")

    args = parser.parse_args()

    source_dir = args.source
    target_dir = args.target

    run_make("clean")
    toml_generated = run_make()

    if not os.path.isdir(source_dir):
        raise NotADirectoryError(f"Source directory does not exist or is not a directory: {source_dir}")

    os.makedirs(target_dir, exist_ok=True)
    print(f"Target directory: {target_dir}")

    if "ffmpeg" in target_dir:
        # Clean target directory before copying
        clean_target_dir(target_dir)

    # Copy files
    if args.recursive:
        copied = copy_files_recursive(source_dir, target_dir)
    else:
        copied = copy_files_flat(source_dir, target_dir)
    print(f"Total files copied: {copied}")

    # Patch TOML if both arguments provided
    if args.patch_target:
        if args.patch_input is None:
            input_text = toml_generated
        else:
            with open(args.patch_input, 'r', encoding='utf-8') as f:
                input_text = f.read()
            if not os.path.exists(args.patch_input):
                raise FileNotFoundError(f"Patch input file not found: {args.patch_input}")
        patch_toml(input_text, args.patch_target)
    elif args.patch_input or args.patch_target:
        print("Warning: To patch TOML, provide both --patch-input and --patch-target.")

if __name__ == "__main__":
    main()
