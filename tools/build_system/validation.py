"""
Component validation and system requirements checking for Devana.
"""

import logging
import shutil
import subprocess
from typing import List, Set
from pathlib import Path

from .components import Component

logger = logging.getLogger(__name__)


def validate_component(comp: Component, all_component_names: Set[str]) -> List[str]:
    errors = []

    if not comp.path.exists():
        errors.append(f"Component directory not found: {comp.path}")
        return errors

    if comp.comp_type == "library" and not comp.sources and not comp.headers:
        errors.append(f"Library component '{comp.name}' has no C++ source or header files")

    for dep in comp.depends_on:
        if dep not in all_component_names:
            errors.append(f"Component '{comp.name}' depends on unknown component '{dep}'")

    if comp.ui_files and not comp.has_qt:
        errors.append(
            f"Component '{comp.name}' has .ui files but Qt was not detected. "
            "Add Q_OBJECT to a header or check Qt dependencies."
        )

    if comp.qrc_files:
        for qrc in comp.qrc_files:
            if not qrc.exists():
                errors.append(f"QRC file not found: {qrc}")

    return errors


def validate_all_components(components: List[Component]) -> bool:
    logger.info("Validating components...")

    all_component_names = {c.name for c in components}
    all_errors = []

    for comp in components:
        comp_errors = validate_component(comp, all_component_names)
        if comp_errors:
            all_errors.extend([f"  • {comp.name}: {err}" for err in comp_errors])

    if all_errors:
        logger.error("Component validation failed:")
        for error in all_errors:
            logger.error(error)
        return False

    logger.info(f"All {len(components)} components validated successfully")
    return True


def check_system_requirements() -> bool:
    logger.info("Checking system requirements...")
    print("\n" + "=" * 70)
    print("  System Requirements Check")
    print("=" * 70)

    requirements = {
        "cmake": {
            "description": "CMake build system",
            "install_debian": "sudo apt install cmake",
            "install_fedora": "sudo dnf install cmake",
            "install_arch": "sudo pacman -S cmake",
            "install_macos": "brew install cmake"
        },
        "g++": {
            "description": "C++ compiler (GCC)",
            "install_debian": "sudo apt install build-essential",
            "install_fedora": "sudo dnf install gcc-c++",
            "install_arch": "sudo pacman -S gcc",
            "install_macos": "xcode-select --install"
        },
        "qmake": {
            "description": "Qt development tools",
            "install_debian": "sudo apt install qt6-base-dev",
            "install_fedora": "sudo dnf install qt6-qtbase-devel",
            "install_arch": "sudo pacman -S qt6-base",
            "install_macos": "brew install qt@6"
        },
        "python3": {
            "description": "Python 3 interpreter",
            "install_debian": "sudo apt install python3",
            "install_fedora": "sudo dnf install python3",
            "install_arch": "sudo pacman -S python",
            "install_macos": "brew install python3"
        }
    }

    all_ok = True
    for tool, info in requirements.items():
        found = shutil.which(tool) is not None
        status = "" if found else ""
        print(f"\n{status} {info['description']:30}", end="")

        if found:
            try:
                result = subprocess.run(
                    [tool, "--version"],
                    capture_output=True,
                    text=True,
                    timeout=2
                )
                version = result.stdout.split('\n')[0][:40]
                print(f" ({version})")
            except:
                print(" (installed)")
        else:
            print(" MISSING")
            print(f"   Debian/Ubuntu: {info['install_debian']}")
            print(f"   Fedora/RHEL:   {info['install_fedora']}")
            print(f"   Arch Linux:    {info['install_arch']}")
            print(f"   macOS:         {info['install_macos']}")
            all_ok = False

    print("\n" + "=" * 70)

    if all_ok:
        logger.info("All required tools are installed")
    else:
        logger.error("Some required tools are missing")
        logger.info("Install missing tools and run './devana doctor' again")

    return all_ok
