"""
Devana Build System v1.0 - Modular Architecture

A modern, configuration-driven build system for C++/Qt6 projects.
"""

__version__ = "1.0.0"
__author__ = "Devana Team"

# Public API exports
from .config import (
    ComponentConfig,
    ProductConfig,
    load_components_config,
    load_product_config,
    load_build_profiles,
    apply_profile
)

from .components import (
    Component,
    discover_component_sources,
    discover_idl_files
)

from .dependencies import (
    resolve_dependencies,
    check_circular_dependencies,
    auto_enable_dependencies
)

from .validation import (
    validate_component,
    validate_all_components,
    check_system_requirements
)

from .cmake_generator import (
    generate_main_cmake,
    generate_component_cmake
)

from .builders import (
    setup_build,
    build_project,
    clean_build,
    clean_logs,
    clean_all,
    run_app
)

from .packagers import (
    generate_deb_package,
    generate_rpm_package,
    generate_arch_package,
    create_package
)

from .utils import (
    run_command,
    ensure_directory,
    get_cpu_count
)

from .logger import (
    setup_logging,
    get_logger
)

from .headers import (
    add_headers_to_directory,
    add_header_to_file
)

from .idl_compiler import (
    compile_idl,
    compile_all_idl,
    IDLParser,
    CppGenerator
)

from .terminal_ui import (
    Color,
    Icon,
    Theme,
    Terminal,
    Box,
    ProgressBar,
    Spinner,
    Table,
    StatusLine,
    Banner,
    format_duration,
    format_size
)

__all__ = [
    '__version__',
    '__author__',
    'ComponentConfig',
    'ProductConfig',
    'load_components_config',
    'load_product_config',
    'load_build_profiles',
    'apply_profile',
    'Component',
    'discover_component_sources',
    'discover_idl_files',
    'resolve_dependencies',
    'check_circular_dependencies',
    'auto_enable_dependencies',
    'validate_component',
    'validate_all_components',
    'check_system_requirements',
    'generate_main_cmake',
    'generate_component_cmake',
    'setup_build',
    'build_project',
    'clean_build',
    'clean_logs',
    'clean_all',
    'run_app',
    'generate_deb_package',
    'generate_rpm_package',
    'generate_arch_package',
    'create_package',
    'run_command',
    'ensure_directory',
    'get_cpu_count',
    'setup_logging',
    'get_logger',
    'add_headers_to_directory',
    'add_header_to_file',
    'compile_idl',
    'compile_all_idl',
    'IDLParser',
    'CppGenerator',
    'Color',
    'Icon',
    'Theme',
    'Terminal',
    'Box',
    'ProgressBar',
    'Spinner',
    'Table',
    'StatusLine',
    'Banner',
    'format_duration',
    'format_size',
]
