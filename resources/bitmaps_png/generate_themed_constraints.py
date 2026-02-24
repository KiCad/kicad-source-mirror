#!/usr/bin/env python3
"""
Generate themed (light/dark) versions of constraint SVGs for KiCad DRC Rule Editor.

Color palette from https://dev-docs.kicad.org/en/rules-guidelines/icon-design/
"""

import os
import re
import shutil
from pathlib import Path

# Color transformations from light to dark theme (case-insensitive)
LIGHT_TO_DARK_COLORS = {
    '#545454': '#ded3dd',  # Primary Gray stroke
    '#606060': '#606060',  # Dense fills - stays same
    '#b9b9b9': '#e0e0e0',  # Accent Grey
    '#1a81c4': '#42b8eb',  # Primary Blue
    '#39b4ea': '#1a81c4',  # Accent Blue (swap)
    '#bf2641': '#f2647e',  # Primary Red
    '#f3f3f3': '#545454',  # Area Fill
    '#dcdcdc': '#545454',  # Light boundary lines
    '#000000': '#ded3dd',  # Black strokes become light
    '#000fe5': '#42b8eb',  # Blue arrows
}

# Files to process - mapping from source name to output name
# Format: 'source_filename.svg': 'output-name.svg'
CONSTRAINT_FILES = {
    # Specialized panels
    'absolute-lenght-2.svg': 'absolute-length-2.svg',
    'allowed-orientation.svg': 'allowed-orientation.svg',
    'vias-under-SMD.svg': 'vias-under-smd.svg',
    'minimum_text_height_and_thickness.svg': 'minimum-text-height-and-thickness.svg',
    'permitted-layers.svg': 'permitted-layers.svg',
    'via-style.svg': 'via-style.svg',

    # Numeric input constraints
    'Basic clearance.svg': 'basic-clearance.svg',
    'board-outline-clearance.svg': 'board-outline-clearance.svg',
    'minimum_clearance.svg': 'minimum-clearance.svg',
    'minimum_connection_width.svg': 'minimum-connection-width.svg',
    'minimum_track_width.svg': 'minimum-track-width.svg',
    'copper_to_hole_clearance.svg': 'copper-to-hole-clearance.svg',
    'hole_to_hole_clearance.svg': 'hole-to-hole-clearance.svg',
    'minimum_annular_width.svg': 'minimum-annular-width.svg',
    'minimum_drill_size.svg': 'minimum-drill-size.svg',
    'hole-to-hole-distance.svg': 'hole-to-hole-distance.svg',
    'minimum_via_diameter.svg': 'minimum-via-diameter.svg',
    'silk-to-soldermask-clearance.svg': 'silk-to-soldermask-clearance.svg',
    'silk-to-silk-clearance.svg': 'silk-to-silk-clearance.svg',
    'minimum-soldermask-silver.svg': 'minimum-soldermask-silver.svg',
    'solderpaste-expansion.svg': 'solderpaste-expansion.svg',
    'maximum_allowed_deviation.svg': 'maximum-allowed-deviation.svg',
    'minimum-angular-ring.svg': 'minimum-angular-ring.svg',
    'matched-lenght-diff-pair.svg': 'matched-length-diff-pair.svg',
}


def transform_colors_to_dark(svg_content: str) -> str:
    """Transform light theme colors to dark theme colors."""
    result = svg_content

    for light_color, dark_color in LIGHT_TO_DARK_COLORS.items():
        # Case-insensitive replacement for hex colors
        pattern = re.compile(re.escape(light_color), re.IGNORECASE)
        result = pattern.sub(dark_color, result)

    return result


def process_svg_files():
    """Process all constraint SVGs and create themed versions."""
    script_dir = Path(__file__).parent
    source_dir = script_dir / 'sources' / 'constraints'
    light_dir = script_dir / 'sources' / 'light' / 'constraints'
    dark_dir = script_dir / 'sources' / 'dark' / 'constraints'

    # Create output directories if they don't exist
    light_dir.mkdir(parents=True, exist_ok=True)
    dark_dir.mkdir(parents=True, exist_ok=True)

    processed = 0
    skipped = 0

    for source_name, output_name in CONSTRAINT_FILES.items():
        source_path = source_dir / source_name

        if not source_path.exists():
            print(f"WARNING: Source file not found: {source_name}")
            skipped += 1
            continue

        # Read source SVG
        with open(source_path, 'r', encoding='utf-8') as f:
            svg_content = f.read()

        # Write light version (copy as-is, since originals use light theme colors)
        light_path = light_dir / output_name
        with open(light_path, 'w', encoding='utf-8') as f:
            f.write(svg_content)
        print(f"Created light: {output_name}")

        # Transform and write dark version
        dark_content = transform_colors_to_dark(svg_content)
        dark_path = dark_dir / output_name
        with open(dark_path, 'w', encoding='utf-8') as f:
            f.write(dark_content)
        print(f"Created dark:  {output_name}")

        processed += 1

    print(f"\nProcessed {processed} files, skipped {skipped}")
    return processed, skipped


if __name__ == '__main__':
    process_svg_files()
