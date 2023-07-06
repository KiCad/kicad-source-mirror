/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Plotting engine (HPGL)
 *
 * @file plotter_hpgl.h
 */

#pragma once

#include <list>

#include "plotter.h"


class HPGL_PLOTTER : public PLOTTER
{
public:
    HPGL_PLOTTER();

    virtual PLOT_FORMAT GetPlotterType() const override
    {
        return PLOT_FORMAT::HPGL;
    }

    static wxString GetDefaultFileExtension()
    {
        return wxString( wxT( "plt" ) );
    }

    /**
     * Set the target length of chords used to draw approximated circles and arcs.
     *
     * @param chord_len the chord length in IUs.
     */
    void SetTargetChordLength( double chord_len );

    /// Switch to the user coordinate system
    void SetUserCoords( bool user_coords ) { m_useUserCoords = user_coords; }

    /// Set whether the user coordinate system is fit to content
    void SetUserCoordsFit( bool user_coords_fit ) { m_fitUserCoords = user_coords_fit; }

    /**
     * At the start of the HPGL plot pen speed and number are requested.
     */
    virtual bool StartPlot( const wxString& aPageNumber ) override;

    /**
     * HPGL end of plot: sort and emit graphics, pen return and release.
     */
    virtual bool EndPlot() override;

    /// HPGL doesn't handle line thickness or color
    virtual void SetCurrentLineWidth( int width, void* aData = nullptr ) override
    {
        // This is the truth
        m_currentPenWidth = userToDeviceSize( m_penDiameter );
    }

    /**
     * HPGL supports dashed lines.
     */
    virtual void SetDash( int aLineWidth, PLOT_DASH_TYPE aLineStyle ) override;

    virtual void SetColor( const COLOR4D& color ) override {}

    virtual void SetPenSpeed( int speed )
    {
        m_penSpeed = speed;
    }

    virtual void SetPenNumber( int number )
    {
        m_penNumber = number;
    }

    virtual void SetPenDiameter( double diameter );

    virtual void SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror ) override;
    virtual void Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T aFill,
                       int aWidth = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const VECTOR2I& aCenter, int aDiameter, FILL_T aFill,
                         int aWidth = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill,
                           int aWidth = USE_DEFAULT_LINE_WIDTH, void* aData = nullptr ) override;

    virtual void ThickSegment( const VECTOR2I& start, const VECTOR2I& end, int width,
                               OUTLINE_MODE tracemode, void* aData ) override;

    virtual void Arc( const VECTOR2I& aCenter, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                      FILL_T aFill, int aWidth, int aMaxError ) override;

    virtual void PenTo( const VECTOR2I& pos, char plume ) override;

    virtual void FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter,
                                 OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aPadOrient, OUTLINE_MODE aTraceMode,
                               void* aData ) override;
    virtual void FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                               const EDA_ANGLE& aOrient, OUTLINE_MODE aTraceMode,
                               void* aData ) override;
    virtual void FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                    int aCornerRadius, const EDA_ANGLE& aOrient,
                                    OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                 const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                 OUTLINE_MODE aTraceMode, void* aData ) override;
    virtual void FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                 const EDA_ANGLE& aPadOrient, OUTLINE_MODE aTraceMode,
                                 void* aData ) override;
    virtual void FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                      const EDA_ANGLE& aOrient, OUTLINE_MODE aTraceMode,
                                      void* aData ) override;

protected:
    /**
     * Plot an arc.
     *
     * Command
     * PU PY x, y; PD start_arc_X AA, start_arc_Y, angle, NbSegm; PU;
     * Or PU PY x, y; PD start_arc_X AA, start_arc_Y, angle, PU;
     *
     * center is the center of the arc.
     * StAngled is the start angle of the arc.
     * aEndAngle is end angle the arc.
     * Radius is the radius of the arc.
     */
    virtual void Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aEndAngle, double aRadius, FILL_T aFill,
                      int aWidth = USE_DEFAULT_LINE_WIDTH ) override;

    /**
     * Start a new HPGL_ITEM if necessary, keeping the current one if it exists.
     *
     * @param location is the location of the item.
     * @return whether a new item was made.
     */
    bool startItem( const VECTOR2D& location );

    /// Flush the current HPGL_ITEM and clear out the current item pointer.
    void flushItem();

    /**
     * Start a new HPGL_ITEM with the given string if necessary, or append the
     * string to the current item.
     *
     * @param location is the location of the item, if a new one is made.
     * @param content is the content substring.
     * @return whether a new item was made.
     */
    bool startOrAppendItem( const VECTOR2D& location, const wxString& content );

    struct HPGL_ITEM
    {
        HPGL_ITEM() :
            lift_before( false ),
            lift_after( false ),
            pen_returns( false ),
            pen( 0 ),
            dashType( PLOT_DASH_TYPE::SOLID ) {}

        /// Location the pen should start at
        VECTOR2D       loc_start;

        /// Location the pen will be at when it finishes. If this is not known,
        /// leave it equal to loc_start and set lift_after.
        VECTOR2D       loc_end;

        /// Bounding box of this item
        BOX2D          bbox;

        /// Whether the command should be executed with the pen lifted
        bool           lift_before;

        /// Whether the pen must be lifted after the command. If the location of the pen
        /// is not known, this must be set (so that another command starting at loc_end
        /// is not immediately executed with no lift).
        bool           lift_after;

        /// Whether the pen returns to its original state after the command. Otherwise,
        /// the pen is assumed to be down following the command.
        bool           pen_returns;

        int            pen;            /// Pen number for this command
        PLOT_DASH_TYPE dashType;       /// Line style for this command
        wxString       content;        /// Text of the command
    };

    /// Sort a list of HPGL items to improve plotting speed on mechanical plotters.
    ///
    /// @param items - items to sort
    static void sortItems( std::list<HPGL_ITEM>& items );

    /// Return the plot command corresponding to a line type
    static const char* lineStyleCommand( PLOT_DASH_TYPE aLineStyle );

protected:
    int                  m_penSpeed;
    int                  m_penNumber;
    double               m_penDiameter;
    double               m_arcTargetChordLength;
    EDA_ANGLE            m_arcMinChordDegrees;
    PLOT_DASH_TYPE       m_lineStyle;
    bool                 m_useUserCoords;
    bool                 m_fitUserCoords;

    std::list<HPGL_ITEM> m_items;
    HPGL_ITEM*           m_current_item;
};
