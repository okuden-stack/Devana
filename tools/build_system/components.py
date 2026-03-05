"""
Component discovery and analysis for the Devana build system.

Scans component directories to find source files, headers, UI files, and detect Qt usage.
"""

import logging
from pathlib import Path
from typing import List
from dataclasses import dataclass, field

from .config import ComponentConfig

logger = logging.getLogger(__name__)


@dataclass
class Component:
    name: str
    path: Path
    sources: List[Path] = field(default_factory=list)
    headers: List[Path] = field(default_factory=list)
    ui_files: List[Path] = field(default_factory=list)
    qrc_files: List[Path] = field(default_factory=list)
    has_qt: bool = False
    comp_type: str = "library"
    depends_on: List[str] = field(default_factory=list)


def discover_component_sources(comp_config: ComponentConfig) -> Component:
    comp_path = comp_config.path

    if not comp_path.exists():
        logger.warning(f" Component path doesn't exist: {comp_path}")
        return Component(
            name=comp_config.name,
            path=comp_path,
            comp_type=comp_config.type,
            depends_on=comp_config.depends_on
        )

    sources = list(comp_path.rglob("*.cpp")) + \
              list(comp_path.rglob("*.cxx")) + \
              list(comp_path.rglob("*.cc"))

    headers = list(comp_path.rglob("*.h")) + \
              list(comp_path.rglob("*.hpp"))

    ui_files = list(comp_path.rglob("*.ui"))
    qrc_files = list(comp_path.rglob("*.qrc"))

    has_qt = bool(ui_files or qrc_files or any(
        'Q_OBJECT' in h.read_text(errors='ignore') or
        'QWidget' in h.read_text(errors='ignore') or
        'QObject' in h.read_text(errors='ignore') or
        'QString' in h.read_text(errors='ignore') or
        'QJson' in h.read_text(errors='ignore') or
        'QMainWindow' in h.read_text(errors='ignore') or
        'QFrame' in h.read_text(errors='ignore')
        for h in headers
    ))

    component = Component(
        name=comp_config.name,
        path=comp_path,
        sources=sources,
        headers=headers,
        ui_files=ui_files,
        qrc_files=qrc_files,
        has_qt=has_qt,
        comp_type=comp_config.type,
        depends_on=comp_config.depends_on
    )

    logger.debug(
        f"  {comp_config.name}: {len(sources)} sources, "
        f"{len(headers)} headers, Qt={has_qt}"
    )

    return component


def discover_idl_files(topics_dir: Path) -> List[Path]:
    if not topics_dir.exists():
        logger.debug(f"Topics directory not found: {topics_dir}")
        return []

    idl_files = sorted(topics_dir.glob("*.idl"))
    if idl_files:
        logger.info(f"Discovered {len(idl_files)} IDL topic files")
    return idl_files
