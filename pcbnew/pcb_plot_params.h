#ifndef PCB_PLOT_PARAMS_H_
#define PCB_PLOT_PARAMS_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <plotters/plotter.h>
#include <layer_ids.h>
#include <plotprint_opts.h>

class COLOR_SETTINGS;
class PCB_PLOT_PARAMS_PARSER;

/**
 * Parameters and options when plotting/printing a board.
 */
class PCB_PLOT_PARAMS
{
public:
    PCB_PLOT_PARAMS();

    void        SetSkipPlotNPTH_Pads( bool aSkip ) { m_skipNPTH_Pads = aSkip; }
    bool        GetSkipPlotNPTH_Pads() const { return m_skipNPTH_Pads; }

    void        Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControl=0 ) const;
    void        Parse( PCB_PLOT_PARAMS_PARSER* aParser );

    /**
     * Compare current settings to aPcbPlotParams, including not saved parameters in brd file.
     *
     * @param aPcbPlotParams is the #PCB_PLOT_PARAMS to compare/
     * @param aCompareOnlySavedPrms set to true to compare only saved in file parameters,
     *        or false to compare the full set of parameters.
     * @return true is parameters are same, false if one (or more) parameter does not match.
     */
    bool        IsSameAs( const PCB_PLOT_PARAMS &aPcbPlotParams ) const;

    void SetColorSettings( COLOR_SETTINGS* aSettings ) { m_colors = aSettings; }

    COLOR_SETTINGS* ColorSettings() const { return m_colors; }

    void SetTextMode( PLOT_TEXT_MODE aVal )
    {
        m_textMode = aVal;
    }

    PLOT_TEXT_MODE GetTextMode() const
    {
        return m_textMode;
    }

    void        SetPlotMode( OUTLINE_MODE aPlotMode ) { m_plotMode = aPlotMode; }
    OUTLINE_MODE GetPlotMode() const { return m_plotMode; }

    void        SetDXFPlotPolygonMode( bool aFlag ) { m_DXFPolygonMode = aFlag; }
    bool        GetDXFPlotPolygonMode() const { return m_DXFPolygonMode; }

    void        SetDXFPlotUnits( DXF_UNITS aUnit ) { m_DXFUnits = aUnit; }
    DXF_UNITS   GetDXFPlotUnits() const { return m_DXFUnits; }

    void        SetDrillMarksType( DRILL_MARKS aVal ) { m_drillMarks = aVal; }
    DRILL_MARKS GetDrillMarksType() const { return m_drillMarks; }

    void        SetScale( double aVal ) { m_scale = aVal; }
    double      GetScale() const { return m_scale; }

    void        SetFineScaleAdjustX( double aVal ) { m_fineScaleAdjustX = aVal; }
    double      GetFineScaleAdjustX() const { return m_fineScaleAdjustX; }
    void        SetFineScaleAdjustY( double aVal ) { m_fineScaleAdjustY = aVal; }
    double      GetFineScaleAdjustY() const { return m_fineScaleAdjustY; }
    void        SetWidthAdjust( int aVal ) { m_widthAdjust = aVal; }
    int         GetWidthAdjust() const { return m_widthAdjust; }

    void        SetAutoScale( bool aFlag ) { m_autoScale = aFlag; }
    bool        GetAutoScale() const { return m_autoScale; }

    void        SetMirror( bool aFlag ) { m_mirror = aFlag; }
    bool        GetMirror() const { return m_mirror; }

    void        SetSketchPadsOnFabLayers( bool aFlag ) { m_sketchPadsOnFabLayers = aFlag; }
    bool        GetSketchPadsOnFabLayers() const { return m_sketchPadsOnFabLayers; }
    void        SetSketchPadLineWidth( int aWidth ) { m_sketchPadLineWidth = aWidth; }
    int         GetSketchPadLineWidth() const { return m_sketchPadLineWidth; }

    void        SetPlotInvisibleText( bool aFlag ) { m_plotInvisibleText = aFlag; }
    bool        GetPlotInvisibleText() const { return m_plotInvisibleText; }
    void        SetPlotValue( bool aFlag ) { m_plotValue = aFlag; }
    bool        GetPlotValue() const { return m_plotValue; }
    void        SetPlotReference( bool aFlag ) { m_plotReference = aFlag; }
    bool        GetPlotReference() const { return m_plotReference; }

    void        SetNegative( bool aFlag ) { m_negative = aFlag; }
    bool        GetNegative() const { return m_negative; }

    void        SetPlotViaOnMaskLayer( bool aFlag ) { m_plotViaOnMaskLayer = aFlag; }
    bool        GetPlotViaOnMaskLayer() const { return m_plotViaOnMaskLayer; }

    void        SetPlotFrameRef( bool aFlag ) { m_plotDrawingSheet = aFlag; }
    bool        GetPlotFrameRef() const { return m_plotDrawingSheet; }

    void        SetFormat( PLOT_FORMAT aFormat ) { m_format = aFormat; }
    PLOT_FORMAT GetFormat() const { return m_format; }

    void        SetOutputDirectory( const wxString& aDir ) { m_outputDirectory = aDir; }
    wxString    GetOutputDirectory() const { return m_outputDirectory; }

    void        SetDisableGerberMacros( bool aDisable ) { m_gerberDisableApertMacros = aDisable; }
    bool        GetDisableGerberMacros() const { return m_gerberDisableApertMacros; }

    void        SetUseGerberX2format( bool aUse ) { m_useGerberX2format = aUse; }
    bool        GetUseGerberX2format() const { return m_useGerberX2format; }

    void        SetIncludeGerberNetlistInfo( bool aUse ) { m_includeGerberNetlistInfo = aUse; }
    bool        GetIncludeGerberNetlistInfo() const { return m_includeGerberNetlistInfo; }

    void        SetCreateGerberJobFile( bool aCreate ) { m_createGerberJobFile = aCreate; }
    bool        GetCreateGerberJobFile() const { return m_createGerberJobFile; }

    void        SetUseGerberProtelExtensions( bool aUse ) { m_useGerberProtelExtensions = aUse; }
    bool        GetUseGerberProtelExtensions() const { return m_useGerberProtelExtensions; }

    void        SetGerberPrecision( int aPrecision );
    int         GetGerberPrecision() const { return m_gerberPrecision; }

    void        SetSvgPrecision( unsigned aPrecision );
    unsigned    GetSvgPrecision() const { return m_svgPrecision; }

    void        SetBlackAndWhite( bool blackAndWhite ) { m_blackAndWhite = blackAndWhite; }
    unsigned    GetBlackAndWhite() const { return m_blackAndWhite; }

    void        SetSubtractMaskFromSilk( bool aSubtract ) { m_subtractMaskFromSilk = aSubtract; }
    bool        GetSubtractMaskFromSilk() const { return m_subtractMaskFromSilk; }

    void        SetLayerSelection( LSET aSelection )    { m_layerSelection = aSelection; }
    LSET        GetLayerSelection() const               { return m_layerSelection; }

    void        SetPlotOnAllLayersSelection( LSET aSelection )
    {
        m_plotOnAllLayersSelection = aSelection;
    }

    LSET        GetPlotOnAllLayersSelection() const { return m_plotOnAllLayersSelection; }

    void        SetUseAuxOrigin( bool aAux ) { m_useAuxOrigin = aAux; }
    bool        GetUseAuxOrigin() const { return m_useAuxOrigin; }

    void        SetScaleSelection( int aSelection ) { m_scaleSelection = aSelection; }
    int         GetScaleSelection() const { return m_scaleSelection; }

    void        SetA4Output( int aForce ) { m_A4Output = aForce; }
    bool        GetA4Output() const { return m_A4Output; }

    // For historical reasons, this parameter is stored in mils
    // (but is in mm in hpgl files...)
    double      GetHPGLPenDiameter() const { return m_HPGLPenDiam; }
    bool        SetHPGLPenDiameter( double aValue );

    // This parameter is always in cm, due to hpgl file format constraint
    int         GetHPGLPenSpeed() const { return m_HPGLPenSpeed; }
    bool        SetHPGLPenSpeed( int aValue );

    void        SetHPGLPenNum( int aVal ) { m_HPGLPenNum = aVal; }
    int         GetHPGLPenNum() const { return m_HPGLPenNum; }

    void        SetDashedLineDashRatio( double aVal ) { m_dashedLineDashRatio = aVal; }
    double      GetDashedLineDashRatio() const { return m_dashedLineDashRatio; }

    void        SetDashedLineGapRatio( double aVal ) { m_dashedLineGapRatio = aVal; }
    double      GetDashedLineGapRatio() const { return m_dashedLineGapRatio; }

public:
    bool        m_PDFFrontFPPropertyPopups;   ///< Generate PDF property popup menus for footprints
    bool        m_PDFBackFPPropertyPopups;    ///<   on front and/or back of board

private:
    friend class PCB_PLOT_PARAMS_PARSER;

    PLOT_FORMAT     m_format;           /// Plot format type (chooses the driver to be used)
    LSET            m_layerSelection;
    LSET            m_plotOnAllLayersSelection;

    bool            m_skipNPTH_Pads;    /// Used to disable NPTH pads plotting on copper layers
    OUTLINE_MODE    m_plotMode;         /// FILLED or SKETCH for filled objects.
    DRILL_MARKS     m_drillMarks;       /// Holes can be not plotted, have a small mark, or be
                                        ///   plotted in actual size
    PLOT_TEXT_MODE  m_textMode;

    DXF_UNITS       m_DXFUnits;
    bool            m_DXFPolygonMode;   /// In polygon mode, each item to plot is converted to a
                                        ///   polygon and all polygons are merged.

    bool       m_A4Output;              /// Autoscale the plot to fit an A4 (landscape?) sheet
    bool       m_autoScale;             /// When true set the scale to fit the board in the page
    double     m_scale;                 /// Global scale factor, 1.0 plots a board at actual size
    bool       m_mirror;                /// Mirror the plot around the X axis

    bool       m_negative;              /// Plot in negative color (supported only by some drivers)
    bool       m_blackAndWhite;         /// Plot in black and white only
    bool       m_plotDrawingSheet;


    bool       m_plotViaOnMaskLayer;    /// True if vias are drawn on Mask layer (ie untented,
                                        ///   *exposed* by mask)
    bool       m_subtractMaskFromSilk;  /// On gerbers 'scrape' away the solder mask from
                                        ///   silkscreen (trim silks)


    /// When plotting gerber files, use a conventional set of Protel extensions instead of .gbr,
    /// that is now the official gerber file extension (this is a deprecated feature)
    bool       m_useGerberProtelExtensions;

    /// Include attributes from the Gerber X2 format (chapter 5 in revision J2)
    bool       m_useGerberX2format;

    /// Disable aperture macros in Gerber format (only for broken Gerber readers).  Ideally,
    /// should be never selected.
    bool       m_gerberDisableApertMacros;

    /// Include netlist info (only in Gerber X2 format) (chapter ? in revision ?)
    bool       m_includeGerberNetlistInfo;

    /// generate the auxiliary "job file" in gerber format
    bool       m_createGerberJobFile;

    /// Precision of coordinates in Gerber: accepted 5 or 6 when units are in mm, 6 or 7 in inches
    /// (but Pcbnew uses mm).
    /// 6 is the internal resolution of Pcbnew, but not always accepted by board maker
    /// 5 is the minimal value for professional boards
    int        m_gerberPrecision;

    /// Precision of coordinates in SVG: accepted 3 - 6; 6 is the internal resolution of Pcbnew
    unsigned   m_svgPrecision;

    bool       m_useAuxOrigin;          ///< Plot gerbers using auxiliary (drill) origin instead
                                        ///<   of absolute coordinates


    wxString   m_outputDirectory;       ///< Output directory for plot files (usually relative to
                                        ///<   the board file)
    int        m_scaleSelection;        ///< Scale ratio index (UI only)

    bool       m_plotReference;         ///< Enable plotting of part references
    bool       m_plotValue;             ///< Enable plotting of part values
    bool       m_plotInvisibleText;     ///< Force plotting of fields marked invisible

    bool       m_sketchPadsOnFabLayers; ///< Plots pads outlines on fab layers
    int        m_sketchPadLineWidth;

    double     m_fineScaleAdjustX;      ///< Compensation for printer scale errors (and therefore
    double     m_fineScaleAdjustY;      ///<   expected to be very near 1.0).  Only X and Y
                                        ///<   dimensions are adjusted: circles are plotted as
                                        ///<   circles, even if X and Y fine scale differ.
                                        ///<   Because of this it is mostly useful for printers:
                                        ///<   postscript plots should use the prologue, which will
                                        ///<   change the whole output matrix.

    int        m_widthAdjust;           ///< Compensation for PS printers/plotters that do not
                                        ///<   strictly obey line width settings. Only used to plot
                                        ///<   pads and tracks.

    int        m_HPGLPenNum;            ///< HPGL only: pen number selection(1 to 9)
    int        m_HPGLPenSpeed;          ///< HPGL only: pen speed, always in cm/s (1 to 99 cm/s)
    double     m_HPGLPenDiam;           ///< HPGL only: pen diameter in MILS, useful to fill areas
                                        ///< However, it is in mm in hpgl files.

    double     m_dashedLineDashRatio;
    double     m_dashedLineGapRatio;

    COLOR_SETTINGS* m_colors;           /// Pointer to color settings to be used for plotting

    /// Dummy colors object that can be created if there is no Pgm context
    std::shared_ptr<COLOR_SETTINGS> m_default_colors;
};


#endif // PCB_PLOT_PARAMS_H_
