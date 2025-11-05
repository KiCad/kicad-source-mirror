/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef PCB_H_
#define PCB_H_

#include <pcad/pcad2kicad_common.h>
#include <pcad/pcad_item_types.h>
#include <pcad/pcad_callbacks.h>
#include <pcad/pcad_footprint.h>

#include <map>
#include <wx/arrstr.h>

class BOARD;
class XNODE;
class wxStatusBar;
class wxString;
class wxRealPoint;
class wxXmlDocument;

namespace PCAD2KICAD {

class PCAD_PCB : public PCAD_FOOTPRINT, public PCAD_CALLBACKS
{
public:
    PCAD_PCB( BOARD* aBoard );
    ~PCAD_PCB();

    PCB_LAYER_ID GetKiCadLayer( int aPCadLayer ) const override;
    LAYER_TYPE_T GetLayerType( int aPCadLayer ) const override;
    wxString GetLayerNetNameRef( int aPCadLayer ) const override;
    int GetNetCode( const wxString& aNetName ) const override;

    void ParseBoard( wxStatusBar* aStatusBar, wxXmlDocument* aXmlDoc,
                     const wxString& aActualConversion );

    void AddToBoard( FOOTPRINT* aFootprint = nullptr ) override;

private:
    XNODE* FindCompDefName( XNODE* aNode, const wxString& aName ) const;

    void SetTextProperty( XNODE* aNode, TTEXTVALUE* aTextValue, const wxString& aPatGraphRefName,
                          const wxString& aXmlName, const wxString& aActualConversion );

    void DoPCBComponents( XNODE* aNode, wxXmlDocument* aXmlDoc, const wxString& aActualConversion,
                          wxStatusBar* aStatusBar );

    void ConnectPinToNet( const wxString& aCr, const wxString& aPr, const wxString& aNetName );

    int FindLayer( const wxString& aLayerName ) const;
    void MapLayer( XNODE* aNode );
    int FindOutlinePoint( const VERTICES_ARRAY* aOutline, wxRealPoint aPoint ) const;
    double GetDistance( const wxRealPoint* aPoint1, const wxRealPoint* aPoint2 ) const;
    void   ExtractOutlinePointsFromEnhancedPolygon( const wxString& aActualConversion, XNODE* lNode );
    void   GetBoardOutline( wxXmlDocument* aXmlDoc, const wxString& aActualConversion );
    void   ExtractLayerStackup( wxXmlDocument* aXmlDoc );
    void   CreatePolygonsForEmptyPowerPlanes();
    void   ProcessLayerContentsObjects( wxStatusBar* aStatusBar, const wxString& aActualConversion, XNODE* aNode );

public:
    PCAD_COMPONENTS_ARRAY   m_PcbComponents;    // PCB footprints,Lines,Routes,Texts, .... and so on
    PCAD_NETS_ARRAY         m_PcbNetlist;       // net objects collection
    wxString                m_DefaultMeasurementUnit;
    std::map<int, TLAYER>   m_LayersMap;        // flexible layers mapping
    int                     m_SizeX;
    int                     m_SizeY;

private:
    std::vector<std::pair<wxString, long>>   m_layersStackup;
};

} // namespace PCAD2KICAD

#endif    // pcb_H_
