# PADS PCB Importer

This plugin imports PADS PowerPCB ASCII (.asc) files into KiCad.

## Supported Features

### Board Elements
- Copper layers (up to 32 layers)
- Silkscreen and solder mask layers
- Board outline
- Tracks and vias
- Pads with various shapes (round, square, rectangular, oblong, annular)
- Copper pours/zones
- Text elements
- Keepout areas

### Via Types
- Standard through-hole vias
- Blind and buried vias (mapped to micro vias where applicable)

### Footprints
- Imported footprints are cached in the board file
- Original PADS decal names preserved where possible

## Command Line Usage

```bash
kicad-cli pcb import --format pads input.asc -o output.kicad_pcb
```

### Options
- `--format pads` - Specify PADS format (auto-detected from .asc extension)
- `-o, --output` - Output KiCad PCB file path
- `--report-format json|text` - Generate import report
- `--report-file` - Path to save import report

## Known Limitations

1. **Copper Pours**: Complex pour shapes may not import perfectly. Zone fills
   should be regenerated in KiCad after import.

2. **Design Rules**: PADS design rules are not imported. Default KiCad DRC
   rules will apply to imported boards.

3. **Net Classes**: PADS net classes are imported but may need adjustment
   for KiCad's net class system.

4. **Schematic Link**: Only PCB data is imported; schematic must be
   recreated or imported separately.

## File Format Support

### Supported Versions
- PADS PowerPCB V9.0 through V9.5
- PADS Layout files exported as ASCII

### Unit Systems
- MILS (default for most PADS files)
- METRIC (millimeters)
- INCHES
- BASIC (PADS internal database units)

## Developer Notes

### Architecture

The importer consists of three main components:

1. **PCB_IO_PADS** (`pcb_io_pads.cpp`) - Plugin interface implementing PCB_IO
2. **PADS_PARSER** (`pads_parser.cpp`) - Parses ASCII format sections
3. **Layer Mapping** - Maps PADS layer numbers to KiCad layer IDs

### Extending the Parser

To add support for new PADS features:

1. Identify the section keyword in the ASCII format (e.g., `*PARTDECAL*`)
2. Add a parsing method in `PADS_PARSER` class
3. Map parsed data to appropriate KiCad objects
4. Add unit tests in `qa/tests/pcbnew/`

### Testing

```bash
# Run PADS importer tests
cd build
ctest -R pads -V

# Run QA visual regression tests
KICAD_CLI=./build/kicad/kicad-cli python -m pytest qa/tests/cli/test_pads_importer.py -v
```

### Test Data

Test files are located in `qa/data/pcbnew/plugins/pads/`:
- `CE086A5_3/` - 8-layer complex board
- `KA014A2/` - 2-layer board
- `TJ005A1t/` - 2-layer board
- `synthetic_*.asc` - Synthetic test cases for edge cases
