/****************************************************************************
**
** This file comes from the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**********************************************************************/


#ifndef FILTERDXFRW_H
#define FILTERDXFRW_H

#include "drw_interface.h"
#include "wx/wx.h"

class BOARD;
class BOARD_ITEM;

/**
 * This format filter class can import and export DXF files.
 * It depends on the dxflib library.
 *
 * @author Rallaz
 */
class DXF2BRD_CONVERTER : public DRW_Interface
{
private:
    std::vector<BOARD_ITEM*> m_newItemsList;    // The list of new items added
                                                // to the board
    BOARD * m_brd;
    double m_xOffset;       // X coord offset for conversion (in mm)
    double m_yOffset;       // Y coord offset for conversion (in mm)
    double m_defaultThickness;  // default line thickness for conversion (in mm)
    double m_Dfx2mm;        // The scale factor to convert DXF units to mm
                            // Seems DRW_Interface always converts DXF coordinates in mm
                            // (to be confirmed)
    int    m_brdLayer;      // The board layer to place imported dfx items
    int m_version;          // the dxf version, not used here
    std::string m_codePage; // The code page, not used here

public:
    DXF2BRD_CONVERTER();
    ~DXF2BRD_CONVERTER();

    /**
     * Set the coordinate offset between the importede dxf items
     * and Pcbnew.
     * because dxf files have the Y axis from bottom to top;
     * aOffsetX = 0, and aOffsetY = - vertical page size to import a full page
     * @param aOffsetX = the X offset in mm
     * @param aOffsetY = the Y offset in mm
     */
    void SetOffset( double aOffsetX, double aOffsetY )
    {
        m_xOffset = aOffsetX;
        m_yOffset = aOffsetY;
    }

    /**
     * Set the layer number to import dxf items.
     * the layer should be a techicanl layer, not a copper layer
     */
    void SetBrdLayer( int aBrdLayer ) { m_brdLayer = aBrdLayer; }

    /**
     * Implementation of the method used for communicate
     * with this filter.
     *
     * @param aFile = the full filename.
     * @param aBoard = where to store the graphical items and text
     */
    bool ImportDxfFile(  const wxString& aFile, BOARD * aBoard );

    /**
     * @return the list of new BOARD_ITEM
     */
    std::vector<BOARD_ITEM*>& GetItemsList()
    {
        return m_newItemsList;
    }

private:
    // coordinate conversions from dxf to internal units
    int mapX( double aDxfCoordX );
    int mapY( double aDxfCoordY );
    int mapDim( double aDxfValue );

    // Add aItem the the board
    // this item is also added to the list of new items
    // (for undo command for instance)
    void appendToBoard( BOARD_ITEM * aItem );

    // Methods from DRW_CreationInterface:
    // They are "call back" fonctions, called when the corresponding object
    // is read in dxf file
    // Depending of the application, they can do something or not
    virtual void addHeader( const DRW_Header* data );
    virtual void addLType( const DRW_LType& data ){}
    virtual void addLayer( const DRW_Layer& data );
    virtual void addDimStyle( const DRW_Dimstyle& data ){}
    virtual void addBlock(const DRW_Block& data ){}
    virtual void endBlock(){}
    virtual void addPoint(const DRW_Point& data ){}
    virtual void addLine(const DRW_Line& data);
    virtual void addRay(const DRW_Ray& data ){}
    virtual void addXline(const DRW_Xline& data ){}
    virtual void addCircle(const DRW_Circle& data );
    virtual void addArc(const DRW_Arc& data );
    virtual void addEllipse(const DRW_Ellipse& data ){}
    virtual void addLWPolyline(const DRW_LWPolyline& data );
    virtual void addText(const DRW_Text& data );
    virtual void addPolyline(const DRW_Polyline& data );
    virtual void addSpline(const DRW_Spline* data ){}
    virtual void addKnot(const DRW_Entity&) {}
    virtual void addInsert(const DRW_Insert& data ){}
    virtual void addTrace(const DRW_Trace& data ){}
    virtual void addSolid(const DRW_Solid& data ){}
    virtual void addMText(const DRW_MText& data);
    virtual void addDimAlign(const DRW_DimAligned *data ){}
    virtual void addDimLinear(const DRW_DimLinear *data ){}
    virtual void addDimRadial(const DRW_DimRadial *data ){}
    virtual void addDimDiametric(const DRW_DimDiametric *data ){}
    virtual void addDimAngular(const DRW_DimAngular *data ){}
    virtual void addDimAngular3P(const DRW_DimAngular3p *data ){}
    virtual void addDimOrdinate(const DRW_DimOrdinate *data ){}
    virtual void addLeader(const DRW_Leader *data ){}
    virtual void addHatch(const DRW_Hatch* data ){}
    virtual void addImage(const DRW_Image* data ){}
    virtual void linkImage(const DRW_ImageDef* data ){}

    virtual void add3dFace(const DRW_3Dface& data ){}
    virtual void addComment(const char*){}

     virtual void addVport(const DRW_Vport& data) {}

    virtual void addTextStyle(const DRW_Textstyle& data);

    virtual void addViewport(const DRW_Viewport& data) {}

    virtual void setBlock(const int handle) {}

    static wxString toDxfString(const wxString& str);
    static wxString toNativeString(const wxString& data);

    // These functions are not used in Kicad.
    // But because they are virtual pure in DRW_Interface, they should be defined
    virtual void writeTextstyles() {}
    virtual void writeVports() {}
    virtual void writeHeader(DRW_Header& data) {}
    virtual void writeEntities() {}
    virtual void writeLTypes() {}
    virtual void writeLayers() {}
    virtual void writeBlockRecords() {}
    virtual void writeBlocks() {}
    virtual void writeDimstyles() {}

    void writeLine();
    void writeMtext();

};

#endif  // FILTERDXFRW_H
