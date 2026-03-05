"""
File header management for source files.

Adds consistent copyright and documentation headers to C++ source files.
"""

import logging
from pathlib import Path
from datetime import datetime
from typing import Tuple

logger = logging.getLogger(__name__)

HEADER_TEMPLATE = """/**
 * @file    {filename}
 * @brief   {brief}.
 * @author  {author}
 * @date    {date}
 *
 * This file is part of the {project} project.
 * {description}
 */
"""


def camel_to_words(name: str) -> str:
    result = []
    word = ""
    for char in name:
        if char.isupper() and word:
            result.append(word)
            word = char
        else:
            word += char
    if word:
        result.append(word)
    return " ".join(result)


def guess_description(filepath: Path, src_dir: Path) -> Tuple[str, str]:
    name = filepath.stem
    readable = camel_to_words(name).capitalize()

    try:
        relative = filepath.relative_to(src_dir)
        category = relative.parts[0] if len(relative.parts) > 1 else "General"
    except ValueError:
        category = "General"

    brief = f"{readable} module in {category}"
    desc = f"Defines and implements the {readable.lower()} functionality for the {category} component."

    return brief, desc


def has_existing_header(lines: list) -> bool:
    return any("@file" in line or "/**" in line for line in lines[:10])


def has_include_guard(lines: list) -> bool:
    return any(
        '#pragma once' in line or
        '#ifndef' in line or
        '#define' in line
        for line in lines[:10]
    )


def add_header_to_file(
    filepath: Path,
    src_dir: Path,
    author: str = "AK",
    project: str = "Devana"
) -> bool:
    try:
        with open(filepath, "r") as f:
            lines = f.readlines()

        if has_existing_header(lines):
            logger.debug(f" Skipped (already has header): {filepath.name}")
            return False

        filename = filepath.name
        brief, desc = guess_description(filepath, src_dir)
        date = datetime.today().strftime("%Y-%m-%d")

        header = HEADER_TEMPLATE.format(
            filename=filename,
            brief=brief,
            author=author,
            date=date,
            project=project,
            description=desc
        )

        new_lines = header.splitlines(keepends=True) + ["\n"]

        if filepath.suffix in ['.h', '.hpp']:
            if not has_include_guard(lines):
                new_lines.append("#pragma once\n\n")

        new_lines += lines

        with open(filepath, "w") as f:
            f.writelines(new_lines)

        logger.info(f"Added header: {filepath.name}")
        return True

    except Exception as e:
        logger.error(f"Failed to update {filepath.name}: {e}")
        return False


def add_headers_to_directory(
    src_dir: Path,
    author: str = "AK",
    project: str = "Devana",
    extensions: tuple = ('.cpp', '.hpp', '.h')
) -> Tuple[int, int]:
    logger.info(f"Adding file headers to source files...")
    logger.info(f"   Source directory: {src_dir}")
    logger.info(f"   Extensions: {', '.join(extensions)}")

    updated = 0
    skipped = 0

    for ext in extensions:
        for filepath in src_dir.rglob(f"*{ext}"):
            if add_header_to_file(filepath, src_dir, author, project):
                updated += 1
            else:
                skipped += 1

    logger.info(f"\nSummary:")
    logger.info(f"   Headers added: {updated}")
    logger.info(f"    Files skipped: {skipped}")
    logger.info(f"   Total files:   {updated + skipped}")

    return updated, skipped
