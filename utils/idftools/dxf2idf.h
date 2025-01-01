/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DXF2IDF_H
#define DXF2IDF_H

#include <string>
#include "dl_dxf.h"
#include "dl_creationadapter.h"
#include <idf_common.h>

class DXF2IDF : public DL_CreationAdapter
{
public:
    DXF2IDF() : m_scale( 1.0 ), m_entityType( 0 ), m_entityParseStatus( 0 ),
                m_entity_flags( 0 ), m_bulgeVertex( 0.0 ) {};
    ~DXF2IDF();

    bool ReadDxf( const std::string& aFile );
    bool WriteOutline( FILE* aFile, bool isInch );

private:
    // DRW_Interface implemented callback functions
    virtual void addLine( const DL_LineData& aData ) override;
    virtual void addArc( const DL_ArcData& aData ) override;
    virtual void addCircle( const DL_CircleData& aData ) override;
    virtual void addPolyline( const DL_PolylineData& aData ) override;

    /**
     * Called for every polyline vertex.
     */
    virtual void addVertex( const DL_VertexData& aData ) override;

    /**
     * Called for every string variable in the DXF file (e.g. "$ACADVER").
     */
    virtual void setVariableString( const std::string& key, const std::string& value,
                                    int code ) override {};

    /**
     * Called for every int variable in the DXF file (e.g. "$ACADMAINTVER").
     */
    virtual void setVariableInt( const std::string& key, int value, int code ) override;

    /**
     * Called for every double variable in the DXF file (e.g. "$DIMEXO").
     */
    virtual void setVariableDouble( const std::string& key, double value, int code ) override {}

    virtual void endEntity() override;

private:
    std::list< IDF_SEGMENT* > lines;    // Unsorted list of graphical segments
    double m_scale;                     // scaling factor to mm
    int m_entityType;                   // the DXF type of entity
    int m_entityParseStatus;            // Inside a entity: status of parsing:
                                        // 0 = no entity
                                        // 1 = first item of entity
                                        // 2 = entity in progress
    int m_entity_flags;                 // state of flags read from last entity
    IDF_POINT m_lastCoordinate;         // the last vertex coordinate read (unit = mm)

    // The first point of the polyline entity, when reading a polyline (unit = mm).
    IDF_POINT m_polylineStart;
    double m_bulgeVertex;               // the last vertex bulge value read

    void insertLine( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd );
    void insertArc( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd, double aBulge );
};

#endif  // DXF2IDF_H
