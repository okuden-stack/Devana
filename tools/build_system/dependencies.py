"""
Dependency resolution for the Devana build system.

Handles topological sorting of components based on their dependencies.
"""

import logging
from typing import List, Dict, Tuple, Set

from .components import Component

logger = logging.getLogger(__name__)


def resolve_dependencies(components: List[Component]) -> Tuple[List[Component], List[str]]:
    errors = []
    comp_map = {c.name: c for c in components}
    comp_names = set(comp_map.keys())

    dependents: Dict[str, List[str]] = {c.name: [] for c in components}
    in_degree: Dict[str, int] = {c.name: 0 for c in components}

    for comp in components:
        for dep in comp.depends_on:
            if dep not in comp_names:
                errors.append(
                    f" Component '{comp.name}' depends on '{dep}' "
                    f"which is not enabled or doesn't exist"
                )
                continue
            dependents[dep].append(comp.name)
            in_degree[comp.name] += 1

    queue = [c for c in components if in_degree[c.name] == 0]
    sorted_components = []

    while queue:
        current = queue.pop(0)
        sorted_components.append(current)
        for dependent_name in dependents[current.name]:
            in_degree[dependent_name] -= 1
            if in_degree[dependent_name] == 0:
                queue.append(comp_map[dependent_name])

    if len(sorted_components) != len(components):
        remaining = [c.name for c in components if c not in sorted_components]
        errors.append(
            f"Circular dependency detected among components: {', '.join(remaining)}"
        )
        return components, errors

    logger.info(f"Resolved component dependencies")
    logger.debug(f"   Build order: {' -> '.join([c.name for c in sorted_components])}")

    return sorted_components, errors


def check_circular_dependencies(components: List[Component]) -> List[str]:
    _, errors = resolve_dependencies(components)
    return [e for e in errors if 'Circular dependency' in e]


def auto_enable_dependencies(
    components: List[Component],
    all_components: List[Component]
) -> List[Component]:
    comp_map = {c.name: c for c in all_components}
    enabled_names = {c.name for c in components}
    to_enable = set()

    for comp in components:
        for dep in comp.depends_on:
            if dep in comp_map and dep not in enabled_names:
                to_enable.add(dep)
                logger.info(f"Auto-enabling dependency: {dep} (required by {comp.name})")

    result = components.copy()
    for name in to_enable:
        result.append(comp_map[name])

    return result
