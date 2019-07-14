/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "class_board_stackup.h"
#include <convert_to_biu.h>
#include <base_units.h>
#include <layers_id_colors_and_visibility.h>
#include <board_design_settings.h>
#include <class_board.h>
#include <i18n_utility.h>       // For _HKI definition
#include "stackup_predefined_prms.h"

// A reasonable thickness for copper layers:
const int copperDefaultThickness = Millimeter2iu( 0.035 );
// A reasonable thickness for solder mask:
const int maskDefaultThickness = Millimeter2iu( 0.01 );

BOARD_STACKUP_ITEM::BOARD_STACKUP_ITEM( BOARD_STACKUP_ITEM_TYPE aType )
{
    m_LayerId = UNDEFINED_LAYER;
    m_Type = aType;
    m_DielectricLayerId = 0;
    m_EpsilonR = 0;
    m_LossTangent = 0.0;

    // Initialize parameters to a usual value for allowed types:
    switch( m_Type )
    {
    case BS_ITEM_TYPE_COPPER:
        m_TypeName = "copper";
        m_Thickness = copperDefaultThickness;
        break;

    case BS_ITEM_TYPE_DIELECTRIC:
        m_TypeName = "core";        // or prepreg
        m_Material = "FR4";         // or other dielectric name
        m_DielectricLayerId = 1;
        m_Thickness = 0;            // will be set later
        m_LossTangent = 0.02;       // for FR4
        m_EpsilonR = 4.5;           // for FR4
        break;

    case BS_ITEM_TYPE_SOLDERPASTE:
        m_TypeName = "solderpaste";
        m_Thickness = 0.0;          // Not used
        break;

    case BS_ITEM_TYPE_SOLDERMASK:
        m_TypeName = "soldermask";
        m_Color = NOT_SPECIFIED;
        m_Thickness = maskDefaultThickness;
        m_EpsilonR = 3.5;
        m_LossTangent = 0.0;
        break;

    case BS_ITEM_TYPE_SILKSCREEN:
        m_TypeName = "silkscreen";
        m_Color = NOT_SPECIFIED;
        m_Thickness = 0.0;          // Not used
        break;

    case BS_ITEM_TYPE_UNDEFINED:
        m_Thickness = 0.0;
        break;
    }
}


BOARD_STACKUP_ITEM::BOARD_STACKUP_ITEM( BOARD_STACKUP_ITEM& aOther )
{
    m_LayerId = aOther.m_LayerId;
    m_Type = aOther.m_Type;
    m_DielectricLayerId = aOther.m_DielectricLayerId;
    m_TypeName = aOther.m_TypeName;
    m_Material = aOther.m_Material;
    m_Color = aOther.m_Color;
    m_Thickness = aOther.m_Thickness;
    m_EpsilonR = aOther.m_EpsilonR;
    m_LossTangent = aOther.m_LossTangent;
}


bool BOARD_STACKUP_ITEM::HasEpsilonRValue()
{
    return m_Type == BS_ITEM_TYPE_DIELECTRIC || m_Type == BS_ITEM_TYPE_SOLDERMASK;
};


bool BOARD_STACKUP_ITEM::HasLossTangentValue()
{
    return m_Type == BS_ITEM_TYPE_DIELECTRIC || m_Type == BS_ITEM_TYPE_SOLDERMASK;
};


bool BOARD_STACKUP_ITEM::IsMaterialEditable()
{
    // The material is editable only for dielectric
    return m_Type == BS_ITEM_TYPE_DIELECTRIC;
}


bool BOARD_STACKUP_ITEM::IsColorEditable()
{
    return m_Type == BS_ITEM_TYPE_SOLDERMASK || m_Type == BS_ITEM_TYPE_SILKSCREEN;
}


bool BOARD_STACKUP_ITEM::IsThicknessEditable()
{
    switch( m_Type )
    {
    case BS_ITEM_TYPE_COPPER:
        return true;

    case BS_ITEM_TYPE_DIELECTRIC:
        return true;

    case BS_ITEM_TYPE_SOLDERMASK:
        return true;

    case BS_ITEM_TYPE_SOLDERPASTE:
        return false;

    case BS_ITEM_TYPE_SILKSCREEN:
        return false;

    default:
        break;
    }

    return false;
}


BOARD_STACKUP::BOARD_STACKUP()
{
    m_HasDielectricConstrains = false;  // True if some dielectric layers have constrains
                                        // (Loss tg and Epison R)
    m_HasThicknessConstrains = false;   // True if some dielectric or copper layers have constrains
    m_EdgeConnectorConstraints = BS_EDGE_CONNECTOR_NONE;
    m_CastellatedPads = false;          // True if some castellated pads exist
    m_EdgePlating = false;              // True if edge board is plated
    m_FinishType = "None";              // undefined finish type
}


BOARD_STACKUP::BOARD_STACKUP( BOARD_STACKUP& aOther )
{
    m_HasDielectricConstrains = aOther.m_HasDielectricConstrains;
    m_EdgeConnectorConstraints = aOther.m_EdgeConnectorConstraints;
    m_CastellatedPads = aOther.m_CastellatedPads;
    m_EdgePlating = aOther.m_EdgePlating;
    m_FinishType = aOther.m_FinishType;

    // All items in aOther.m_list have to be duplicated, because aOther.m_list
    // manage pointers to these items
    for( auto item : aOther.m_list )
    {
        BOARD_STACKUP_ITEM* dup_item = new BOARD_STACKUP_ITEM( *item );
        Add( dup_item );
    }
}


BOARD_STACKUP& BOARD_STACKUP::operator=( const BOARD_STACKUP& aOther )
{
    m_HasDielectricConstrains = aOther.m_HasDielectricConstrains;
    m_EdgeConnectorConstraints = aOther.m_EdgeConnectorConstraints;
    m_CastellatedPads = aOther.m_CastellatedPads;
    m_EdgePlating = aOther.m_EdgePlating;
    m_FinishType = aOther.m_FinishType;

    RemoveAll();

    // All items in aOther.m_list have to be duplicated, because aOther.m_list
    // manage pointers to these items
    for( auto item : aOther.m_list )
    {
        BOARD_STACKUP_ITEM* dup_item = new BOARD_STACKUP_ITEM( *item );
        Add( dup_item );
    }

    return *this;
}


void BOARD_STACKUP::RemoveAll()
{
    for( auto item : m_list )
        delete item;

    m_list.clear();
}


BOARD_STACKUP_ITEM* BOARD_STACKUP::GetStackupLayer( int aIndex )
{
    if( aIndex < 0 || aIndex >= GetCount() )
        return nullptr;

    return GetList()[aIndex];
}


int BOARD_STACKUP::BuildBoardTicknessFromStackup() const
{
    // return the board thickness from the thickness of BOARD_STACKUP_ITEM list
    int thickness = 0;

    for( auto item : m_list )
    {
        if( item->IsThicknessEditable() )
            thickness += item->m_Thickness;
    }

    return thickness;
}


bool BOARD_STACKUP::SynchronizeWithBoard( BOARD_DESIGN_SETTINGS* aSettings )
{
    bool change = false;
    // Build the suitable stackup:
    BOARD_STACKUP stackup;
    stackup.BuildDefaultStackupList( aSettings );

    // First test for removed layers:
    for( BOARD_STACKUP_ITEM* old_item: m_list )
    {
        bool found = false;

        for( BOARD_STACKUP_ITEM* item: stackup.GetList() )
        {
            if( item->m_LayerId == old_item->m_LayerId )
            {
                found = true;
                break;
            }
        }

        if( !found )    // a layer was removed: a change is found
        {
            change = true;
            break;
        }
    }

    // Now initialize all stackup items to the initial values, when exist
    for( BOARD_STACKUP_ITEM* item: stackup.GetList() )
    {
        bool found = false;
        // Search for initial settings:
        for( BOARD_STACKUP_ITEM* initial_item: m_list )
        {
            if( item->m_LayerId != UNDEFINED_LAYER )
            {
                if( item->m_LayerId == initial_item->m_LayerId )
                {
                    *item = *initial_item;
                    found = true;
                    break;
                }
            }
            else    // dielectric layer: see m_DielectricLayerId for identification
            {
                if( item->m_DielectricLayerId == initial_item->m_DielectricLayerId )
                {
                    *item = *initial_item;
                    found = true;
                    break;
                }
            }
        }

        if( !found )
            change = true;
    }

    // Transfer other stackup settings from aSettings
    BOARD_STACKUP& source_stackup = aSettings->GetStackupDescriptor();
    m_HasDielectricConstrains = source_stackup.m_HasDielectricConstrains;
    m_EdgeConnectorConstraints = source_stackup.m_EdgeConnectorConstraints;
    m_CastellatedPads = source_stackup.m_CastellatedPads;
    m_EdgePlating = source_stackup.m_EdgePlating;
    m_FinishType = source_stackup.m_FinishType;

    *this = stackup;

    return change;
}


void BOARD_STACKUP::BuildDefaultStackupList( BOARD_DESIGN_SETTINGS* aSettings )
{
    // Creates a default stackup, according to the current BOARD_DESIGN_SETTINGS settings.
    // Note: the m_TypeName string is made translatable using _HKI marker, but is not
    // translated when building the stackup.
    // It will be used as this in files, and can be translated only in dialog
    LSET enabledLayer = aSettings->GetEnabledLayers();
    int copperLayerCount = aSettings->GetCopperLayerCount();
    double diel_thickness = aSettings->GetBoardThickness()
                            - (copperDefaultThickness * copperLayerCount);
    diel_thickness /= copperLayerCount - 1;

    int dielectric_idx = 0;

    // Add silk screen, solder mask and solder paste layers on top
    if( enabledLayer[F_SilkS] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SILKSCREEN );
        item->m_LayerId = F_SilkS;
        item->m_TypeName = _HKI( "Top Silk Screen" );
        Add( item );
    }

    if( enabledLayer[F_Paste] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERPASTE );
        item->m_LayerId = F_Paste;
        item->m_TypeName = _HKI( "Top Solder Paste" );
        Add( item );
    }

    if( enabledLayer[F_Mask] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERMASK );
        item->m_LayerId = F_Mask;
        item->m_TypeName = _HKI( "Top Solder Mask" );
        Add( item );
    }

    // Add copper and dielectric layers
    for( int ii = 0; ii < copperLayerCount; ii++ )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_COPPER );
        item->m_LayerId = ( PCB_LAYER_ID )ii;
        Add( item );

        if( ii == copperLayerCount-1 )
        {
            item->m_LayerId = B_Cu;
            break;
        }

        // Add the dielectric layer:
        item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_DIELECTRIC );
        item->m_Thickness = diel_thickness;
        item->m_DielectricLayerId = dielectric_idx + 1;

        // Display a dielectric default layer name:
        if( (dielectric_idx & 1) == 0 )
        {
            item->m_TypeName = _HKI( "core" );
            item->m_Material = "FR4";
        }
        else
        {
            item->m_TypeName = _HKI( "prepreg" );
            item->m_Material = "FR4";
        }

        Add( item );
        dielectric_idx++;
    }

    // Add silk screen, solder mask and solder paste layers on bottom
    if( enabledLayer[B_Mask] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERMASK );
        item->m_LayerId = B_Mask;
        item->m_TypeName = _HKI( "Bottom Solder Mask" );
        Add( item );
    }

    if( enabledLayer[B_Paste] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERPASTE );
        item->m_LayerId = B_Paste;
        item->m_TypeName = _HKI( "Bottom Solder Paste" );
        Add( item );
    }

    if( enabledLayer[B_SilkS] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SILKSCREEN );
        item->m_LayerId = B_SilkS;
        item->m_TypeName = _HKI( "Bottom Silk Screen" );
        Add( item );
    }

    // Transfer other stackup settings from aSettings
    BOARD_STACKUP& source_stackup = aSettings->GetStackupDescriptor();
    m_HasDielectricConstrains = source_stackup.m_HasDielectricConstrains;
    m_EdgeConnectorConstraints = source_stackup.m_EdgeConnectorConstraints;
    m_CastellatedPads = source_stackup.m_CastellatedPads;
    m_EdgePlating = source_stackup.m_EdgePlating;
    m_FinishType = source_stackup.m_FinishType;
}



void BOARD_STACKUP::FormatBoardStackup( OUTPUTFORMATTER* aFormatter,
                                         BOARD* aBoard, int aNestLevel ) const
{
    // Board stackup is the ordered list from top to bottom of
    // physical layers and substrate used to build the board.
    if( m_list.empty() )
        return;

    aFormatter->Print( aNestLevel, "(board_stackup\n" );
    int nest_level = aNestLevel+1;

    for( BOARD_STACKUP_ITEM* item: m_list )
    {
        wxString layer_name;

        if( item->m_LayerId == UNDEFINED_LAYER )
        {
            layer_name.Printf( "dielectric %d", item->m_DielectricLayerId );
        }
        else
            layer_name = aBoard->GetLayerName( item->m_LayerId );

        aFormatter->Print( nest_level, "(layer %s (type %s)",
                           aFormatter->Quotew( layer_name ).c_str(),
                           aFormatter->Quotew( item->m_TypeName ).c_str() );

        if( item->IsThicknessEditable() )
            aFormatter->Print( 0, " (thickness %s)",
                               FormatInternalUnits( (int)item->m_Thickness ).c_str() );

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
            aFormatter->Print( 0, " (material %s)",
                               aFormatter->Quotew( item->m_Material ).c_str() );

        if( item->HasEpsilonRValue() )
            aFormatter->Print( 0, " (epsilon_r %g)", item->m_EpsilonR );

        if( item->HasLossTangentValue() )
            aFormatter->Print( 0, " (loss %s)",
                               Double2Str(item->m_LossTangent ).c_str() );

        if( item->IsColorEditable() && !item->m_Color.IsEmpty()
            && item->m_Color != NOT_SPECIFIED )
            aFormatter->Print( 0, " (color %s)",
                               aFormatter->Quotew( item->m_Color ).c_str() );

        aFormatter->Print( 0, ")\n" );
    }

    // Other infos about board, related to layers and other fabrication specifications
    if( !m_FinishType.IsEmpty() && m_FinishType != NOT_SPECIFIED )
        aFormatter->Print( nest_level, "(copper_finish %s)\n",
                           aFormatter->Quotew( m_FinishType ).c_str() );

    aFormatter->Print( nest_level, "(dielectric_constrains %s)\n",
                       m_HasDielectricConstrains ? "yes" : "no" );

    if( m_EdgeConnectorConstraints > 0 )
        aFormatter->Print( nest_level, "(edge_connector %s)\n",
                           m_EdgeConnectorConstraints > 1 ? "bevelled": "yes" );

    if( m_CastellatedPads )
        aFormatter->Print( nest_level, "(castellated_pads yes)\n" );

    if( m_EdgePlating )
        aFormatter->Print( nest_level, "(edge_plating yes)\n" );

    aFormatter->Print( aNestLevel, ")\n" );
}
