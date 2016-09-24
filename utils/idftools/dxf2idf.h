/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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
#include <drw_interface.h>
#include <idf_common.h>

class DXF2IDF : public DRW_Interface
{
private:
    std::list< IDF_SEGMENT* > lines;    // Unsorted list of graphical segments
    double m_scale;                     // scaling factor to mm

    void insertLine( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd );
    void insertArc( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd, double aBulge );

public:
    DXF2IDF() : m_scale( 1.0 ) {};
    ~DXF2IDF();

    bool ReadDxf( const std::string aFile );
    bool WriteOutline( FILE* aFile, bool isInch );

private:
    // DRW_Interface implemented callback functions
    virtual void addHeader( const DRW_Header* data ) override;
    virtual void addLine(const DRW_Line& data) override;
    virtual void addArc(const DRW_Arc& data ) override;
    virtual void addCircle(const DRW_Circle& data ) override;
    virtual void addLWPolyline(const DRW_LWPolyline& data ) override;
    virtual void addPolyline(const DRW_Polyline& data ) override;

    // DRW_Interface callbacks unsupported by DXF2IDF
    virtual void addLType( const DRW_LType& data ) override {}
    virtual void addLayer( const DRW_Layer& data ) override {}
    virtual void addDimStyle( const DRW_Dimstyle& data ) override {}
    virtual void addVport(const DRW_Vport& data) override {}
    virtual void addTextStyle(const DRW_Textstyle& data) override {}
    virtual void addBlock(const DRW_Block& data ) override {}
    virtual void setBlock(const int handle) override {}
    virtual void endBlock() override {}
    virtual void addPoint(const DRW_Point& data ) override {}
    virtual void addRay(const DRW_Ray& data ) override {}
    virtual void addXline(const DRW_Xline& data ) override {}
    virtual void addEllipse(const DRW_Ellipse& data ) override {}
    virtual void addSpline(const DRW_Spline* data ) override {}
    virtual void addKnot(const DRW_Entity&) override {}
    virtual void addInsert(const DRW_Insert& data ) override {}
    virtual void addTrace(const DRW_Trace& data ) override {}
    virtual void add3dFace(const DRW_3Dface& data ) override {}
    virtual void addSolid(const DRW_Solid& data ) override {}
    virtual void addMText(const DRW_MText& data) override {}
    virtual void addText(const DRW_Text& data ) override {}
    virtual void addDimAlign(const DRW_DimAligned *data ) override {}
    virtual void addDimLinear(const DRW_DimLinear *data ) override {}
    virtual void addDimRadial(const DRW_DimRadial *data ) override {}
    virtual void addDimDiametric(const DRW_DimDiametric *data ) override {}
    virtual void addDimAngular(const DRW_DimAngular *data ) override {}
    virtual void addDimAngular3P(const DRW_DimAngular3p *data ) override {}
    virtual void addDimOrdinate(const DRW_DimOrdinate *data ) override {}
    virtual void addLeader(const DRW_Leader *data ) override {}
    virtual void addHatch(const DRW_Hatch* data ) override {}
    virtual void addViewport(const DRW_Viewport& data) override {}
    virtual void addImage(const DRW_Image* data ) override {}
    virtual void linkImage(const DRW_ImageDef* data ) override {}
    virtual void addComment(const char*) override {}
    virtual void writeHeader(DRW_Header& data) override {}
    virtual void writeBlocks() override {}
    virtual void writeBlockRecords() override {}
    virtual void writeEntities() override {}
    virtual void writeLTypes() override {}
    virtual void writeLayers() override {}
    virtual void writeTextstyles() override {}
    virtual void writeVports() override {}
    virtual void writeDimstyles() override {}
    virtual void addAppId( const DRW_AppId& data ) override {}
    virtual void writeAppId() override {}
};

#endif  // DXF2IDF_H
