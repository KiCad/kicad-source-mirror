/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board.h>
#include <i18n_utility.h>       // For _HKI definition
#include "stackup_predefined_prms.h"


BOARD_STACKUP_ITEM::BOARD_STACKUP_ITEM( BOARD_STACKUP_ITEM_TYPE aType )
{
    DIELECTRIC_PRMS item_prms;
    m_DielectricPrmsList.emplace_back( item_prms );
    m_LayerId = UNDEFINED_LAYER;
    m_Type = aType;
    SetDielectricLayerId( 1 );
    SetEnabled( true );

    // Initialize parameters to a usual value for allowed types:
    switch( m_Type )
    {
    case BS_ITEM_TYPE_COPPER:
        m_TypeName = KEY_COPPER;
        SetThickness( GetCopperDefaultThickness() );
        break;

    case BS_ITEM_TYPE_DIELECTRIC:
        m_TypeName = KEY_CORE;      // or prepreg
        SetMaterial( "FR4" );       // or other dielectric name
        SetLossTangent( 0.02 );     // for FR4
        SetEpsilonR( 4.5 );         // for FR4
        break;

    case BS_ITEM_TYPE_SOLDERPASTE:
        m_TypeName = "solderpaste";
        break;

    case BS_ITEM_TYPE_SOLDERMASK:
        m_TypeName = "soldermask";
        m_Color = "Green";
        SetMaterial( NotSpecifiedPrm() ); // or other solder mask material name
        SetThickness( GetMaskDefaultThickness() );
        SetEpsilonR( DEFAULT_EPSILON_R_SOLDERMASK );
        break;

    case BS_ITEM_TYPE_SILKSCREEN:
        m_TypeName = "silkscreen";
        m_Color = NotSpecifiedPrm();
        SetMaterial( NotSpecifiedPrm() ); // or other silkscreen material name
        SetEpsilonR( DEFAULT_EPSILON_R_SILKSCREEN );
        break;

    case BS_ITEM_TYPE_UNDEFINED:
        break;
    }
}


BOARD_STACKUP_ITEM::BOARD_STACKUP_ITEM( const BOARD_STACKUP_ITEM& aOther )
{
    m_LayerId = aOther.m_LayerId;
    m_DielectricLayerId = aOther.m_DielectricLayerId;
    m_Type = aOther.m_Type;
    m_enabled = aOther.m_enabled;
    m_DielectricPrmsList = aOther.m_DielectricPrmsList;
    m_TypeName = aOther.m_TypeName;
    m_LayerName = aOther.m_LayerName;
    m_Color = aOther.GetColor();
}


void BOARD_STACKUP_ITEM::AddDielectricPrms( int aDielectricPrmsIdx )
{
    // add a DIELECTRIC_PRMS item to m_DielectricPrmsList
    DIELECTRIC_PRMS new_prms;

    m_DielectricPrmsList.emplace( m_DielectricPrmsList.begin() + aDielectricPrmsIdx,
                                  new_prms );
}


void BOARD_STACKUP_ITEM::RemoveDielectricPrms( int aDielectricPrmsIdx )
{
    // Remove a DIELECTRIC_PRMS item from m_DielectricPrmsList if possible

    if( GetSublayersCount() < 2 || aDielectricPrmsIdx < 0
            || aDielectricPrmsIdx >= GetSublayersCount() )
        return;

    m_DielectricPrmsList.erase( m_DielectricPrmsList.begin() + aDielectricPrmsIdx );
}



int BOARD_STACKUP_ITEM::GetCopperDefaultThickness()
{
    // A reasonable thickness for copper layers:
    return Millimeter2iu( 0.035 );
}


int BOARD_STACKUP_ITEM::GetMaskDefaultThickness()
{
    // A reasonable thickness for solder mask:
    return Millimeter2iu( 0.01 );
}


// Getters:
int BOARD_STACKUP_ITEM::GetThickness( int aDielectricSubLayer ) const
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    return m_DielectricPrmsList[aDielectricSubLayer].m_Thickness;
}


double BOARD_STACKUP_ITEM::GetLossTangent( int aDielectricSubLayer ) const
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    return m_DielectricPrmsList[aDielectricSubLayer].m_LossTangent;
}


double BOARD_STACKUP_ITEM::GetEpsilonR( int aDielectricSubLayer ) const
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    return m_DielectricPrmsList[aDielectricSubLayer].m_EpsilonR;
}


bool BOARD_STACKUP_ITEM::IsThicknessLocked( int aDielectricSubLayer ) const
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    return m_DielectricPrmsList[aDielectricSubLayer].m_ThicknessLocked;
}


wxString BOARD_STACKUP_ITEM::GetMaterial( int aDielectricSubLayer ) const
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    return m_DielectricPrmsList[aDielectricSubLayer].m_Material;
}


// Setters:
void BOARD_STACKUP_ITEM::SetThickness( int aThickness, int aDielectricSubLayer )
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    if( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() )
        m_DielectricPrmsList[aDielectricSubLayer].m_Thickness = aThickness;
}


void BOARD_STACKUP_ITEM::SetLossTangent( double aTg, int aDielectricSubLayer )
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    if( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() )
        m_DielectricPrmsList[aDielectricSubLayer].m_LossTangent = aTg;
}


void BOARD_STACKUP_ITEM::SetEpsilonR( double aEpsilon, int aDielectricSubLayer )
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    if( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() )
        m_DielectricPrmsList[aDielectricSubLayer].m_EpsilonR = aEpsilon;
}


void BOARD_STACKUP_ITEM::SetThicknessLocked( bool aLocked, int aDielectricSubLayer )
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    if( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() )
        m_DielectricPrmsList[aDielectricSubLayer].m_ThicknessLocked = aLocked;
}


void BOARD_STACKUP_ITEM::SetMaterial( const wxString& aName, int aDielectricSubLayer )
{
    wxASSERT( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() );

    if( aDielectricSubLayer >= 0 && aDielectricSubLayer < GetSublayersCount() )
        m_DielectricPrmsList[aDielectricSubLayer].m_Material = aName;
}


bool BOARD_STACKUP_ITEM::HasEpsilonRValue() const
{
    return m_Type == BS_ITEM_TYPE_DIELECTRIC
           || m_Type == BS_ITEM_TYPE_SOLDERMASK
           //|| m_Type == BS_ITEM_TYPE_SILKSCREEN
            ;
};


bool BOARD_STACKUP_ITEM::HasLossTangentValue() const
{
    return m_Type == BS_ITEM_TYPE_DIELECTRIC
           || m_Type == BS_ITEM_TYPE_SOLDERMASK;
};


bool BOARD_STACKUP_ITEM::HasMaterialValue( int aDielectricSubLayer ) const
{
    // return true if the material is specified
    return IsMaterialEditable() && IsPrmSpecified( GetMaterial( aDielectricSubLayer ) );
}


bool BOARD_STACKUP_ITEM::IsMaterialEditable() const
{
    // The material is editable only for dielectric
    return m_Type == BS_ITEM_TYPE_DIELECTRIC ||
           m_Type == BS_ITEM_TYPE_SOLDERMASK ||
           m_Type == BS_ITEM_TYPE_SILKSCREEN;
}


bool BOARD_STACKUP_ITEM::IsColorEditable() const
{
    return m_Type == BS_ITEM_TYPE_SOLDERMASK || m_Type == BS_ITEM_TYPE_SILKSCREEN;
}


bool BOARD_STACKUP_ITEM::IsThicknessEditable() const
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


wxString BOARD_STACKUP_ITEM::FormatEpsilonR( int aDielectricSubLayer ) const
{
    // return a wxString to print/display Epsilon R
    wxString txt;
    txt.Printf( "%.2f", GetEpsilonR( aDielectricSubLayer ) );
    return txt;
}


wxString BOARD_STACKUP_ITEM::FormatLossTangent( int aDielectricSubLayer ) const
{
    // return a wxString to print/display Loss Tangent
    wxString txt;
    txt.Printf( "%g", GetLossTangent( aDielectricSubLayer ) );
    return txt;
}


wxString BOARD_STACKUP_ITEM::FormatDielectricLayerName() const
{
    // return a wxString to print/display a dielectric name
    wxString lname;
    lname.Printf( _( "Dielectric %d" ), GetDielectricLayerId() );

    return lname;
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


BOARD_STACKUP::BOARD_STACKUP( const BOARD_STACKUP& aOther )
{
    m_HasDielectricConstrains  = aOther.m_HasDielectricConstrains;
    m_HasThicknessConstrains   = aOther.m_HasThicknessConstrains;
    m_EdgeConnectorConstraints = aOther.m_EdgeConnectorConstraints;
    m_CastellatedPads          = aOther.m_CastellatedPads;
    m_EdgePlating              = aOther.m_EdgePlating;
    m_FinishType               = aOther.m_FinishType;

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
    m_HasDielectricConstrains  = aOther.m_HasDielectricConstrains;
    m_HasThicknessConstrains   = aOther.m_HasThicknessConstrains;
    m_EdgeConnectorConstraints = aOther.m_EdgeConnectorConstraints;
    m_CastellatedPads          = aOther.m_CastellatedPads;
    m_EdgePlating              = aOther.m_EdgePlating;
    m_FinishType               = aOther.m_FinishType;

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
        if( item->IsThicknessEditable() && item->IsEnabled() )
            thickness += item->GetThickness();
    }

    return thickness;
}


bool BOARD_STACKUP::SynchronizeWithBoard( BOARD_DESIGN_SETTINGS* aSettings )
{
    bool change = false;
    // Build the suitable stackup:
    BOARD_STACKUP stackup;
    stackup.BuildDefaultStackupList( aSettings );

    // First, find removed layers:
    for( BOARD_STACKUP_ITEM* curr_item: m_list )
    {
        bool found = false;

        for( BOARD_STACKUP_ITEM* item: stackup.GetList() )
        {
            if( curr_item->GetBrdLayerId() != UNDEFINED_LAYER )
            {
                if( item->GetBrdLayerId() == curr_item->GetBrdLayerId() )
                {
                    found = true;
                    break;
                }
            }
            else    // curr_item = dielectric layer
            {
                if( item->GetBrdLayerId() != UNDEFINED_LAYER )
                    continue;

                if( item->GetDielectricLayerId() == curr_item->GetDielectricLayerId() )
                {
                    found = true;
                    break;
                }
            }
        }

        if( !found )    // a layer was removed: a change is found
        {
            change = true;
            break;
        }
    }

    // Now initialize all stackup items to the initial values, when exist
    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        bool found = false;
        // Search for initial settings:
        for( const BOARD_STACKUP_ITEM* initial_item : m_list )
        {
            if( item->GetBrdLayerId() != UNDEFINED_LAYER )
            {
                if( item->GetBrdLayerId() == initial_item->GetBrdLayerId() )
                {
                    *item = *initial_item;
                    found = true;
                    break;
                }
            }
            else    // dielectric layer: see m_DielectricLayerId for identification
            {
                // Compare dielectric layer with dielectric layer
                if( initial_item->GetBrdLayerId() != UNDEFINED_LAYER )
                    continue;

                if( item->GetDielectricLayerId() == initial_item->GetDielectricLayerId() )
                {
                    *item = *initial_item;
                    found = true;
                    break;
                }
            }
        }

        if( !found )
        {
            change = true;
        }
    }

    // Transfer other stackup settings from aSettings
    const BOARD_STACKUP& source_stackup = aSettings->GetStackupDescriptor();
    m_HasDielectricConstrains  = source_stackup.m_HasDielectricConstrains;
    m_EdgeConnectorConstraints = source_stackup.m_EdgeConnectorConstraints;
    m_CastellatedPads = source_stackup.m_CastellatedPads;
    m_EdgePlating     = source_stackup.m_EdgePlating;
    m_FinishType      = source_stackup.m_FinishType;

    *this = stackup;

    return change;
}


void BOARD_STACKUP::BuildDefaultStackupList( const BOARD_DESIGN_SETTINGS* aSettings,
                                             int aActiveCopperLayersCount )
{
    // Creates a default stackup, according to the current BOARD_DESIGN_SETTINGS settings.
    // Note: the m_TypeName string is made translatable using _HKI marker, but is not
    // translated when building the stackup.
    // It will be used as this in files, and can be translated only in dialog
    // if aSettings == NULL, build a full stackup (with 32 copper layers)
    LSET enabledLayer = aSettings ? aSettings->GetEnabledLayers() : StackupAllowedBrdLayers();
    int copperLayerCount = aSettings ? aSettings->GetCopperLayerCount() : B_Cu+1;

    // We need to calculate a suitable dielectric layer thickness.
    // If no settings, and if aActiveCopperLayersCount is given, use it
    // (If no settings, and no aActiveCopperLayersCount, the full 32 layers are used)
    int activeCuLayerCount = copperLayerCount;

    if( aSettings == nullptr && aActiveCopperLayersCount > 0 )
        activeCuLayerCount = aActiveCopperLayersCount;

    int brd__thickness = aSettings ? aSettings->GetBoardThickness() : Millimeter2iu( 1.6 );
    int diel_thickness = brd__thickness -
                         ( BOARD_STACKUP_ITEM::GetCopperDefaultThickness() * activeCuLayerCount );

    // Take in account the solder mask thickness:
    int sm_count = ( enabledLayer & LSET( 2, F_Mask, B_Mask) ).count();
    diel_thickness -= BOARD_STACKUP_ITEM::GetMaskDefaultThickness() * sm_count;

    int dielectric_idx = 0;

    // Add silk screen, solder mask and solder paste layers on top
    if( enabledLayer[F_SilkS] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SILKSCREEN );
        item->SetBrdLayerId( F_SilkS );
        item->SetTypeName( _HKI( "Top Silk Screen" ) );
        Add( item );
    }

    if( enabledLayer[F_Paste] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERPASTE );
        item->SetBrdLayerId( F_Paste );
        item->SetTypeName( _HKI( "Top Solder Paste" ) );
        Add( item );
    }

    if( enabledLayer[F_Mask] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERMASK );
        item->SetBrdLayerId( F_Mask );
        item->SetTypeName( _HKI( "Top Solder Mask" ) );
        Add( item );
    }

    // Add copper and dielectric layers
    for( int ii = 0; ii < copperLayerCount; ii++ )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_COPPER );
        item->SetBrdLayerId( ( PCB_LAYER_ID )ii );
        item->SetTypeName( KEY_COPPER );
        Add( item );

        if( ii == copperLayerCount-1 )
        {
            item->SetBrdLayerId( B_Cu );
            break;
        }

        // Add the dielectric layer:
        item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_DIELECTRIC );
        item->SetThickness( diel_thickness );
        item->SetDielectricLayerId( dielectric_idx + 1 );

        // Display a dielectric default layer name:
        if( (dielectric_idx & 1) == 0 )
        {
            item->SetTypeName( KEY_CORE );
            item->SetMaterial( "FR4" );
        }
        else
        {
            item->SetTypeName( KEY_PREPREG );
            item->SetMaterial( "FR4" );
        }

        Add( item );
        dielectric_idx++;
    }

    // Add silk screen, solder mask and solder paste layers on bottom
    if( enabledLayer[B_Mask] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERMASK );
        item->SetBrdLayerId( B_Mask );
        item->SetTypeName( _HKI( "Bottom Solder Mask" ) );
        Add( item );
    }

    if( enabledLayer[B_Paste] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SOLDERPASTE );
        item->SetBrdLayerId( B_Paste );
        item->SetTypeName( _HKI( "Bottom Solder Paste" ) );
        Add( item );
    }

    if( enabledLayer[B_SilkS] )
    {
        BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_SILKSCREEN );
        item->SetBrdLayerId( B_SilkS );
        item->SetTypeName( _HKI( "Bottom Silk Screen" ) );
        Add( item );
    }

    // Transfer other stackup settings from aSettings
    if( aSettings )
    {
        const BOARD_STACKUP& source_stackup = aSettings->GetStackupDescriptor();
        m_EdgeConnectorConstraints = source_stackup.m_EdgeConnectorConstraints;
        m_CastellatedPads = source_stackup.m_CastellatedPads;
        m_EdgePlating     = source_stackup.m_EdgePlating;
        m_FinishType      = source_stackup.m_FinishType;
    }
}


void BOARD_STACKUP::FormatBoardStackup( OUTPUTFORMATTER* aFormatter,
                                        const BOARD* aBoard, int aNestLevel ) const
{
    // Board stackup is the ordered list from top to bottom of
    // physical layers and substrate used to build the board.
    if( m_list.empty() )
        return;

    aFormatter->Print( aNestLevel, "(stackup\n" );
    int nest_level = aNestLevel+1;

    // Note:
    // Unspecified parameters are not stored in file.
    for( BOARD_STACKUP_ITEM* item: m_list )
    {
        wxString layer_name;

        if( item->GetBrdLayerId() == UNDEFINED_LAYER )
            layer_name.Printf( "dielectric %d", item->GetDielectricLayerId() );
        else
            layer_name = LSET::Name( item->GetBrdLayerId() );

        aFormatter->Print( nest_level, "(layer %s (type %s)",
                           aFormatter->Quotew( layer_name ).c_str(),
                           aFormatter->Quotew( item->GetTypeName() ).c_str() );

        if( item->IsColorEditable() && IsPrmSpecified( item->GetColor() ) )
            aFormatter->Print( 0, " (color %s)",
                               aFormatter->Quotew( item->GetColor() ).c_str() );

        for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
        {
            if( idx )    // not for the main (first) layer.
            {
                aFormatter->Print( 0, "\n" );
                aFormatter->Print( nest_level+1, "addsublayer" );
            }

            if( item->IsThicknessEditable() )
            {
                if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC && item->IsThicknessLocked( idx ) )
                    aFormatter->Print( 0, " (thickness %s locked)",
                                       FormatInternalUnits( item->GetThickness( idx ) ).c_str() );
                else
                    aFormatter->Print( 0, " (thickness %s)",
                                       FormatInternalUnits( item->GetThickness( idx ) ).c_str() );
            }

            if( item->HasMaterialValue( idx ) )
                aFormatter->Print( 0, " (material %s)",
                                   aFormatter->Quotew( item->GetMaterial( idx ) ).c_str() );

            if( item->HasEpsilonRValue() && item->HasMaterialValue( idx ) )
                aFormatter->Print( 0, " (epsilon_r %g)", item->GetEpsilonR( idx ) );

            if( item->HasLossTangentValue() && item->HasMaterialValue( idx ) )
                aFormatter->Print( 0, " (loss_tangent %s)",
                                   Double2Str(item->GetLossTangent( idx ) ).c_str() );
        }

        aFormatter->Print( 0, ")\n" );
    }

    // Other infos about board, related to layers and other fabrication specifications
    if( IsPrmSpecified( m_FinishType ) )
        aFormatter->Print( nest_level, "(copper_finish %s)\n",
                           aFormatter->Quotew( m_FinishType ).c_str() );

    aFormatter->Print( nest_level, "(dielectric_constraints %s)\n",
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


bool IsPrmSpecified( const wxString& aPrmValue )
{
    // return true if the param value is specified:

    if( !aPrmValue.IsEmpty()
        && ( aPrmValue.CmpNoCase( NotSpecifiedPrm() ) != 0 )
        && aPrmValue != wxGetTranslation( NotSpecifiedPrm() ) )
        return true;

    return false;
}
