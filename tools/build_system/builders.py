"""
Build execution for the Devana build system.

Handles CMake configuration, compilation, and build cleanup.
"""

import logging
import shutil
import subprocess
import time
from pathlib import Path
from typing import Optional

from .config import ProductConfig
from .utils import run_command, get_cpu_count
from .terminal_ui import (
    Color, Icon, StatusLine, Theme, Terminal,
    format_duration, Box, header
)

logger = logging.getLogger(__name__)


def setup_build(build_dir: Path, buildtype: str = "release") -> bool:
    theme = Theme.DEFAULT

    print(f"\n{theme.primary}{Icon.WRENCH} {Color.BOLD}Build Configuration{Color.RESET}")
    print(f"{theme.dim}{'─' * Terminal.width()}{Color.RESET}")
    print(f"{theme.info}{Icon.BULLET}{Color.RESET} Build type:      {theme.primary}{Color.BOLD}{buildtype.upper()}{Color.RESET}")
    print(f"{theme.info}{Icon.BULLET}{Color.RESET} Build directory: {theme.dim}{build_dir}{Color.RESET}")
    print()

    logger.info(f"Setting up build (type: {buildtype})...")
    build_dir.mkdir(parents=True, exist_ok=True)

    cmake_build_type = "Debug" if buildtype == "debug" else "Release"
    cmd = [
        "cmake",
        "-S", ".",
        "-B", str(build_dir),
        f"-DCMAKE_BUILD_TYPE={cmake_build_type}",
        "-G", "Unix Makefiles"
    ]

    result = run_command(cmd, "Configuring build with CMake")

    if result:
        StatusLine.success("Build configuration complete", theme)
    else:
        StatusLine.error("Build configuration failed", theme)

    return result


def build_project(build_dir: Path, jobs: Optional[int] = None) -> bool:
    if jobs is None:
        jobs = get_cpu_count()

    theme = Theme.DEFAULT

    print(f"\n{theme.primary}{Icon.HAMMER} {Color.BOLD}Building Project{Color.RESET}")
    print(f"{theme.dim}{'─' * Terminal.width()}{Color.RESET}")
    print(f"{theme.info}{Icon.BULLET}{Color.RESET} Parallel jobs: {theme.primary}{Color.BOLD}{jobs}{Color.RESET}")
    print()

    logger.info(f"Building with {jobs} parallel jobs...")

    start_time = time.time()
    cmd = ["cmake", "--build", str(build_dir), "--", "-j", str(jobs)]

    try:
        result = subprocess.run(cmd, capture_output=False, text=True, cwd=None)
        success = result.returncode == 0
    except subprocess.CalledProcessError:
        success = False

    elapsed = time.time() - start_time

    print()
    if success:
        StatusLine.success(
            f"Build completed in {Color.BOLD}{format_duration(elapsed)}{Color.RESET}",
            theme
        )
    else:
        StatusLine.error(f"Build failed after {format_duration(elapsed)}", theme)

    return success


def clean_build(build_dir: Path) -> bool:
    theme = Theme.DEFAULT

    if build_dir.exists():
        total_size = sum(f.stat().st_size for f in build_dir.rglob('*') if f.is_file())
        file_count = sum(1 for _ in build_dir.rglob('*') if _.is_file())

        print(f"\n{theme.warning}{Icon.BULLET}{Color.RESET} Removing build directory: {theme.dim}{build_dir}{Color.RESET}")
        print(f"{theme.dim}  └─ {file_count} files, {total_size / 1024 / 1024:.1f} MB{Color.RESET}")

        logger.info("Cleaning build directory...")
        shutil.rmtree(build_dir)
        StatusLine.success("Build directory cleaned", theme)
    else:
        StatusLine.info("Build directory doesn't exist, nothing to clean", theme)

    return True


def clean_logs(logs_dir: Path) -> bool:
    theme = Theme.DEFAULT

    if logs_dir.exists():
        log_files = list(logs_dir.rglob('*.log'))
        if log_files:
            total_size = sum(f.stat().st_size for f in log_files)
            print(f"{theme.warning}{Icon.BULLET}{Color.RESET} Removing log files: {theme.dim}{logs_dir}{Color.RESET}")
            print(f"{theme.dim}  └─ {len(log_files)} log files, {total_size / 1024:.1f} KB{Color.RESET}")

        logger.info("Cleaning log directory...")
        shutil.rmtree(logs_dir)
        logs_dir.mkdir(parents=True, exist_ok=True)
        StatusLine.success("Log directory cleaned", theme)
    else:
        StatusLine.info("Log directory doesn't exist, nothing to clean", theme)

    return True


def clean_all(build_dir: Path, logs_dir: Path) -> bool:
    theme = Theme.DEFAULT

    print(f"\n{theme.warning}{Icon.WARNING} {Color.BOLD}Cleaning All Build Artifacts{Color.RESET}")
    print(f"{theme.dim}{'─' * Terminal.width()}{Color.RESET}\n")

    clean_build(build_dir)
    clean_logs(logs_dir)

    print(f"\n{theme.success}{Icon.SPARKLES} {Color.BOLD}Clean Complete{Color.RESET}")
    print(f"{theme.dim}All build artifacts and logs have been removed{Color.RESET}\n")

    return True


def run_app(build_dir: Path, product_config: ProductConfig) -> bool:
    import platform

    possible_paths = []

    if platform.system() == "Darwin":
        possible_paths.append(
            build_dir / "bin" / f"{product_config.output_name}.app" / "Contents" / "MacOS" / product_config.output_name
        )

    possible_paths.extend([
        build_dir / "bin" / product_config.output_name,
        build_dir / product_config.output_name
    ])

    exe_path = None
    for path in possible_paths:
        if path.exists():
            exe_path = path
            break

    if not exe_path:
        logger.error("Executable not found. Tried:")
        for path in possible_paths:
            logger.error(f"   - {path}")
        logger.info("Run 'devana build' first")
        return False

    logger.info(f"Running {product_config.display_name}...")
    try:
        result = subprocess.run([str(exe_path)], cwd=build_dir.parent)
        return result.returncode == 0
    except KeyboardInterrupt:
        logger.info("\nApplication stopped")
        return True
    except Exception as e:
        logger.error(f"Failed to run: {e}")
        return False
