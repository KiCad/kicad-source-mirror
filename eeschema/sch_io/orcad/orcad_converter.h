/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file orcad_converter.h
 *
 * ORCAD_CONVERTER turns a parsed ORCAD_DESIGN into KiCad schematic objects.
 * The class is implemented across two translation units:
 *
 *   - orcad_converter_symbols.cpp: LIB_SYMBOL building from cache definitions,
 *     variant selection, placeholder synthesis, SCH_SYMBOL/power-symbol placement
 *     and field placement.
 *   - orcad_converter_sheet.cpp: root/page sheet assembly, wires, buses, labels,
 *     junctions, no-connects, page graphics, title blocks, page sizing and
 *     bitmaps.
 *
 * Each private method below is tagged with the translation unit that implements
 * it.  Only these two files may define ORCAD_CONVERTER members.
 *
 * Coordinate model: 1 OrCAD DBU = 10 mil, Y down on both sides, so
 * IU = DBU * 2540 with no axis flip anywhere: KiCad's in-memory LIB_SYMBOL
 * space is Y-down like the canvas (only the .kicad_sch writer flips symbol
 * bodies into the file's Y-up space), so cache definition coordinates are
 * used unchanged.  Electrical points land on a 100-mil grid natively.
 */

#ifndef ORCAD_CONVERTER_H_
#define ORCAD_CONVERTER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <wx/string.h>

#include <math/box2.h>
#include <math/vector2d.h>
#include <gal/color4d.h>
#include <eda_shape.h>

#include <sch_io/orcad/orcad_records.h>

class LIB_SYMBOL;
class EDA_TEXT;
class PROGRESS_REPORTER;
class REPORTER;
class SCHEMATIC;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_SYMBOL;
class wxMemoryBuffer;


/// Schematic internal units per OrCAD DBU: 10 mil * 254 IU/mil.
inline constexpr int ORCAD_IU_PER_DBU = 2540;

KIGFX::COLOR4D OrcadColor( int aColorIndex );
int            OrcadLineWidthIu( int aWidth );
LINE_STYLE     OrcadLineStyle( int aStyle );
FILL_T         OrcadFillType( int aFillStyle, int aHatchStyle );
int            OrcadPageOrder( wxString& aName );

inline VECTOR2I OrcadDbuToIu( int aX, int aY )
{
    return VECTOR2I( aX * ORCAD_IU_PER_DBU, aY * ORCAD_IU_PER_DBU );
}


/**
 * One entry of the 8-orientation placement table.
 *
 * The OrCAD instance orientation byte packs bits 0..1 = quarter turns and
 * bit 2 = mirror.  OrCAD rotates/mirrors about the symbol bounding box and
 * re-anchors the box, while KiCad rotates about the anchor point; the offset
 * selectors reconcile the two (calibrated against T0x10 absolute pin positions).
 *
 * Offset selectors: 0 -> 0, 1 -> bbox width, 2 -> bbox height.
 * (a, b, c, d) is the OrCAD canvas transform of symbol-space points:
 * (px, py) -> (a*px + b*py, c*px + d*py), applied after the offset.
 */
struct ORCAD_ORIENT_ENTRY
{
    int    angle;    ///< KiCad placement angle in degrees (0/90/180/270, CCW)
    char   mirror;   ///< 0 = none, 'x' or 'y' = KiCad mirror axis
    int8_t txSel;    ///< X offset selector
    int8_t tySel;    ///< Y offset selector
    int8_t a;
    int8_t b;
    int8_t c;
    int8_t d;
};

/**
 * Orientation table, indexed by the 3-bit orientation code.  KiCad angle/mirror
 * pairs and bbox anchor offsets:
 *
 *   ori : (angle, mirror) offset(w, h)     matrix (a b c d)
 *    0  : (  0, -)        (0, 0)           ( 1  0  0  1)
 *    1  : ( 90, -)        (0, w)           ( 0  1 -1  0)
 *    2  : (180, -)        (w, h)           (-1  0  0 -1)
 *    3  : (270, -)        (h, 0)           ( 0 -1  1  0)
 *    4  : (  0, y)        (w, 0)           (-1  0  0  1)
 *    5  : ( 90, x)        (0, 0)           ( 0  1  1  0)
 *    6  : (  0, x)        (0, h)           ( 1  0  0 -1)
 *    7  : (270, x)        (h, w)           ( 0 -1 -1  0)
 */
inline constexpr ORCAD_ORIENT_ENTRY ORCAD_ORIENT_TABLE[8] = {
    { 0,   0,   0, 0,    1,  0,  0,  1 },
    { 90,  0,   0, 1,    0,  1, -1,  0 },
    { 180, 0,   1, 2,   -1,  0,  0, -1 },
    { 270, 0,   2, 0,    0, -1,  1,  0 },
    { 0,   'y', 1, 0,   -1,  0,  0,  1 },
    { 90,  'x', 0, 0,    0,  1,  1,  0 },
    { 0,   'x', 0, 2,    1,  0,  0, -1 },
    { 270, 'x', 2, 1,    0, -1, -1,  0 }
};


/// Compose the 3-bit orientation code from the rotation bits and mirror bit.
inline int OrcadOrientOf( int aRotation, bool aMirror )
{
    return ( aRotation & 3 ) | ( aMirror ? 0x4 : 0 );
}


inline int OrcadOrientDim( int aSelector, int aWidth, int aHeight )
{
    return aSelector == 1 ? aWidth : aSelector == 2 ? aHeight : 0;
}


/// Bbox re-anchoring offset for the given orientation and body size (DBU).
inline VECTOR2I OrcadOrientOffset( int aOrient, int aWidth, int aHeight )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    return VECTOR2I( OrcadOrientDim( e.txSel, aWidth, aHeight ),
                     OrcadOrientDim( e.tySel, aWidth, aHeight ) );
}


/**
 * Absolute canvas position (DBU) of symbol-space point (aPx, aPy) for an instance
 * with transform base (aBaseX, aBaseY), orientation aOrient and body size
 * (aWidth, aHeight).  For placed parts the base is the instance anchor; for
 * power/off-page/port symbols it is the placed bbox min corner, NOT the anchor.
 */
inline VECTOR2I OrcadTransformPoint( int aOrient, int aWidth, int aHeight, int aBaseX, int aBaseY,
                                     int aPx, int aPy )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    VECTOR2I                  t = OrcadOrientOffset( aOrient, aWidth, aHeight );

    return VECTOR2I( aBaseX + t.x + e.a * aPx + e.b * aPy,
                     aBaseY + t.y + e.c * aPx + e.d * aPy );
}


/**
 * Builds the KiCad schematic from a parsed OrCAD design.
 *
 * Usage (from SCH_IO_ORCAD::LoadSchematicFile, after the standard root-sheet /
 * root-screen boilerplate):
 *
 *     ORCAD_CONVERTER converter( design, aSchematic, m_reporter, m_progressReporter );
 *     converter.Convert( rootSheet );
 *
 * The converter reports recoverable problems through the REPORTER (warnings for
 * conversion issues, info notes for facts about the source design) and never
 * aborts the import for a single bad record.  It does NOT call
 * UpdateAllScreenReferences() / FixupJunctionsAfterImport() — the plugin shell
 * does that after Convert() returns.
 */
class ORCAD_CONVERTER
{
public:
    /**
     * @param aDesign parsed design; held by reference and MUTATED during
     *                conversion (page-content shift, placeholder symbol
     *                insertion), so it must outlive the converter.
     *
     * [orcad_converter_sheet.cpp]
     */
    ORCAD_CONVERTER( ORCAD_DESIGN& aDesign, SCHEMATIC* aSchematic, REPORTER* aReporter,
                     PROGRESS_REPORTER* aProgressReporter = nullptr );

    /// Out-of-line: LIB_ENTRY holds LIB_SYMBOL by unique_ptr.  [orcad_converter_sheet.cpp]
    ~ORCAD_CONVERTER();

    /**
     * Populate the schematic.  [orcad_converter_sheet.cpp]
     *
     * @param aRootSheet the top-level sheet, already registered on the SCHEMATIC
     *                   with a screen attached and the sheet UUID set to the
     *                   screen UUID (the standard importer boilerplate).
     * @return aRootSheet.
     *
     * Steps:
     *  1. prepareSymbols() — placeholders, lib-entry registration, font baseline.
     *  2. An unambiguous Capture hierarchy becomes KiCad sheets at the source
     *     block positions, with source sheet pins and hierarchical labels.
     *     Otherwise, pages become separate top-level sheets in source page order.
     *  3. Per page: applyPageSettings(), then convertPage() with the page's
     *     SCH_SHEET_PATH (references are set against that path).
     *
     * Polls the PROGRESS_REPORTER (when present) once per page; a cancel throws
     * IO_ERROR.
     */
    SCH_SHEET* Convert( SCH_SHEET* aRootSheet );

    /**
     * Build KiCad library symbols from the design cache and packages alone (no
     * schematic pages), for importing an OrCAD .OLB library.  One entry per package
     * (multi-unit from its devices), plus any cache symbol not owned by a package.
     * The returned symbols are owned by the caller.  [orcad_converter_symbols.cpp]
     */
    std::vector<LIB_SYMBOL*> BuildSymbolLibrary();

    /// Symbol library nickname used in every emitted LIB_ID.
    static constexpr const char* LIB_NICK = "orcad_import";

private:
    // -- shared bookkeeping types ---------------------------------------------------

    /// One unit of an emitted multi-unit symbol.
    struct UNIT_INFO
    {
        std::string              letter;      ///< unit discriminator, e.g. "A", "-16", "B:Convert"
        const ORCAD_SYMBOL_DEF*  symbol = nullptr;
        std::vector<std::string> pinNumbers;  ///< device pin-number map for this unit
    };

    /// One emitted KiCad lib symbol (possibly multi-unit).
    struct LIB_ENTRY
    {
        std::string                 name;      ///< lib item name (no nickname)
        std::vector<UNIT_INFO>      units;     ///< sorted by letter; unit numbers are 1-based indices
        bool                        isPower = false;
        std::string                 powerNet;  ///< power symbols are keyed by NET name
        std::string                 refPrefix = "U";
        std::string                 footprint;
        std::unique_ptr<LIB_SYMBOL> kicadSymbol;  ///< built lazily by kicadSymbolFor()
    };

    /// One resolved off-page connector: index into page.offpage, net, pin position.
    struct OFFPAGE_NET
    {
        int         index = 0;
        std::string net;
        int         x = 0;  ///< DBU
        int         y = 0;
    };

    /// (sourcePackage-or-pkgName, pkgName, variant index) -> (lib name, unit number).
    using PKG_KEY = std::tuple<std::string, std::string, int>;

    // -- constants (calibrated; do not change) ----------------------------------------

    /// Standard OrCAD pin length used for synthesized placeholder pins, DBU.
    static constexpr int PIN_LEN_DBU = 10;

    /// Page content margins, DBU.  KiCad draws its frame about 10 mm inside the
    /// paper edge, so content needs clearance the OrCAD canvas did not; the
    /// bottom margin leaves extra room for KiCad's title block strip.
    static constexpr int MARGIN_L_DBU = 60;
    static constexpr int MARGIN_T_DBU = 60;
    static constexpr int MARGIN_R_DBU = 60;
    static constexpr int MARGIN_B_DBU = 100;

    // -- reporting [orcad_converter_sheet.cpp] ------------------------------------------

    /// Conversion problem: REPORTER at RPT_SEVERITY_WARNING.
    void warn( const wxString& aMsg );

    /// Fact about the source design (not a conversion problem): RPT_SEVERITY_INFO.
    void note( const wxString& aMsg );

    // ================================================================================
    // implemented in orcad_converter_symbols.cpp
    // ================================================================================

    /**
     * Pre-pass over all pages: synthesize placeholder definitions for symbols
     * absent from the cache (one note per name), register a LIB_ENTRY + unit for
     * every placed instance via libForInstance(), then computeFontBaseline().
     */
    void prepareSymbols();

    /**
     * Best-effort placeholder for a cache-less symbol, built from the instances'
     * T0x10 absolute pin records and the placed bbox: rectangle body with pins on
     * the sides where they actually sit.  Uses the instance closest to the
     * reference orientation (code 0 preferred); local pin p solves
     * T = I + t(w, h) + M*p, and the t-term cancels for instances sharing the
     * reference orientation, so connectivity is exact there.  With no pin records
     * the body comes from the inverse-transformed placed bbox.  Sets
     * synthesized = true.
     */
    ORCAD_SYMBOL_DEF synthesizeSymbol( const std::string& aPkgName,
                                       const std::vector<const ORCAD_PLACED_INSTANCE*>& aInstances ) const;

    /**
     * Unit discriminator of an instance: the part of pkgName between the package
     * base name and the '.View' suffix ("A", "-16", "", ...), plus ":View" when
     * the view is not "Normal" (DeMorgan variants).
     */
    std::string unitLetter( const ORCAD_PLACED_INSTANCE& aInst ) const;

    /**
     * The cache may hold several stale library versions of one symbol name; pick
     * the variant whose transformed pin hot points match this instance's T0x10
     * records exactly (pin count must match too), else the first cache entry.
     * @return (definition, variant index); (nullptr, 0) when the name is uncached.
     */
    std::pair<const ORCAD_SYMBOL_DEF*, int> pickVariant( const ORCAD_PLACED_INSTANCE& aInst ) const;

    /**
     * Resolve/register the emitted lib symbol and unit number for a placed part.
     * Lib name = sanitized package base name, with "_v<n+1>" appended for variant
     * index n >= 1.  Unit letters sort alphabetically and define the 1-based unit
     * numbers; inserting a new letter renumbers, so stale PKG_KEY map entries for
     * the lib are rebuilt.  Pin numbers come from the package device whose
     * unitRef equals the BARE unit letter (':View' suffix stripped — DeMorgan
     * views share the Normal view's pin map); an instance without a cached symbol
     * gets a placeholder (with warning).
     * @return (lib name, unit number).
     */
    std::pair<std::string, int> libForInstance( const ORCAD_PLACED_INSTANCE& aInst );

    /**
     * Resolve/register the power lib symbol for a power-symbol instance.  Power
     * lib symbols are keyed by NET name — "PWR_" + sanitized net — because users
     * rename power ports (e.g. a VCC_BAR symbol naming the net +3V3); the
     * graphics come from the named cache symbol (empty definition when uncached).
     * The entry has isPower = true, powerNet = aNetName, refPrefix = "#PWR" and a
     * single unit with pin number "1".
     * @return the lib name.
     */
    std::string powerLibFor( const std::string& aSymbolName, const std::string& aNetName );

    /**
     * Return the fully built KiCad LIB_SYMBOL for a registered LIB_ENTRY,
     * building and caching it on first use.  The symbol gets SetName + SetLibId
     * (LIB_NICK nickname), unit count from the entry, per-unit body graphics and
     * pins (Y-down, same as the cache definition space), and KiCad-style field
     * defaults: Reference = refPrefix (power: "#PWR", hidden), Value = lib name
     * (power: net name), hidden Footprint/Datasheet.  Power symbols get
     * SetGlobalPower() and a single invisible power-input pin named after the net.
     */
    LIB_SYMBOL* kicadSymbolFor( const std::string& aLibName );

    /**
     * Add one body primitive to a LIB_SYMBOL unit on LAYER_DEVICE.
     * Ellipses with unequal radii and all arcs become polyline approximations
     * (arcs are drawn counter-clockwise in the Y-down source space); polygons
     * are closed by repeating the first vertex; text becomes a symbol text item.
     */
    void addSymbolPrimitive( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit, int aColor, int aOffsetX = 0,
                             int aOffsetY = 0 );

    /// Polyline approximation of an arc primitive.  Helper of addSymbolPrimitive().
    void addSymbolArc( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit, int aColor, int aOffsetX = 0,
                       int aOffsetY = 0 );

    /**
     * Add one pin: position = hot point, length = |start - hot|,
     * orientation from the start-relative-to-hot direction, shape from shapeBits
     * (bit 1 = clock, bit 2 = inverted dot), electrical type from portType (power
     * symbols force power-in + hidden pin).  Pin name: aNameOverride when given,
     * else the pin name unless empty or "$PIN"-prefixed or equal to the number
     * (then "~").
     */
    void addSymbolPin( LIB_SYMBOL* aSymbol, const ORCAD_SYMBOL_PIN& aPin, const wxString& aNumber,
                       int aUnit, bool aPower, const std::string& aNameOverride );

    /**
     * Place one part instance on a screen: SCH_SYMBOL from the built LIB_SYMBOL
     * (flattened-copy constructor route), position = anchor + orientation offset,
     * orientation via toKicadOrientation(), fields via placeSymbolFields(),
     * SetRef( aSheetPath, reference ), unit selection, then screen->Append.
     * Verifies the transformed pin hot points against the instance's T0x10
     * records: mismatch is a warning (note when the definition is synthesized).
     */
    void placeInstance( ORCAD_RAW_PAGE& aPage, const ORCAD_PLACED_INSTANCE& aInst,
                        SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aSheetPath );

    /**
     * Resolve a placed instance's reference designator, falling back to the
     * Hierarchy-stream occurrence designator when the instance carries only the
     * unannotated "C?" template; returns "?" when neither is annotated.
     */
    wxString resolveReference( const ORCAD_PLACED_INSTANCE& aInst ) const;

    /**
     * Place one power symbol: transform base = placed bbox min corner (fall back
     * to the anchor when the bbox is all zero), reference "#PWR%04d" from a
     * design-wide counter (hidden), value = net name centered on the body —
     * above it, or below it for ground-style symbols whose pin connects in the
     * top quarter of the placed bbox.
     */
    void placePowerSymbol( ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst,
                           const std::string& aNet, SCH_SCREEN* aScreen,
                           const SCH_SHEET_PATH& aSheetPath );

    /**
     * KiCad-style field placement for a placed part, always reading horizontally
     * (fields rotate with the symbol, so a 90/270-degree placement gets a
     * compensating 90-degree field angle).  The true body box is derived from the
     * symbol bbox + orientation transform (the placed bbox includes displayed
     * text).  Vertical two-pin passives (placement angle 0/180, exactly 2 pins,
     * vertical pin span >= horizontal) get Reference/Value stacked to the right
     * of the body, left-justified, centered on it; everything else gets Reference
     * above and Value below, centered.  Visibility follows the source: a field is
     * shown only when the instance carries a display prop for it ("Part
     * Reference" / "Value").  Value fallback: instance value, else the "Value"
     * property, else the lib name.  Footprint: "PCB Footprint" property, else the
     * package footprint (hidden).  Remaining properties become user fields —
     * visible at their display-prop position (transformed to canvas) when
     * displayed, hidden at the anchor otherwise; bookkeeping names ("Part
     * Reference", "Reference", "Name", "Graphic", "Implementation",
     * "Implementation Path", "Implementation Type", "Source Library", "Source
     * Package", "Source Part") are dropped.
     */
    void placeSymbolFields( SCH_SYMBOL* aSymbol, const ORCAD_PLACED_INSTANCE& aInst,
                            const ORCAD_SYMBOL_DEF& aDef, int aOrient, const std::string& aValue,
                            const std::string& aFootprint );

    /**
     * Map an orientation code to the composed SYMBOL_ORIENTATION_T value for
     * SCH_SYMBOL::SetOrientation():
     *   0 -> SYM_ORIENT_0                 4 -> SYM_ORIENT_0   + SYM_MIRROR_Y
     *   1 -> SYM_ORIENT_90                5 -> SYM_ORIENT_90  + SYM_MIRROR_X
     *   2 -> SYM_ORIENT_180               6 -> SYM_ORIENT_0   + SYM_MIRROR_X
     *   3 -> SYM_ORIENT_270               7 -> SYM_ORIENT_270 + SYM_MIRROR_X
     */
    static int toKicadOrientation( int aOrient );

    /**
     * Find the design's dominant text height (most frequent font height over all
     * alias and display-prop font references); that height renders as KiCad's
     * default 1.27 mm and every other height scales relative to it, so uniform
     * designs come out uniform.  0 when the design has no font references.
     */
    void computeFontBaseline();

    /// |lfHeight| of a 1-based font index; 0 for invalid/default indices.
    int fontHeightDbu( int aFontIdx ) const;

    /**
     * Text size in IU for a font reference: 1.27 mm when the height is unknown;
     * with a baseline, 1.27 mm * height / baseline clamped to [0.7, 5.0] mm;
     * without a baseline, height * 25.4 * 0.75 / 96 mm with a 0.7 mm floor.
     */
    int textSizeIU( int aFontIdx ) const;

    void applyFont( EDA_TEXT* aText, int aFontIdx ) const;

    /// LIB_ID-safe symbol name: ':', '"', '/' and whitespace runs -> '_'; "SYM" if empty.
    static std::string SymbolId( const std::string& aName );

    // ================================================================================
    // implemented in orcad_converter_sheet.cpp
    // ================================================================================

    /**
     * Convert one page onto a screen: applyTitleBlock(), buildNetLookup(), then
     * junctions, no-connects, bus entries, wires + alias labels, off-page
     * connectors, ports, page graphics, part instances and power symbols.
     */
    void convertPage( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen,
                      const SCH_SHEET_PATH& aSheetPath );

    /**
     * "P%02d_<pagename>.kicad_sch" (1-based index), sanitized: control characters
     * and the characters <>:"/\|?* replaced by '_', leading/trailing spaces and
     * dots stripped, "unnamed" when empty; then ReplaceIllegalFileNameChars.
     */
    static wxString MakePageFileName( int aPageIndex, const std::string& aPageName );

    /**
     * Fit the page and set the screen's PAGE_INFO.  Content is shifted clear of
     * the frame: dx = max( 0, MARGIN_L - minX ), dy likewise for Y, both rounded
     * UP to the 10-DBU grid (connectivity preserved), applied via offsetPage().
     * Paper size: nominal = stored page size converted to mm (mils * 0.0254, or
     * um * 0.001 when isMetric); needed = (maxX + MARGIN_R, maxY + MARGIN_B) DBU
     * * 0.254 mm; final = ceil of the max of both per axis, as a custom "User"
     * page (PAGE_INFO::SetCustomWidthMils / SetCustomHeightMils then SetType).
     */
    void applyPageSettings( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /**
     * Bounding box (DBU) of everything drawable on the page: wire ends, instance
     * placed bboxes + T0x10 pin positions, anchor + bbox of ports/globals/
     * off-page/ERC objects, bus entries, block rectangles + pins, and — for free
     * graphics — ONLY the nested primitives' coordinates (the outer bbox of a
     * Graphic*Inst is anchor-relative junk).  Returns (0, 0, 3800, 2700) for an
     * empty page.
     */
    static BOX2I pageExtentDbu( const ORCAD_RAW_PAGE& aPage );

    /**
     * Shift every drawable coordinate on the page by (aDx, aDy) DBU: wires and
     * their aliases, instances (anchor, bbox, pin positions), graphic instances
     * (anchor, bbox, nested primitive coordinates including points/start/end),
     * bus entries and blocks.  aDx/aDy must be multiples of the 10-DBU grid.
     */
    static void offsetPage( ORCAD_RAW_PAGE& aPage, int aDx, int aDy );

    /**
     * Fill the screen TITLE_BLOCK from the first page title block carrying
     * properties: Title, date ("Page Modify Date", else "Doc Date"), revision
     * ("RevCode"), company ("OrgName"), comments ("Doc", "OrgAddr1", "OrgAddr2").
     */
    void applyTitleBlock( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /// Rebuild m_wireEndpoints for a page (both endpoints of every wire).
    void buildNetLookup( const ORCAD_RAW_PAGE& aPage );

    /**
     * Net name at a point: first a wire with an endpoint here whose id is in the
     * page net table; then any wire passing strictly through the point (id in
     * table); then the first alias of an endpoint wire; else "".
     */
    std::string netAt( const ORCAD_RAW_PAGE& aPage, int aX, int aY ) const;

    /**
     * Net stamped by a power-symbol instance: the net of the wire its pin touches
     * (authoritative — ports can be renamed), falling back to the symbol name.
     * Trimmed of surrounding whitespace.
     */
    std::string powerNet( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const;

    /**
     * Absolute connection point (DBU) of a GraphicInst-derived instance (power
     * symbol / off-page connector / port): the cache symbol's first pin hot point
     * transformed with base = placed bbox min corner (NOT the anchor).  Falls
     * back to the anchor when the symbol or its pins are unknown.
     */
    VECTOR2I graphicPinPos( const ORCAD_GRAPHIC_INST& aInst ) const;

    /// Resolve every off-page connector on the page to (index, net, pin position).
    std::vector<OFFPAGE_NET> offpageNets( const ORCAD_RAW_PAGE& aPage ) const;

    /**
     * Junction points (DBU) from wire geometry + shared net ids.  For every
     * candidate point (wire endpoints and T0x10/block pin positions) count wire
     * ends `end`, wires passing strictly through `through`, and pin presence
     * `pin` (0/1); a junction is needed when end + 2*through + pin >= 3 AND
     * end + pin >= 1 AND end + through >= 2 (three or more connection
     * contributions meet, at least one terminating here).
     */
    std::vector<VECTOR2I> computeJunctions( const ORCAD_RAW_PAGE& aPage ) const;

    /// Append computed junctions as SCH_JUNCTION items.
    void placeJunctions( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /**
     * Wires/buses as SCH_LINE on LAYER_WIRE / LAYER_BUS, plus a local SCH_LABEL
     * per wire alias: anchor snapped from the alias's free display position onto
     * the owning wire segment (snapToWire) so KiCad binds it electrically; text
     * angle 0/90 (source quadrants 2/3 fold onto 0/90); size from textSizeIU().
     */
    void placeWires( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /// SCH_BUS_WIRE_ENTRY per bus entry (position x1,y1; size x2-x1, y2-y1).
    void placeBusEntries( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /// SCH_NO_CONNECT per ERC object, at the object's anchor.
    void placeNoConnects( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /**
     * SCH_GLOBALLABEL per resolved off-page connector, at the connection point;
     * an unconnected connector (empty net) is skipped with a note.  Spin: with a
     * wire endpoint at the point, the label points away from the wire
     * (x1 + x2 - 2*px <= 0 selects the angle-0 spin, else angle-180); hide the
     * INTERSHEET_REFS field.
     */
    void placeOffpageConnectors( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /**
     * SCH_HIERLABEL per port on a hierarchical child page, otherwise
     * SCH_GLOBALLABEL.  Net = logical name, else the symbol name; empty nets are
     * skipped.  Shape: "LEFT" in the symbol name -> input, "RIGHT" -> output,
     * else bidirectional.
     */
    void placePorts( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, bool aHierarchical );

    /**
     * Free page graphics from the nested primitives of each Graphic*Inst (the
     * outer record itself is not drawable): comment text as SCH_TEXT (angle 0/90;
     * quadrants 2/3 fold to 0), lines/rects/polylines/polygons/beziers as
     * LAYER_NOTES polylines (rects closed, polygons closed), arcs/ellipses as
     * polyline approximations, images via placeBitmap().
     */
    void placeGraphics( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /**
     * One image primitive -> SCH_BITMAP centered on the primitive's page extent,
     * scaled from its native pixel size to the extent.  Plain DIB payloads have
     * a BMP file header synthesized in front of them (see MakeBmpFromDib).  OLE
     * payloads use their native bitmap or rasterized WMF preview.  Undecodable
     * payloads are skipped with a warning.  Extents smaller than 2 DBU or empty
     * payloads are ignored.
     */
    void placeBitmap( const ORCAD_PRIMITIVE& aPrim, SCH_SCREEN* aScreen, int aOrient = 0 );

    void placeDefinitionImages( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                SCH_SCREEN* aScreen );

    void placeDefinitionVectors( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                 SCH_SCREEN* aScreen );

    /**
     * Synthesize a .BMP in front of a raw DIB (BITMAPINFOHEADER + optional
     * palette + pixels): pixel data offset = 14 + biSize + 4 * paletteEntries
     * (paletteEntries = biClrUsed, else 2^biBitCount when biBitCount <= 8, else
     * 0) + 12 when biCompression == BI_BITFIELDS.  Rejects headers with
     * biSize < 40 or > 200.
     * @return false when the DIB is not usable.
     */
    static bool MakeBmpFromDib( const std::vector<uint8_t>& aDib, wxMemoryBuffer& aOut );

    /// Closest point (DBU) on a wire segment to (aX, aY); exact for H/V wires.
    static VECTOR2I snapToWire( int aX, int aY, const ORCAD_WIRE& aWire );

    /// True when the point lies strictly INSIDE (not at an endpoint of) an H/V wire.
    static bool onSegment( int aX, int aY, const ORCAD_WIRE& aWire );

    /**
     * Filesystem-safe name: control characters and <>:"/\|?* replaced by '_',
     * surrounding spaces/dots stripped, "unnamed" when empty.
     */
    static wxString SanitizeFileName( const std::string& aName );

private:
    ORCAD_DESIGN&      m_design;
    SCHEMATIC*         m_schematic;
    REPORTER*          m_reporter;
    PROGRESS_REPORTER* m_progressReporter;
    SCH_SHEET*         m_rootSheet;

    std::map<std::string, LIB_ENTRY>               m_libSymbols;  ///< keyed by emitted lib name
    std::map<PKG_KEY, std::pair<std::string, int>> m_pkgToLib;
    int                                            m_powerCount;      ///< "#PWR%04d" counter
    int                                            m_fontBaselineDbu; ///< dominant text height; 0 = none

    /// Occurrence reference designators of the scope currently being converted; set
    /// per page so a child schematic reused by several block occurrences takes each
    /// occurrence's own designators.  Null when the design has no occurrence tree.
    const std::map<uint32_t, std::string>*         m_currentOccRefs = nullptr;

    /// Lower-cased sheet names already emitted, to keep sibling sheet names unique.
    std::set<wxString>                             m_usedSheetNames;

    /// Per-page wire lookup: endpoint -> wires ending there.  Rebuilt by buildNetLookup().
    std::map<std::pair<int, int>, std::vector<const ORCAD_WIRE*>> m_wireEndpoints;
};

#endif // ORCAD_CONVERTER_H_
