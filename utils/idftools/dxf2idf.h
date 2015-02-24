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

public:
    ~DXF2IDF();

    bool ReadDxf( const std::string aFile );
    bool WriteOutline( FILE* aFile, bool isInch );

private:
    // DRW_Interface implemented callback functions
    virtual void addLine(const DRW_Line& data);
    virtual void addArc(const DRW_Arc& data );
    virtual void addCircle(const DRW_Circle& data );

    // DRW_Interface callbacks unsupported by DXF2IDF
    virtual void addHeader( const DRW_Header* data ){}
    virtual void addLType( const DRW_LType& data ){}
    virtual void addLayer( const DRW_Layer& data ){}
    virtual void addDimStyle( const DRW_Dimstyle& data ){}
    virtual void addVport(const DRW_Vport& data){}
    virtual void addTextStyle(const DRW_Textstyle& data){}
    virtual void addBlock(const DRW_Block& data ){}
    virtual void setBlock(const int handle){}
    virtual void endBlock(){}
    virtual void addPoint(const DRW_Point& data ){}
    virtual void addRay(const DRW_Ray& data ){}
    virtual void addXline(const DRW_Xline& data ){}
    virtual void addEllipse(const DRW_Ellipse& data ){}
    virtual void addLWPolyline(const DRW_LWPolyline& data ){}
    virtual void addPolyline(const DRW_Polyline& data ){}
    virtual void addSpline(const DRW_Spline* data ){}
    virtual void addKnot(const DRW_Entity&){}
    virtual void addInsert(const DRW_Insert& data ){}
    virtual void addTrace(const DRW_Trace& data ){}
    virtual void add3dFace(const DRW_3Dface& data ){}
    virtual void addSolid(const DRW_Solid& data ){}
    virtual void addMText(const DRW_MText& data){}
    virtual void addText(const DRW_Text& data ){}
    virtual void addDimAlign(const DRW_DimAligned *data ){}
    virtual void addDimLinear(const DRW_DimLinear *data ){}
    virtual void addDimRadial(const DRW_DimRadial *data ){}
    virtual void addDimDiametric(const DRW_DimDiametric *data ){}
    virtual void addDimAngular(const DRW_DimAngular *data ){}
    virtual void addDimAngular3P(const DRW_DimAngular3p *data ){}
    virtual void addDimOrdinate(const DRW_DimOrdinate *data ){}
    virtual void addLeader(const DRW_Leader *data ){}
    virtual void addHatch(const DRW_Hatch* data ){}
    virtual void addViewport(const DRW_Viewport& data){}
    virtual void addImage(const DRW_Image* data ){}
    virtual void linkImage(const DRW_ImageDef* data ){}
    virtual void addComment(const char*){}
    virtual void writeHeader(DRW_Header& data){}
    virtual void writeBlocks(){}
    virtual void writeBlockRecords(){}
    virtual void writeEntities(){}
    virtual void writeLTypes(){}
    virtual void writeLayers(){}
    virtual void writeTextstyles(){}
    virtual void writeVports(){}
    virtual void writeDimstyles(){}
    virtual void addAppId( const DRW_AppId& data ) {}
    virtual void writeAppId() {}
};

#endif  // DXF2IDF_H
