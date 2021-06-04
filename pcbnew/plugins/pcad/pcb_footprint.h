/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef PCB_FOOTPRINT_H
#define PCB_FOOTPRINT_H

#include <pcad/pcad2kicad_common.h>
#include <pcad/pcad_item_types.h>
#include <pcad/pcb_component.h>

class BOARD;
class FOOTPRINT;
class wxStatusBar;
class wxString;
class XNODE;

namespace PCAD2KICAD {

class PCB_FOOTPRINT : public PCB_COMPONENT
{
public:
    PCB_FOOTPRINT( PCB_CALLBACKS* aCallbacks, BOARD* aBoard );
    ~PCB_FOOTPRINT();

    XNODE*  FindModulePatternDefName( XNODE* aNode, const wxString& aName );

    void DoLayerContentsObjects( XNODE* aNode, PCB_FOOTPRINT* aFootprint,
                                 PCB_COMPONENTS_ARRAY* aList, wxStatusBar* aStatusBar,
                                 const wxString&  aDefaultMeasurementUnit,
                                 const wxString& aActualConversion );

    void SetName( const wxString& aPin, const wxString& aName );

    virtual void Parse( XNODE* aNode, wxStatusBar* aStatusBar,
                        const wxString& aDefaultMeasurementUnit,
                        const wxString& aActualConversion );

    virtual void Flip() override;
    void AddToBoard() override;

    TTEXTVALUE            m_Value;           // has reference (Name from parent) and value
    PCB_COMPONENTS_ARRAY  m_FootprintItems;  // set of objects like PCB_LINE, PCB_PAD, PCB_VIA....
    int                   m_Mirror;
    VERTICES_ARRAY        m_BoardOutline;

private:
    XNODE*   FindPatternMultilayerSection( XNODE* aNode, wxString* aPatGraphRefName );
    wxString ModuleLayer( int aMirror );
};

} // namespace PCAD2KICAD

#endif    // PCB_FOOTPRINT_H
