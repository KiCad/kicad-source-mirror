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
#include <list>

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
    std::list<BOARD_ITEM*> m_newItemsList;    // The list of new items added to the board
    double m_xOffset;       // X coord offset for conversion (in mm)
    double m_yOffset;       // Y coord offset for conversion (in mm)
    double m_defaultThickness;  // default line thickness for conversion (in mm)
    double m_DXF2mm;        // The scale factor to convert DXF units to mm
    int m_brdLayer;         // The board layer to place imported DXF items
    int m_version;          // the dxf version, not used here
    std::string m_codePage; // The code page, not used here
    bool m_useModuleItems;  // Use module items instead of board items when true.

public:
    DXF2BRD_CONVERTER();
    ~DXF2BRD_CONVERTER();

    bool IsUsingModuleItems() const { return m_useModuleItems; }
    void UseModuleItems( bool aUseModuleItems = true ) { m_useModuleItems = aUseModuleItems; }

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
     */
    bool ImportDxfFile( const wxString& aFile );

    /**
     * @return the list of new BOARD_ITEM
     */
    const std::list<BOARD_ITEM*>& GetItemsList() const
    {
        return m_newItemsList;
    }

private:
    // coordinate conversions from dxf to internal units
    int mapX( double aDxfCoordX );
    int mapY( double aDxfCoordY );
    int mapDim( double aDxfValue );

    // Functions to aid in the creation of a LWPolyline
    void insertLine( const wxRealPoint& aSegStart, const wxRealPoint& aSegEnd, int aWidth );
    void insertArc( const wxRealPoint& aSegStart, const wxRealPoint& aSegEnd,
                    double aBulge, int aWidth );

    // Methods from DRW_CreationInterface:
    // They are "call back" fonctions, called when the corresponding object
    // is read in dxf file
    // Depending of the application, they can do something or not
    virtual void addHeader( const DRW_Header* aData ) override;
    virtual void addLType( const DRW_LType& aData ) override {}
    virtual void addLayer( const DRW_Layer& aData ) override;
    virtual void addDimStyle( const DRW_Dimstyle& aData ) override {}
    virtual void addBlock( const DRW_Block& aData ) override {}
    virtual void endBlock() override {}
    virtual void addPoint( const DRW_Point& aData ) override {}
    virtual void addLine( const DRW_Line& aData) override;
    virtual void addRay( const DRW_Ray& aData ) override {}
    virtual void addXline( const DRW_Xline& aData ) override {}
    virtual void addCircle( const DRW_Circle& aData ) override;
    virtual void addArc( const DRW_Arc& aData ) override;
    virtual void addEllipse( const DRW_Ellipse& aData ) override {}
    virtual void addLWPolyline( const DRW_LWPolyline& aData ) override;
    virtual void addText( const DRW_Text& aData ) override;
    virtual void addPolyline( const DRW_Polyline& aData ) override;
    virtual void addSpline( const DRW_Spline* aData ) override {}
    virtual void addKnot( const DRW_Entity&) override {}
    virtual void addInsert( const DRW_Insert& aData ) override {}
    virtual void addTrace( const DRW_Trace& aData ) override {}
    virtual void addSolid( const DRW_Solid& aData ) override {}
    virtual void addMText( const DRW_MText& aData) override;
    virtual void addDimAlign( const DRW_DimAligned* aData ) override {}
    virtual void addDimLinear( const DRW_DimLinear* aData ) override {}
    virtual void addDimRadial( const DRW_DimRadial* aData ) override {}
    virtual void addDimDiametric( const DRW_DimDiametric* aData ) override {}
    virtual void addDimAngular( const DRW_DimAngular* aData ) override {}
    virtual void addDimAngular3P( const DRW_DimAngular3p* aData ) override {}
    virtual void addDimOrdinate( const DRW_DimOrdinate* aData ) override {}
    virtual void addLeader( const DRW_Leader* aData ) override {}
    virtual void addHatch( const DRW_Hatch* aData ) override {}
    virtual void addImage( const DRW_Image* aData ) override {}
    virtual void linkImage( const DRW_ImageDef* aData ) override {}

    virtual void add3dFace( const DRW_3Dface& aData ) override {}
    virtual void addComment( const char*) override {}

    virtual void addVport( const DRW_Vport& aData ) override {}

    virtual void addTextStyle( const DRW_Textstyle& aData ) override;

    virtual void addViewport( const DRW_Viewport& aData ) override {}

    virtual void setBlock( const int aHandle ) override {}

    /**
     * Converts a native unicode string into a DXF encoded string.
     *
     * DXF endoding includes the following special sequences:
     * - %%%c for a diameter sign
     * - %%%d for a degree sign
     * - %%%p for a plus/minus sign
     */
    static wxString toDxfString( const wxString& aStr );

    /**
     * Converts a DXF encoded string into a native Unicode string.
     */
    static wxString toNativeString( const wxString& aData );

    // These functions are not used in Kicad.
    // But because they are virtual pure in DRW_Interface, they should be defined
    virtual void writeTextstyles() override {}
    virtual void writeVports() override {}
    virtual void writeHeader( DRW_Header& aData ) override {}
    virtual void writeEntities() override {}
    virtual void writeLTypes() override {}
    virtual void writeLayers() override {}
    virtual void writeBlockRecords() override {}
    virtual void writeBlocks() override {}
    virtual void writeDimstyles() override {}

    void writeLine();
    void writeMtext();

    virtual void addAppId( const DRW_AppId& data ) override {}
    virtual void writeAppId() override {}
};

#endif  // FILTERDXFRW_H
