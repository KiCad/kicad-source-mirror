/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <math/box2.h>
#include <base_struct.h>       // FILL_T
#include <plotter.h>


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

    virtual bool StartPlot() override;
    virtual bool EndPlot() override;

    /// HPGL doesn't handle line thickness or color
    virtual void SetCurrentLineWidth( int width, void* aData = NULL ) override
    {
        // This is the truth
        currentPenWidth = userToDeviceSize( penDiameter );
    }

    virtual void SetDash( PLOT_DASH_TYPE dashed ) override;

    virtual void SetColor( COLOR4D color ) override {}

    virtual void SetPenSpeed( int speed )
    {
        penSpeed = speed;
    }

    virtual void SetPenNumber( int number )
    {
        penNumber = number;
    }

    virtual void SetPenDiameter( double diameter );

    virtual void SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror ) override;
    virtual void Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
               int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void Circle( const wxPoint& pos, int diametre, FILL_T fill,
                         int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth = USE_DEFAULT_LINE_WIDTH,
                           void * aData = NULL) override;

    virtual void ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                               EDA_DRAW_MODE_T tracemode, void* aData ) override;
    virtual void Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int rayon, FILL_T fill, int width = USE_DEFAULT_LINE_WIDTH ) override;
    virtual void PenTo( const wxPoint& pos, char plume ) override;
    virtual void FlashPadCircle( const wxPoint& aPadPos, int aDiameter,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize, double aPadOrient,
                               EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize,
                               double aOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                    int aCornerRadius, double aOrient,
                                    EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                 SHAPE_POLY_SET* aPolygons,
                                 EDA_DRAW_MODE_T aTraceMode, void* aData ) override;
    virtual void FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                 double aPadOrient, EDA_DRAW_MODE_T aTraceMode,
                                 void* aData ) override;
    virtual void FlashRegularPolygon( const wxPoint& aShapePos, int aDiameter, int aCornerCount,
                            double aOrient, EDA_DRAW_MODE_T aTraceMode, void* aData ) override;

protected:
    void penControl( char plume );

    int    penSpeed;
    int    penNumber;
    double penDiameter;
};


