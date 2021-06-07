/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * Plotting engine (Gerber)
 *
 * @file plotter_gerber.h
 */

#pragma once

#include <vector>
#include <math/box2.h>
#include <eda_item.h>       // FILL_TYPE

#include <plotter.h>
#include "gbr_plotter_apertures.h"

class SHAPE_ARC;

class GERBER_PLOTTER : public PLOTTER
{
public:
    GERBER_PLOTTER();

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::GERBER;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "gbr" ) );
    }

    /**
     * Write GERBER header to file initialize global variable g_Plot_PlotOutputFile.
     */
    virtual bool StartPlot() override;
    virtual bool EndPlot() override;
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override;

    // RS274X has no dashing, nor colors
    virtual void SetDash( PLOT_DASH_TYPE dashed ) override
    {
    }

    virtual void SetColor( const COLOR4D& color ) override {}

    // Currently, aScale and aMirror are not used in gerber plotter
    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;

    // Basic plot primitives
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill,
                       int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_TYPE fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Arc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                      int aRadius, FILL_TYPE aFill, int aWidth = USE_DEFAULT_LINE_WIDTH ) override;

    virtual void Arc( const SHAPE_ARC& aArc ) override;

    // These functions plot an item and manage X2 gerber attributes
    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               OUTLINE_MODE tracemode, void* aData ) override;

    virtual void ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                           int rayon, int width, OUTLINE_MODE tracemode, void* aData ) override;
    virtual void ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            OUTLINE_MODE tracemode, void* aData ) override;
    virtual void ThickCircle( const wxPoint& pos, int diametre, int width,
                              OUTLINE_MODE tracemode, void* aData ) override;
    virtual void FilledCircle( const wxPoint& pos, int diametre,
                              OUTLINE_MODE tracemode, void* aData ) override;

    /**
     * Gerber polygon: they can (and *should*) be filled with the
     * appropriate G36/G37 sequence
     */
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList, FILL_TYPE aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_TYPE aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    virtual void PenTo( const wxPoint& pos, char plume ) override;

    virtual void Text( const wxPoint&              aPos,
                       const COLOR4D&              aColor,
                       const wxString&             aText,
                       double                      aOrient,
                       const wxSize&               aSize,
                       enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                       enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                       int                         aWidth,
                       bool                        aItalic,
                       bool                        aBold,
                       bool                        aMultilineAllowed = false,
                       void* aData = nullptr ) override;

    /**
     * Filled circular flashes are stored as apertures
     */
    virtual void FlashPadCircle( const wxPoint& pos, int diametre,
                                 OUTLINE_MODE trace_mode, void* aData ) override;

    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& size, double orient,
                               OUTLINE_MODE trace_mode, void* aData ) override;

    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& size,
                               double orient, OUTLINE_MODE trace_mode, void* aData ) override;

    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 double aPadOrient, SHAPE_POLY_SET* aPolygons,
                                 OUTLINE_MODE aTraceMode, void* aData ) override;

    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                            double aPadOrient, OUTLINE_MODE aTraceMode, void* aData ) override;

    virtual void FlashRegularPolygon( const wxPoint& aShapePos, int aDiameter, int aCornerCount,
                            double aOrient, OUTLINE_MODE aTraceMode, void* aData ) override;

    /**
     * Flash a chamfered round rect pad.
     *
     * @param aShapePos is the position of the pad shape.
     * @param aPadSize is the size of the rectangle.
     * @param aCornerRadius is the radius of rounded corners.
     * @param aChamferRatio is the chamfer value (ratio < 0.5 between smallest size and chamfer).
     * @param aChamferPositions is the identifier of the corners to chamfer:
     *  0 = no chamfer
     *  1 = TOP_LEFT
     *  2 = TOP_RIGHT
     *  4 = BOTTOM_LEFT
     *  8 = BOTTOM_RIGHT
     * @param aPadOrient is the rotation in 0.1 degrees of the shape.
     * @param aPlotMode is the drawing mode, FILLED or SKETCH.
     * @param aData is the a reference to Gerber attributes descr.
     */
    void FlashPadChamferRoundRect( const wxPoint& aShapePos, const wxSize& aPadSize,
                                   int aCornerRadius, double aChamferRatio,
                                   int aChamferPositions, double aPadOrient,
                                   OUTLINE_MODE aPlotMode, void* aData );

    /**
     * Plot a Gerber region: similar to PlotPoly but plot only filled polygon,
     * and add the TA.AperFunction if aData contains this attribute, and clear it
     * after plotting.
     */
    void PlotGerberRegion( const std::vector< wxPoint >& aCornerList, void* aData = nullptr );

    void PlotGerberRegion( const SHAPE_LINE_CHAIN& aPoly, void* aData = nullptr );

    /**
     * Change the plot polarity and begin a new layer.
     *
     * Used to 'scratch off' silk screen away from solder mask.
     */
    virtual void SetLayerPolarity( bool aPositive ) override;

    /**
     * Selection of Gerber units and resolution (number of digits in mantissa).
     *
     * Should be called only after SetViewport() is called.
     *
     * @param aResolution is the number of digits in mantissa of coordinate
     *                    use 5 or 6 for mm and 6 or 7 for inches
     *                    do not use value > 6 (mm) or > 7 (in) to avoid overflow.
     * @param aUseInches use true to use inches, false to use mm (default).
     */
    virtual void SetGerberCoordinatesFormat( int aResolution, bool aUseInches = false ) override;

    void UseX2format( bool aEnable ) { m_useX2format = aEnable; }
    void UseX2NetAttributes( bool aEnable ) { m_useNetAttributes = aEnable; }

    /**
     * Disable Aperture Macro (AM) command, only for broken Gerber Readers.
     *
     * Regions will be used instead of AM shapes to draw complex shapes.
     *
     * @param aDisable use true to disable Aperture Macro (AM) command.
     */
    void DisableApertMacros( bool aDisable ) { m_gerberDisableApertMacros = aDisable; }

    /**
     * Calling this function allows one to define the beginning of a group
     * of drawing items (used in X2 format with netlist attributes).
     *
     * @param aData can define any parameter.
     */
    virtual void StartBlock( void* aData ) override;

    /**
     * Define the end of a group of drawing items the group is started by StartBlock().
     *
     * Used in X2 format with netlist attributes.
     *
     * @param aData can define any parameter
     */
    virtual void EndBlock( void* aData ) override;

    /**
     * Remove (clear) all attributes from object attributes dictionary (TO. and TA commands)
     * similar to clearNetAttribute(), this is an unconditional reset of TO. and TA. attributes.
     */
    void ClearAllAttributes();

    /**
     * @param aSize is the size of tool.
     * @param aRadius is the radius used for some shapes tool (oval, roundrect macros).
     * @param aRotDegree is the rotation of tool (primitives round, oval rect accept only 0.0).
     * @param aType is the type ( shape ) of tool.
     * @param aApertureAttribute is an aperture attribute of the tool (a tool can have only one
     *                           attribute) 0 = no specific attribute.
     * @return an index to the aperture in aperture list which meets the size and type of tool
     *         if the aperture does not exist, it is created and entered in aperture list.
     */
    int GetOrCreateAperture( const wxSize& aSize, int aRadius, double aRotDegree,
                             APERTURE::APERTURE_TYPE aType, int aApertureAttribute );

    /**
     * @param aCorners is the corner list.
     * @param aRotDegree is the rotation of tool.
     * @param aType is the type ( shape ) of tool that can manage a list of corners (polygon).
     * @param aApertureAttribute is an aperture attribute of the tool (a tool can have only one
     *        attribute) 0 = no specific attribute.
     * @return an index to the aperture in aperture list which meets the data and type of tool
     *         if the aperture does not exist, it is created and entered in aperture list.
     */
    int GetOrCreateAperture( const std::vector<wxPoint>& aCorners, double aRotDegree,
                             APERTURE::APERTURE_TYPE aType, int aApertureAttribute );

protected:
    /**
     * Plot a round rect (a round rect shape in fact) as a Gerber region using lines and arcs
     * for corners.
     *
     * @note Only the G36 ... G37 region is created.
     *
     * @param aRectCenter is the center of the rectangle.
     * @param aSize is the size of the rectangle.
     * @param aCornerRadius is the radius of the corners.
     * @param aOrient is the rotation of the rectangle.
     */
    void plotRoundRectAsRegion( const wxPoint& aRectCenter, const wxSize& aSize,
                                int aCornerRadius, double aOrient );
    /**
     * Plot a Gerber arc.
     *
     * If aPlotInRegion = true, the current pen position will not be initialized to the arc
     * start position, and therefore the arc can be used to define a region outline item
     * a line will be created from current position to arc start point.  If aPlotInRegion
     * = false, the current pen position will be initialized to the arc start position, to
     * plot an usual arc item.  The line thickness is not initialized in plotArc, and must
     * be initialized before calling it if needed.
     */
    void plotArc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                  int aRadius, bool aPlotInRegion );
    void plotArc( const SHAPE_ARC& aArc, bool aPlotInRegion );

    /**
     * Pick an existing aperture or create a new one, matching the size, type and attributes.
     *
     * Write the DCode selection on gerber file.
     */
    void selectAperture( const wxSize& aSize, int aRadius, double aRotDegree,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute );
    /**
     * Pick an existing aperture or create a new one, matching the aDiameter, aPolygonRotation,
     * type and attributes.
     *
     * It apply only to apertures with type = AT_REGULAR_POLY3 to AT_REGULAR_POLY12
     * write the DCode selection on gerber file
     */
    void selectAperture( const std::vector<wxPoint>& aCorners, double aPolygonRotation,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute );

    /**
     * Pick an existing aperture or create a new one, matching the corner list, aRotDegree,
     * type and attributes.
     *
     * It only applies to apertures managing a polygon that differs from AT_REGULAR_POLY3
     * to AT_REGULAR_POLY12 (for instance APER_MACRO_TRAPEZOID ) write the DCode selection
     * on gerber file.
     */
    void selectAperture( int aDiameter, double aRotDegree,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute );

    /**
     * Emit a D-Code record, using proper conversions to format a leading zero omitted gerber
     * coordinate.
     *
     * For n decimal positions, see header generation in start_plot.
     */
    void emitDcode( const DPOINT& pt, int dcode );

    /**
     * Print a Gerber net attribute object record.
     *
     * In a gerber file, a net attribute is owned by a graphic object formatNetAttribute must
     * be called before creating the object.  The generated string depends on the type of
     * netlist info.
     *
     * @param aData contains the data to format.
     */
    void formatNetAttribute( GBR_NETLIST_METADATA* aData );

    /**
     * Clear a Gerber net attribute record (clear object attribute dictionary)
     * and output the clear object attribute dictionary command to gerber file
     * has effect only if a net attribute is stored in m_objectAttributesDictionary.
     */
    void clearNetAttribute();

    // the attributes dictionary created/modifed by %TO, attached to objects, when they are created
    // by D01, D03, G36/G37 commands
    // standard attributes are .P, .C and .N
    // this is used by gerber readers when creating a new object. Cleared by %TD command
    // Note: m_objectAttributesDictionary can store more than one attribute
    // the string stores the line(s) actually written to the gerber file
    // it can store a .P, .C or .N attribute, or 2 or 3 attributes, separated by a \n char (EOL)
    std::string   m_objectAttributesDictionary;

    // The last aperture attribute generated (only one aperture attribute can be set)
    int           m_apertureAttribute;

    FILE* workFile;
    FILE* finalFile;
    wxString m_workFilename;

    /**
     * Generate the table of D codes
     */
    void writeApertureList();

    std::vector<APERTURE> m_apertures;  // The list of available apertures
    int     m_currentApertureIdx;       // The index of the current aperture in m_apertures
    bool    m_hasApertureRoundRect;     // true is at least one round rect aperture is in use
    bool    m_hasApertureRotOval;       // true is at least one oval rotated aperture is in use
    bool    m_hasApertureRotRect;       // true is at least one rect. rotated aperture is in use
    bool    m_hasApertureOutline4P;     // true is at least one 4 corners outline (free polygon
                                        // with 4 corners) aperture is in use
    bool    m_hasApertureChamferedRect; // true is at least one chamfered rect is in use
                                        // (with no rounded corner)

    bool    m_gerberUnitInch;          // true if the gerber units are inches, false for mm
    int     m_gerberUnitFmt;           // number of digits in mantissa.
                                       // usually 6 in Inches and 5 or 6  in mm
    bool    m_gerberDisableApertMacros; // True to disable Aperture Macro (AM) command,
                                       // for broken Gerber Readers
                                       // Regions will be used instead of AM shapes
    bool    m_useX2format;             // Add X2 file header attributes.  If false, attributes
                                       // will be added as comments.
    bool    m_useNetAttributes;        // In recent gerber files, netlist info can be added.
                                       // It will be added if this param is true, using X2 or
                                       // X1 format

    // A list of aperture macros defined "on the fly" because the number of parameters is not
    // defined: this is the case of the macro using the primitive 4 to create a polygon.
    // The number of vertices is not known for free polygonal shapes, and an aperture macro
    // must be created for each specific polygon
    APER_MACRO_FREEPOLY_LIST m_am_freepoly_list;
};
