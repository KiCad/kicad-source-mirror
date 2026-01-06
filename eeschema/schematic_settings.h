/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SCHEMATIC_SETTINGS_H
#define KICAD_SCHEMATIC_SETTINGS_H

#include <default_values.h>
#include <settings/nested_settings.h>
#include <settings/bom_settings.h>
#include <template_fieldnames.h>
#include <font/font.h>

class NGSPICE_SETTINGS;
class REFDES_TRACKER;


// The minimal grid size allowed to place a pin is 25 mils.  Tthe best grid size is 50 mils,
// but 25 mils is still usable.
// This is because all symbols are using a 50 mils grid to place pins, and therefore the wires
// must be on the 50 mils grid.
#define MIN_CONNECTION_GRID_MILS 25
#define DEFAULT_CONNECTION_GRID_MILS 50


/**
 * These are loaded from Eeschema settings but then overwritten by the project settings.
 * All of the values are stored in IU, but the backing file stores in mils.
 */
class SCHEMATIC_SETTINGS : public NESTED_SETTINGS
{
public:
    SCHEMATIC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~SCHEMATIC_SETTINGS();

    wxString SubReference( int aUnit, bool aAddSeparator = true ) const;

public:
    // Default sizes are all stored in IU here, and in mils in the JSON file

    int       m_DefaultLineWidth;
    int       m_DefaultTextSize;
    double    m_LabelSizeRatio;
    double    m_TextOffsetRatio;
    int       m_PinSymbolSize;

    int       m_JunctionSizeChoice;     // none = 0, smallest = 1, small = 2, etc.
    int       m_JunctionSize;           // a runtime cache of the calculated size

    int       m_HopOverSizeChoice;      // none = 0, smallest = 1, etc.
    double    m_HopOverScale;           // a runtime cache of the calculated lineWidth multiplier

    int       m_ConnectionGridSize;     // usually 50mils (IU internally; mils in the JSON file)

    int       m_AnnotateStartNum;       // Starting value for annotation
    int       m_AnnotateSortOrder;      // Annotation sort order
    int       m_AnnotateMethod;         // Annotation numbering method (linear, sheet * 100, etc)

    int       m_SubpartIdSeparator;     // the separator char between the subpart id and the
                                        //   reference like U1A, U1.A or U1-A
    int       m_SubpartFirstId;         // the ASCII char value to calculate the subpart symbol
                                        //   id from the symbol number: only 'A', 'a' or '1' can
                                        //   be used, other values have no sense.

    bool      m_IntersheetRefsShow;
    bool      m_IntersheetRefsListOwnPage;
    bool      m_IntersheetRefsFormatShort;
    wxString  m_IntersheetRefsPrefix;
    wxString  m_IntersheetRefsSuffix;

    double    m_DashedLineDashRatio;        // Dash length as ratio of the lineWidth
    double    m_DashedLineGapRatio;         // Gap length as ratio of the lineWidth

    int       m_OPO_VPrecision;         // Operating-point overlay voltage significant digits
    wxString  m_OPO_VRange;             // Operating-point overlay voltage range
    int       m_OPO_IPrecision;         // Operating-point overlay current significant digits
    wxString  m_OPO_IRange;             // Operating-point overlay current range

    wxString  m_SchDrawingSheetFileName;
    wxString  m_PlotDirectoryName;

    TEMPLATES m_TemplateFieldNames;

    wxString  m_BomExportFileName;

    /// List of stored BOM presets
    BOM_PRESET                  m_BomSettings;
    std::vector<BOM_PRESET>     m_BomPresets;

    /// List of stored BOM format presets
    BOM_FMT_PRESET              m_BomFmtSettings;
    std::vector<BOM_FMT_PRESET> m_BomFmtPresets;

    KIFONT::METRICS             m_FontMetrics;

    /// Max deviation allowable when approximating circles and curves (in IU).
    int                         m_MaxError;

    /**
     * Ngspice simulator settings.
     */
    std::shared_ptr<NGSPICE_SETTINGS> m_NgspiceSettings;

    /**
     * A list of previously used schematic reference designators.
     * This is used to avoid reusing designators in the same project.
     */
    std::shared_ptr<REFDES_TRACKER> m_refDesTracker;

    /**
     * A map of variant names to their descriptions.
     * This is stored in the project file and is the authoritative source for variant metadata.
     */
    std::map<wxString, wxString> m_VariantDescriptions;
};

#endif
