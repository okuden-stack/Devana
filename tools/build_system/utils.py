"""
Shared utility functions for the Devana build system.
"""

import subprocess
import multiprocessing
import logging
from pathlib import Path
from typing import List, Optional

logger = logging.getLogger(__name__)


def run_command(
    cmd: List[str],
    description: Optional[str] = None,
    cwd: Optional[Path] = None
) -> bool:
    if description:
        logger.info(f"{description}")

    try:
        result = subprocess.run(
            cmd,
            check=True,
            capture_output=True,
            text=True,
            cwd=cwd
        )
        if result.stdout and logger.level == logging.DEBUG:
            logger.debug(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Command failed: {' '.join(cmd)}")
        if e.stderr:
            logger.error(e.stderr)
        if e.stdout and logger.level == logging.DEBUG:
            logger.debug(e.stdout)
        return False


def ensure_directory(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def get_cpu_count() -> int:
    return min(multiprocessing.cpu_count(), 8)
