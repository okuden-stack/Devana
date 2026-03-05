"""
Configuration management for the Devana build system.

Handles loading and parsing of components.json and product.json configuration files.
"""

import json
import logging
from pathlib import Path
from typing import List, Dict
from dataclasses import dataclass

logger = logging.getLogger(__name__)


@dataclass
class ComponentConfig:
    name: str
    path: Path
    enabled: bool
    type: str
    required: bool
    description: str
    depends_on: List[str]


@dataclass
class ProductConfig:
    name: str
    display_name: str
    version: str
    description: str
    homepage: str
    license: str
    vendor: str
    maintainer: str
    copyright: str
    output_name: str
    cpp_standard: str
    meson_version: str
    default_buildtype: str
    qt6_modules: List[str]
    openssl_required: bool
    idl_enabled: bool
    idl_source_dir: str
    idl_compiler: str
    packaging_enabled: bool
    install_prefix: str
    packaging_config: Dict


def load_components_config(config_path: Path) -> List[ComponentConfig]:
    if not config_path.exists():
        logger.error(f"Components configuration not found: {config_path}")
        raise FileNotFoundError(f"Missing {config_path}")

    try:
        with open(config_path, 'r') as f:
            data = json.load(f)

        root_dir = config_path.parent.parent.parent  # tools/config/ -> tools/ -> root
        components = []

        for comp_data in data.get('components', []):
            if comp_data.get('enabled', True):
                components.append(ComponentConfig(
                    name=comp_data['name'],
                    path=root_dir / comp_data['path'],
                    enabled=comp_data.get('enabled', True),
                    type=comp_data.get('type', 'library'),
                    required=comp_data.get('required', False),
                    description=comp_data.get('description', ''),
                    depends_on=comp_data.get('depends_on', [])
                ))

        logger.info(f"Loaded {len(components)} enabled components from configuration")
        return components

    except json.JSONDecodeError as e:
        logger.error(f"Invalid JSON in {config_path}: {e}")
        raise
    except KeyError as e:
        logger.error(f"Missing required field in components.json: {e}")
        raise


def load_product_config(config_path: Path) -> ProductConfig:
    if not config_path.exists():
        logger.error(f"Product configuration not found: {config_path}")
        raise FileNotFoundError(f"Missing {config_path}")

    try:
        with open(config_path, 'r') as f:
            data = json.load(f)

        product = data['product']
        build = data['build']
        deps = data.get('dependencies', {})
        idl = data.get('idl', {})
        packaging = data.get('packaging', {})

        config = ProductConfig(
            name=product['name'],
            display_name=product['display_name'],
            version=product['version'],
            description=product['description'],
            homepage=product.get('homepage', ''),
            license=product.get('license', 'Proprietary'),
            vendor=product.get('vendor', ''),
            maintainer=product.get('maintainer', ''),
            copyright=product.get('copyright', ''),
            output_name=build['output_name'],
            cpp_standard=build['cpp_standard'],
            meson_version=build['meson_version'],
            default_buildtype=build['default_buildtype'],
            qt6_modules=deps.get('qt6', {}).get('modules', []),
            openssl_required=deps.get('openssl', {}).get('required', False),
            idl_enabled=idl.get('enabled', False),
            idl_source_dir=idl.get('source_dir', 'topics'),
            idl_compiler=idl.get('compiler', 'tools/idl_compiler.py'),
            packaging_enabled=packaging.get('enabled', False),
            install_prefix=packaging.get('install_prefix', '/opt/devana'),
            packaging_config=packaging
        )

        logger.info(f"Loaded product configuration: {config.display_name} v{config.version}")
        return config

    except (json.JSONDecodeError, KeyError) as e:
        logger.error(f"Error loading product config: {e}")
        raise


def load_build_profiles(config_path: Path) -> Dict[str, Dict]:
    try:
        with open(config_path, 'r') as f:
            data = json.load(f)
        return data.get('profiles', {})
    except Exception as e:
        logger.warning(f" Could not load build profiles: {e}")
        return {}


def apply_profile(
    profile_name: str,
    all_components: List[ComponentConfig],
    config_path: Path
) -> List[ComponentConfig]:
    profiles = load_build_profiles(config_path)

    if profile_name not in profiles:
        logger.error(f"Unknown profile: {profile_name}")
        logger.info(f"Available profiles: {', '.join(profiles.keys())}")
        return all_components

    profile = profiles[profile_name]
    enabled_names = set(profile.get('components', []))
    filtered = [c for c in all_components if c.name in enabled_names]

    logger.info(f"Applied profile '{profile_name}': {len(filtered)} components")
    logger.info(f"   {profile.get('description', '')}")

    return filtered
