/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef OCE_VIS_OCE_UTILS_H
#define OCE_VIS_OCE_UTILS_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "base.h"
#include "kicadpcb.h"
#include "kicadcurve.h"

#include <BRepBuilderAPI_MakeWire.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>


typedef std::pair< std::string, TDF_Label > MODEL_DATUM;
typedef std::map< std::string, TDF_Label > MODEL_MAP;

class KICADPAD;

class OUTLINE
{
public:
    OUTLINE();
    virtual ~OUTLINE();

    void Clear();

    // attempt to add a curve to the outline; on success returns true
    bool AddSegment( const KICADCURVE& aCurve );

    bool IsClosed()
    {
        return m_closed;
    }

    void SetMinSqDistance( double aDistance )
    {
        m_minDistance2 = aDistance;
    }

    bool MakeShape( TopoDS_Shape& aShape, double aThickness );

private:
    bool addEdge( BRepBuilderAPI_MakeWire* aWire, KICADCURVE& aCurve, DOUBLET& aLastPoint );
    bool testClosed( const KICADCURVE& aFrontCurve, const KICADCURVE& aBackCurve );

public:
    std::list< KICADCURVE > m_curves;   // list of contiguous segments

private:
    bool   m_closed;        // set true if the loop is closed
    double m_minDistance2;  // min squared distance to treat points as separate entities (mm)
};


class PCBMODEL
{
public:
    PCBMODEL();
    virtual ~PCBMODEL();

    // add an outline segment (must be in final position)
    bool AddOutlineSegment( KICADCURVE* aCurve );

    // add a pad hole or slot (must be in final position)
    bool AddPadHole( const KICADPAD* aPad );

    // add a component at the given position and orientation
    bool AddComponent( const std::string& aFileName, const std::string& aRefDes,
        bool aBottom, DOUBLET aPosition, double aRotation,
        TRIPLET aOffset, TRIPLET aOrientation, TRIPLET aScale,
        bool aSubstituteModels = true );

    // set the thickness of the PCB (mm); the top of the PCB shall be at Z = aThickness
    // aThickness < 0.0 == use default thickness
    // aThickness <= THICKNESS_MIN == use THICKNESS_MIN
    // aThickness > THICKNESS_MIN == use aThickness
    void SetPCBThickness( double aThickness );

    // Set the minimum distance (in mm) to consider 2 points have the same coordinates
    void SetMinDistance( double aDistance );

    // create the PCB model using the current outlines and drill holes
    bool CreatePCB();

#ifdef SUPPORTS_IGES
    // write the assembly model in IGES format
    bool WriteIGES( const wxString& aFileName );
#endif

    // write the assembly model in STEP format
    bool WriteSTEP( const wxString& aFileName );

private:
    /**
     * Load a 3D model data.
     *
     * @param aFileName is the filename (different formats allowed) but for WRML files a model
     *        data can be loaded instead of the vrml data, not suitable in a step file.
     * @param aScale is the X,Y,Z scaling factors.
     * @param aLabel is the TDF_Label to store the data.
     * @param aSubstituteModels = true to allows data substitution, false to disallow.
     * @param aErrorMessage (can be nullptr) is an error message to be displayed on error.
     * @return true if successfully loaded, false on error.
     */
    bool getModelLabel( const std::string& aFileName, TRIPLET aScale, TDF_Label& aLabel,
                        bool aSubstituteModels, wxString* aErrorMessage = nullptr );

    bool getModelLocation( bool aBottom, DOUBLET aPosition, double aRotation, TRIPLET aOffset,
                           TRIPLET aOrientation, TopLoc_Location& aLocation );

    bool readIGES( Handle( TDocStd_Document )& m_doc, const char* fname );
    bool readSTEP( Handle( TDocStd_Document )& m_doc, const char* fname );

    TDF_Label transferModel( Handle( TDocStd_Document )& source,
                             Handle( TDocStd_Document )& dest, TRIPLET aScale );

    Handle( XCAFApp_Application )   m_app;
    Handle( TDocStd_Document )      m_doc;
    Handle( XCAFDoc_ShapeTool )     m_assy;
    TDF_Label                       m_assy_label;
    bool                            m_hasPCB;       // set true if CreatePCB() has been invoked
    TDF_Label                       m_pcb_label;    // label for the PCB model
    MODEL_MAP                       m_models;       // map of file names to model labels
    int                             m_components;   // number of successfully loaded components;
    double                          m_precision;    // model (length unit) numeric precision
    double                          m_angleprec;    // angle numeric precision
    double                          m_thickness;    // PCB thickness, mm

    // minimum X value in curves (leftmost curve feature).
    double                          m_minx;
    double                          m_minDistance2; // minimum squared distance between items (mm)
    std::list<KICADCURVE>::iterator m_mincurve;     // iterator to the leftmost curve

    std::list<KICADCURVE>           m_curves;
    std::vector<TopoDS_Shape>       m_cutouts;
};

#endif // OCE_VIS_OCE_UTILS_H
