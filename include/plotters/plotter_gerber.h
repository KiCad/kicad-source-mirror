/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#pragma once

#include "plotter.h"
#include "gbr_plotter_apertures.h"

class SHAPE_ARC;
class GBR_METADATA;

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
    virtual bool StartPlot( const wxString& pageNumber ) override;
    virtual bool EndPlot() override;
    virtual void SetCurrentLineWidth( int aLineWidth, void* aData = nullptr ) override;

    // RS274X has no dashing, nor colors
    virtual void SetDash( int aLineWidth, LINE_STYLE aLineStyle ) override
    {
    }

    virtual void SetColor( const COLOR4D& aColor ) override {}

    // Currently, aScale and aMirror are not used in gerber plotter
    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;

    // Basic plot primitives

    // Note: do not use Rect() to plot rounded-corner rectangles.  Use PlotPolyAsRegion() instead.
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius = 0 ) override;

    virtual void Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width ) override;

    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth ) override;

    // These functions plot an item and manage X2 gerber attributes
    virtual void ThickSegment( const VECTOR2I& start, const VECTOR2I& end, int width,
                               void* aData ) override;

    virtual void ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width,
                            void* aData ) override;

    virtual void ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData ) override;

    virtual void FilledCircle( const VECTOR2I& pos, int diametre, void* aData ) override;

    virtual void ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData ) override;

    /**
     * Gerber polygon: they can (and *should*) be filled with the
     * appropriate G36/G37 sequence
     */
    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                           void* aData ) override;

    virtual void PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_T aFill, int aWidth,
                           void* aData ) override;

    /**
     * Similar to PlotPoly(), plot a filled polygon using Gerber region,
     * therefore adding X2 attributes to the region object, like TA.xxx
     */
    void PlotPolyAsRegion( const SHAPE_LINE_CHAIN& aPoly, FILL_T aFill,
                           int aWidth, GBR_METADATA* aGbrMetadata );

    virtual void PenTo( const VECTOR2I& pos, char plume ) override;

    virtual void Text( const VECTOR2I&        aPos,
                       const COLOR4D&         aColor,
                       const wxString&        aText,
                       const EDA_ANGLE&       aOrient,
                       const VECTOR2I&        aSize,
                       enum GR_TEXT_H_ALIGN_T aH_justify,
                       enum GR_TEXT_V_ALIGN_T aV_justify,
                       int                    aWidth,
                       bool                   aItalic,
                       bool                   aBold,
                       bool                   aMultilineAllowed,
                       KIFONT::FONT*          aFont,
                       const KIFONT::METRICS& aFontMetrics,
                       void*                  aData = nullptr ) override;


    virtual void PlotText( const VECTOR2I&        aPos,
                           const COLOR4D&         aColor,
                           const wxString&        aText,
                           const TEXT_ATTRIBUTES& aAttributes,
                           KIFONT::FONT*          aFont,
                           const KIFONT::METRICS& aFontMetrics,
                           void*                  aData = nullptr ) override;

    /**
     * Filled circular flashes are stored as apertures
     */
    virtual void FlashPadCircle( const VECTOR2I& pos, int diametre, void* aData ) override;

    virtual void FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aOrient, void* aData ) override;

    virtual void FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aOrient, void* aData ) override;

    virtual void FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                    int aCornerRadius, const EDA_ANGLE& aOrient,
                                    void* aData ) override;
    virtual void FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                 const EDA_ANGLE& aPadOrient, SHAPE_POLY_SET* aPolygons,
                                 void* aData ) override;

    virtual void FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                 const EDA_ANGLE& aPadOrient, void* aData ) override;

    virtual void FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                      const EDA_ANGLE& aOrient, void* aData ) override;

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
     * @param aPadOrient is the rotation of the shape.
     * @param aData is the a reference to Gerber attributes descr.
     */
    void FlashPadChamferRoundRect( const VECTOR2I& aShapePos, const VECTOR2I& aPadSize,
                                   int aCornerRadius, double aChamferRatio, int aChamferPositions,
                                   const EDA_ANGLE& aPadOrient, void* aData );

    /**
     * Plot a Gerber region: similar to PlotPoly but plot only filled polygon,
     * and add the TA.AperFunction if aGbrMetadata contains this attribute, and clear it
     * after plotting.
     */
    void PlotGerberRegion( const std::vector<VECTOR2I>& aCornerList, GBR_METADATA* aGbrMetadata );

    void PlotGerberRegion( const SHAPE_LINE_CHAIN& aPoly, GBR_METADATA* aGbrMetadata );

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
     * @param aRotation is the rotation of tool (primitives round, oval rect accept only 0.0).
     * @param aType is the type ( shape ) of tool.
     * @param aApertureAttribute is an aperture attribute of the tool (a tool can have only one
     *                           attribute) 0 = no specific attribute.
     * @param aCustomAttribute a String describing custom tools
     * @return an index to the aperture in aperture list which meets the size and type of tool
     *         if the aperture does not exist, it is created and entered in aperture list.
     */
    int GetOrCreateAperture( const VECTOR2I& aSize, int aRadius, const EDA_ANGLE& aRotation,
                             APERTURE::APERTURE_TYPE aType, int aApertureAttribute,
                             const std::string& aCustomAttribute );

    /**
     * @param aCorners is the corner list.
     * @param aRotation is the rotation of tool.
     * @param aType is the type ( shape ) of tool that can manage a list of corners (polygon).
     * @param aApertureAttribute is an aperture attribute of the tool (a tool can have only one
     *        attribute) 0 = no specific attribute.
     * @param aCustomAttribute a String describing custom tools
     * @return an index to the aperture in aperture list which meets the data and type of tool
     *         if the aperture does not exist, it is created and entered in aperture list.
     */
    int GetOrCreateAperture( const std::vector<VECTOR2I>& aCorners, const EDA_ANGLE& aRotation,
                             APERTURE::APERTURE_TYPE aType, int aApertureAttribute,
                             const std::string& aCustomAttribute );

protected:
    virtual void ThickArc( const VECTOR2D& aCentre, const EDA_ANGLE& aStartAngle,
                           const EDA_ANGLE& aAngle, double aRadius, int aWidth, void* aData ) override;

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
    void plotRoundRectAsRegion( const VECTOR2I& aRectCenter, const VECTOR2I& aSize,
                                int aCornerRadius, const EDA_ANGLE& aOrient );
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
    void plotArc( const VECTOR2I& aCenter, const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle,
                  double aRadius, bool aPlotInRegion );
    void plotArc( const SHAPE_ARC& aArc, bool aPlotInRegion );

    /**
     * Pick an existing aperture or create a new one, matching the size, type and attributes.
     *
     * Write the DCode selection on gerber file.
     */
    void selectAperture( const VECTOR2I& aSize, int aRadius, const EDA_ANGLE& aRotation,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute,
                         const std::string& aCustomAttribute );
    /**
     * Pick an existing aperture or create a new one, matching the aDiameter, aPolygonRotation,
     * type and attributes.
     *
     * It apply only to apertures with type = AT_REGULAR_POLY3 to AT_REGULAR_POLY12
     * write the DCode selection on gerber file
     */
    void selectAperture( const std::vector<VECTOR2I>& aCorners, const EDA_ANGLE& aPolygonRotation,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute,
                         const std::string& aCustomAttribute );


    /**
     * Pick an aperture or create a new one and emits the DCode.
     * 
     */
    void selectApertureWithAttributes( const VECTOR2I& aPos, GBR_METADATA* aGbrMetadata,
                                       VECTOR2I aSize, int aRadius, const EDA_ANGLE& aAngle,
                                       APERTURE::APERTURE_TYPE aType );

    /**
     * Emit a D-Code record, using proper conversions to format a leading zero omitted gerber
     * coordinate.
     *
     * For n decimal positions, see header generation in start_plot.
     */
    void emitDcode( const VECTOR2D& pt, int dcode );

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

    // the attributes dictionary created/modified by %TO, attached to objects, when they are created
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
