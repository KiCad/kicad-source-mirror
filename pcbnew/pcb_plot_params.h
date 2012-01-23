#ifndef PCB_PLOT_PARAMS_H_
#define PCB_PLOT_PARAMS_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/wx.h>
#include <pcb_plot_params_lexer.h>
#include <base_struct.h>

class PCB_PLOT_PARAMS_PARSER;

/**
 * Class PCB_PLOT_PARAMS
 * handles plot parameters and options when plotting/printing a board.
 */
class PCB_PLOT_PARAMS
{
    friend class PCB_PLOT_PARAMS_PARSER;
public:
    bool        m_ExcludeEdgeLayer;     ///< True: do not plot edge layer when plotting other layers
                                        ///< False: Edge layer always plotted (merged) when plotting other layers
    int         m_PlotLineWidth;
    bool        m_PlotFrameRef;         ///< True to plot/print frame references
    bool        m_PlotViaOnMaskLayer;   ///< True if vias are drawn on Mask layer
                                        ///< (ie protected by mask)
    EDA_DRAW_MODE_T m_PlotMode;         ///< LINE, FILLED or SKETCH: select how to plot filled objects.
                                        ///< depending on plot format or layers, all options are not always allowed
    int         m_HPGLPenNum;
    int         m_HPGLPenSpeed;
    int         m_HPGLPenDiam;
    int         m_HPGLPenOvr;
    int         m_PlotPSColorOpt;       ///< True for color Postscript output
    bool        m_PlotPSNegative;       ///< True to create a  negative board ps plot

    //  Flags to enable or disable ploting of various PCB elements.

    bool        m_SkipNPTH_Pads;        ///< true to disable plot NPTH pads if hole and size have same value
                                        ///< GERBER only
    bool        m_PlotReference;
    bool        m_PlotValue;
    bool        m_PlotTextOther;
    bool        m_PlotInvisibleTexts;
    bool        m_PlotPadsOnSilkLayer;  ///< allows pads outlines on silkscreen layer (when pads are also o, silk screen

    int         m_PlotFormat;           ///< id for plot format (see enum PlotFormat in plot_common.h) */
    bool        m_PlotMirror;

    enum DrillShapeOptT {
        NO_DRILL_SHAPE    = 0,
        SMALL_DRILL_SHAPE = 1,
        FULL_DRILL_SHAPE  = 2
    };
    DrillShapeOptT m_DrillShapeOpt;     ///< For postscript output: holes can be not plotted,
                                        ///< or have a small size or plotted with their actual size
    bool        m_AutoScale;            ///< If true, use the better scale to fit in page
    double      m_PlotScale;            ///< The global scale factor. a 1.0 scale factor plot a board
                                        ///< with its actual size.

    // These next two scale factors are intended to compensable plotters (and mainly printers) X and Y scale error.
    // Therefore they are expected very near 1.0
    // Only X and Y dimensions are adjusted: circles are plotted as circle, even if X and Y fine scale differ.

    double      m_FineScaleAdjustX;     ///< fine scale adjust X axis
    double      m_FineScaleAdjustY;     ///< dine scale adjust Y axis

    /// This width factor is intended to compensate printers and plotters that do
    /// not strictly obey line width settings.
    double      m_FineWidthAdjust;

private:
    long        layerSelection;
    bool        useGerberExtensions;
    bool        useAuxOrigin;
    bool        subtractMaskFromSilk;
    bool        psA4Output;
    int         scaleSelection;
    wxString    outputDirectory;

public:
    PCB_PLOT_PARAMS();

    void        Format( OUTPUTFORMATTER* aFormatter, int aNestLevel ) const throw( IO_ERROR );
    void        Parse( PCB_PLOT_PARAMS_PARSER* aParser ) throw( IO_ERROR, PARSE_ERROR );

    bool        operator==( const PCB_PLOT_PARAMS &aPcbPlotParams ) const;
    bool        operator!=( const PCB_PLOT_PARAMS &aPcbPlotParams ) const;

    void        SetPlotFormat( int aFormat ) { m_PlotFormat = aFormat; };
    int         GetPlotFormat() const { return m_PlotFormat; };
    void        SetOutputDirectory( wxString aDir ) { outputDirectory = aDir; };
    wxString    GetOutputDirectory() const { return outputDirectory; };
    void        SetUseGerberExtensions( bool aUse ) { useGerberExtensions = aUse; };
    bool        GetUseGerberExtensions() const { return useGerberExtensions; };
    void        SetSubtractMaskFromSilk( bool aSubtract ) { subtractMaskFromSilk = aSubtract; };
    bool        GetSubtractMaskFromSilk() const { return subtractMaskFromSilk; };
    void        SetLayerSelection( long aSelection ) { layerSelection = aSelection; };
    long        GetLayerSelection() const { return layerSelection; };
    void        SetUseAuxOrigin( bool aAux ) { useAuxOrigin = aAux; };
    bool        GetUseAuxOrigin() const { return useAuxOrigin; };
    void        SetScaleSelection( int aSelection ) { scaleSelection = aSelection; };
    int         GetScaleSelection() const { return scaleSelection; };
    void        SetPsA4Output( int aForce ) { psA4Output = aForce; };
    bool        GetPsA4Output() const { return psA4Output; };

    int         GetHpglPenDiameter() const { return m_HPGLPenDiam; };
    bool        SetHpglPenDiameter( int aValue );
    int         GetHpglPenSpeed() const { return m_HPGLPenSpeed; };
    bool        SetHpglPenSpeed( int aValue );
    int         GetHpglPenOverlay() const { return m_HPGLPenOvr; };
    bool        SetHpglPenOverlay( int aValue );
    int         GetPlotLineWidth() const { return m_PlotLineWidth; };
    bool        SetPlotLineWidth( int aValue );
};


/**
 * Class PCB_PLOT_PARAMS_PARSER
 * is the parser class for PCB_PLOT_PARAMS.
 */
class PCB_PLOT_PARAMS_PARSER : public PCB_PLOT_PARAMS_LEXER
{
public:
    PCB_PLOT_PARAMS_PARSER( LINE_READER* aReader );
    PCB_PLOT_PARAMS_PARSER( char* aLine, wxString aSource );
    LINE_READER* GetReader() { return reader; };
    void Parse( PCB_PLOT_PARAMS* aPcbPlotParams ) throw( IO_ERROR, PARSE_ERROR );
    bool ParseBool() throw( IO_ERROR );

    /**
     * Function ParseInt
     * parses an integer and constrains it between two values.
     * @param aMin is the smallest return value.
     * @param aMax is the largest return value.
     * @return int - the parsed integer.
     */
    int ParseInt( int aMin, int aMax ) throw( IO_ERROR );
};


extern PCB_PLOT_PARAMS g_PcbPlotOptions;

#endif // PCB_PLOT_PARAMS_H_
