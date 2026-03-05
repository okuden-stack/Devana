"""
Package generation for the Devana build system.

Creates DEB, RPM, and Arch Linux packages for easy distribution.
"""

import logging
import shutil
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Optional

from .config import ProductConfig
from .utils import run_command, ensure_directory

logger = logging.getLogger(__name__)


def generate_deb_package(build_dir: Path, package_dir: Path, product_config: ProductConfig) -> bool:
    logger.info("Generating Debian package...")

    packaging = product_config.packaging_config

    if not packaging.get('enabled', False):
        logger.error("Packaging is not enabled in product.json")
        return False

    deb_config = packaging.get('debian', {})
    pkg_name = f"{product_config.name}_{product_config.version}_{deb_config.get('architecture', 'amd64')}"
    pkg_dir = package_dir / pkg_name

    debian_dir = ensure_directory(pkg_dir / "DEBIAN")
    bin_dir = ensure_directory(pkg_dir / "opt" / product_config.name / "bin")

    exe_src = build_dir / product_config.output_name
    exe_dst = bin_dir / product_config.output_name

    if not exe_src.exists():
        logger.error(f"Executable not found: {exe_src}")
        logger.info("Run './devana build' first")
        return False

    shutil.copy2(exe_src, exe_dst)

    control_content = f"""Package: {product_config.name}
Version: {product_config.version}
Section: {deb_config.get('section', 'misc')}
Priority: {deb_config.get('priority', 'optional')}
Architecture: {deb_config.get('architecture', 'amd64')}
Maintainer: {product_config.maintainer}
Depends: {', '.join(deb_config.get('depends', []))}
Description: {product_config.description}
 {product_config.display_name}
Homepage: {product_config.homepage}
"""
    (debian_dir / "control").write_text(control_content)

    postinst = f"""#!/bin/bash
set -e
ln -sf /opt/{product_config.name}/bin/{product_config.output_name} /usr/local/bin/{product_config.output_name}
echo "{product_config.display_name} installed successfully"
"""
    postinst_file = debian_dir / "postinst"
    postinst_file.write_text(postinst)
    postinst_file.chmod(0o755)

    prerm = f"""#!/bin/bash
set -e
rm -f /usr/local/bin/{product_config.output_name}
"""
    prerm_file = debian_dir / "prerm"
    prerm_file.write_text(prerm)
    prerm_file.chmod(0o755)

    deb_file = package_dir / f"{pkg_name}.deb"
    cmd = ["dpkg-deb", "--build", str(pkg_dir), str(deb_file)]
    if run_command(cmd, "Creating Debian package"):
        logger.info(f"Debian package created: {deb_file}")
        return True
    return False


def generate_rpm_package(build_dir: Path, package_dir: Path, product_config: ProductConfig) -> bool:
    logger.info("Generating RPM package...")

    packaging = product_config.packaging_config

    if not packaging.get('enabled', False):
        logger.error("Packaging is not enabled in product.json")
        return False

    rpm_config = packaging.get('rpm', {})

    if shutil.which("rpmbuild") is None:
        logger.error("rpmbuild not found - install rpm-build package")
        return False

    rpm_root = package_dir / "rpm"
    for d in ['BUILD', 'RPMS', 'SOURCES', 'SPECS', 'SRPMS']:
        ensure_directory(rpm_root / d)

    spec_content = f"""%define _topdir {rpm_root}

Name:           {product_config.name}
Version:        {product_config.version}
Release:        1%{{?dist}}
Summary:        {product_config.description}

License:        {product_config.license}
URL:            {product_config.homepage}

Requires:       {', '.join(rpm_config.get('requires', []))}

%description
{product_config.description}

%install
mkdir -p %{{buildroot}}/opt/{product_config.name}/bin
cp {build_dir / product_config.output_name} %{{buildroot}}/opt/{product_config.name}/bin/
mkdir -p %{{buildroot}}/usr/local/bin
ln -s /opt/{product_config.name}/bin/{product_config.output_name} %{{buildroot}}/usr/local/bin/{product_config.output_name}

%files
/opt/{product_config.name}/bin/{product_config.output_name}
/usr/local/bin/{product_config.output_name}

%changelog
* {datetime.now().strftime('%a %b %d %Y')} {product_config.vendor}
- Release {product_config.version}
"""

    spec_file = rpm_root / "SPECS" / f"{product_config.name}.spec"
    spec_file.write_text(spec_content)

    cmd = ["rpmbuild", "-bb", str(spec_file)]
    if run_command(cmd, "Creating RPM package"):
        logger.info(f"RPM package created in {rpm_root / 'RPMS'}")
        return True
    return False


def generate_arch_package(build_dir: Path, package_dir: Path, product_config: ProductConfig) -> bool:
    logger.info("Generating Arch Linux package...")

    packaging = product_config.packaging_config

    if not packaging.get('enabled', False):
        logger.error("Packaging is not enabled in product.json")
        return False

    arch_config = packaging.get('arch', {})

    if shutil.which("makepkg") is None:
        logger.error("makepkg not found - are you on Arch Linux?")
        return False

    pkg_dir = ensure_directory(package_dir / "arch")

    pkgbuild_content = f"""# Maintainer: {product_config.vendor}

pkgname={product_config.name}
pkgver={product_config.version}
pkgrel=1
pkgdesc="{product_config.description}"
arch=('{arch_config.get('architecture', 'x86_64')}')
url="{product_config.homepage}"
license=('{product_config.license}')
depends=({' '.join(f"'{d}'" for d in arch_config.get('depends', []))})

package() {{
    install -Dm755 "{build_dir / product_config.output_name}" "$pkgdir/opt/{product_config.name}/bin/{product_config.output_name}"
    install -dm755 "$pkgdir/usr/local/bin"
    ln -s "/opt/{product_config.name}/bin/{product_config.output_name}" "$pkgdir/usr/local/bin/{product_config.output_name}"
}}
"""

    (pkg_dir / "PKGBUILD").write_text(pkgbuild_content)

    cmd = ["makepkg", "-f"]
    if run_command(cmd, "Creating Arch package", cwd=pkg_dir):
        logger.info(f"Arch package created in {pkg_dir}")
        return True
    return False


def create_package(format_type: str, build_dir: Path, package_dir: Path, product_config: ProductConfig) -> bool:
    ensure_directory(package_dir)

    if format_type == 'deb':
        return generate_deb_package(build_dir, package_dir, product_config)
    elif format_type == 'rpm':
        return generate_rpm_package(build_dir, package_dir, product_config)
    elif format_type == 'arch':
        return generate_arch_package(build_dir, package_dir, product_config)
    else:
        logger.error(f"Unknown package format: {format_type}")
        return False
