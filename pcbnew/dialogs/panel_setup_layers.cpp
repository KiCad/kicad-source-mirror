/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <confirm.h>
#include <core/arraydim.h>
#include <core/kicad_algo.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <collectors.h>
#include <panel_setup_layers.h>
#include <board_stackup_manager/panel_board_stackup.h>

#include <wx/choicdlg.h>
#include <wx/treebook.h>
#include <eda_list_dialog.h>


// some define to choose how copper layers widgets are shown

// if defined, display only active copper layers
// if not displays always 1=the full set (32 copper layers)
#define HIDE_INACTIVE_LAYERS


static LSEQ dlg_layers()
{
    // Layers that are put out into the dialog UI, coordinate with wxformbuilder and
    // getCTLs( LAYER_NUM aLayerNumber )
    static const PCB_LAYER_ID layers[] = {
        F_CrtYd,
        F_Fab,
        F_Adhes,
        F_Paste,
        F_SilkS,
        F_Mask,
        F_Cu,

        In1_Cu,
        In2_Cu,
        In3_Cu,
        In4_Cu,
        In5_Cu,
        In6_Cu,
        In7_Cu,
        In8_Cu,
        In9_Cu,
        In10_Cu,
        In11_Cu,
        In12_Cu,
        In13_Cu,
        In14_Cu,
        In15_Cu,

        In16_Cu,
        In17_Cu,
        In18_Cu,
        In19_Cu,
        In20_Cu,
        In21_Cu,
        In22_Cu,
        In23_Cu,
        In24_Cu,
        In25_Cu,
        In26_Cu,
        In27_Cu,
        In28_Cu,
        In29_Cu,
        In30_Cu,

        B_Cu,
        B_Mask,
        B_SilkS,
        B_Paste,
        B_Adhes,
        B_Fab,
        B_CrtYd,

        Edge_Cuts,
        Margin,
        Eco2_User,
        Eco1_User,
        Cmts_User,
        Dwgs_User,

        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9,
    };

    return LSEQ( layers, layers + arrayDim( layers ) );
}


PANEL_SETUP_LAYERS::PANEL_SETUP_LAYERS( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_LAYERS_BASE( aParent->GetTreebook() ),
        m_parentDialog( aParent ),
        m_frame( aFrame ),
        m_physicalStackup( nullptr )
{
    m_pcb = aFrame->GetBoard();
}


PANEL_SETUP_LAYERS_CTLs PANEL_SETUP_LAYERS::getCTLs( LAYER_NUM aLayerNumber )
{
#define RETURN_COPPER( x )    return PANEL_SETUP_LAYERS_CTLs( x##Name, x##CheckBox, x##Choice )
#define RETURN_AUX( x )       return PANEL_SETUP_LAYERS_CTLs( x##Name, x##CheckBox, x##StaticText )
#define RETURN_MANDATORY( x ) return PANEL_SETUP_LAYERS_CTLs( x##Name, nullptr, x##StaticText )

    switch( aLayerNumber )
    {
    case F_CrtYd:               RETURN_MANDATORY( m_CrtYdFront );
    case F_Fab:                 RETURN_AUX( m_FabFront );
    case F_Adhes:               RETURN_AUX( m_AdhesFront );
    case F_Paste:               RETURN_AUX( m_SoldPFront );
    case F_SilkS:               RETURN_AUX( m_SilkSFront );
    case F_Mask:                RETURN_AUX( m_MaskFront );
    case F_Cu:                  RETURN_COPPER( m_Front );

    case In1_Cu:                RETURN_COPPER( m_In1 );
    case In2_Cu:                RETURN_COPPER( m_In2 );
    case In3_Cu:                RETURN_COPPER( m_In3 );
    case In4_Cu:                RETURN_COPPER( m_In4 );
    case In5_Cu:                RETURN_COPPER( m_In5 );
    case In6_Cu:                RETURN_COPPER( m_In6 );
    case In7_Cu:                RETURN_COPPER( m_In7 );
    case In8_Cu:                RETURN_COPPER( m_In8 );
    case In9_Cu:                RETURN_COPPER( m_In9 );
    case In10_Cu:               RETURN_COPPER( m_In10 );
    case In11_Cu:               RETURN_COPPER( m_In11 );
    case In12_Cu:               RETURN_COPPER( m_In12 );
    case In13_Cu:               RETURN_COPPER( m_In13 );
    case In14_Cu:               RETURN_COPPER( m_In14 );
    case In15_Cu:               RETURN_COPPER( m_In15 );

    case In16_Cu:               RETURN_COPPER( m_In16 );
    case In17_Cu:               RETURN_COPPER( m_In17 );
    case In18_Cu:               RETURN_COPPER( m_In18 );
    case In19_Cu:               RETURN_COPPER( m_In19 );
    case In20_Cu:               RETURN_COPPER( m_In20 );
    case In21_Cu:               RETURN_COPPER( m_In21 );
    case In22_Cu:               RETURN_COPPER( m_In22 );
    case In23_Cu:               RETURN_COPPER( m_In23 );
    case In24_Cu:               RETURN_COPPER( m_In24 );
    case In25_Cu:               RETURN_COPPER( m_In25 );
    case In26_Cu:               RETURN_COPPER( m_In26 );
    case In27_Cu:               RETURN_COPPER( m_In27 );
    case In28_Cu:               RETURN_COPPER( m_In28 );
    case In29_Cu:               RETURN_COPPER( m_In29 );
    case In30_Cu:               RETURN_COPPER( m_In30 );

    case B_Cu:                  RETURN_COPPER( m_Back );
    case B_Mask:                RETURN_AUX( m_MaskBack );
    case B_SilkS:               RETURN_AUX( m_SilkSBack );
    case B_Paste:               RETURN_AUX( m_SoldPBack );
    case B_Adhes:               RETURN_AUX( m_AdhesBack );
    case B_Fab:                 RETURN_AUX( m_FabBack );
    case B_CrtYd:               RETURN_MANDATORY( m_CrtYdBack );

    case Edge_Cuts:             RETURN_MANDATORY( m_PCBEdges );
    case Margin:                RETURN_MANDATORY( m_Margin );
    case Eco2_User:             RETURN_AUX( m_Eco2 );
    case Eco1_User:             RETURN_AUX( m_Eco1 );
    case Cmts_User:             RETURN_AUX( m_Comments );
    case Dwgs_User:             RETURN_AUX( m_Drawings );

    case User_1:                RETURN_AUX( m_User1 );
    case User_2:                RETURN_AUX( m_User2 );
    case User_3:                RETURN_AUX( m_User3 );
    case User_4:                RETURN_AUX( m_User4 );
    case User_5:                RETURN_AUX( m_User5 );
    case User_6:                RETURN_AUX( m_User6 );
    case User_7:                RETURN_AUX( m_User7 );
    case User_8:                RETURN_AUX( m_User8 );
    case User_9:                RETURN_AUX( m_User9 );

    default:
        wxASSERT_MSG( 0, wxT( "bad layer id" ) );
        return PANEL_SETUP_LAYERS_CTLs( nullptr,  nullptr, nullptr );
    }
}


wxControl* PANEL_SETUP_LAYERS::getName( LAYER_NUM aLayer )
{
    return getCTLs( aLayer ).name;
}


wxCheckBox* PANEL_SETUP_LAYERS::getCheckBox( LAYER_NUM aLayer )
{
    return getCTLs( aLayer ).checkbox;
}


wxChoice* PANEL_SETUP_LAYERS::getChoice( LAYER_NUM aLayer )
{
    return (wxChoice*) getCTLs( aLayer ).choice;
}


bool PANEL_SETUP_LAYERS::TransferDataToWindow()
{
    m_enabledLayers = m_pcb->GetEnabledLayers();

    // Rescue may be enabled, but should not be shown in this dialog
    m_enabledLayers.reset( Rescue );

    setCopperLayerCheckBoxes( m_pcb->GetCopperLayerCount() );

    showBoardLayerNames();
    showSelectedLayerCheckBoxes( m_enabledLayers );

    showLayerTypes();
    setMandatoryLayerCheckBoxes();
    setUserDefinedLayerCheckBoxes();

    return true;
}


void PANEL_SETUP_LAYERS::SyncCopperLayers( int aNumCopperLayers )
{
    setCopperLayerCheckBoxes( aNumCopperLayers );
}


void PANEL_SETUP_LAYERS::setMandatoryLayerCheckBoxes()
{
    for( int layer : { F_CrtYd, B_CrtYd, Edge_Cuts, Margin } )
        setLayerCheckBox( layer, true );
}


void PANEL_SETUP_LAYERS::setUserDefinedLayerCheckBoxes()
{
    for( LSEQ seq = LSET::UserDefinedLayers().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        bool     state = m_pcb->IsLayerEnabled( layer );

#ifdef HIDE_INACTIVE_LAYERS
        // This code hides inactive copper layers, or redisplays hidden layers which are now needed.
        PANEL_SETUP_LAYERS_CTLs ctl = getCTLs( layer );

        // All user-defined layers should have a checkbox
        wxASSERT( ctl.checkbox );

        ctl.name->Show( state );
        ctl.checkbox->Show( state );
        ctl.choice->Show( state );
#endif

        setLayerCheckBox( layer, state );
    }

#ifdef HIDE_INACTIVE_LAYERS
    // Send an size event to force sizers to be updated,
    // because the number of copper layers can have changed.
    wxSizeEvent evt_size( m_LayersListPanel->GetSize() );
    m_LayersListPanel->GetEventHandler()->ProcessEvent( evt_size );
#endif
}


void PANEL_SETUP_LAYERS::showBoardLayerNames()
{
    // Set all the board's layer names into the dialog by calling BOARD::GetLayerName(),
    // which will call BOARD::GetStandardLayerName() for non-coppers.

    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        wxControl*   ctl = getName( layer );

        if( ctl )
        {
            wxString lname = m_pcb->GetLayerName( layer );

            if( auto textCtl = dynamic_cast<wxTextCtrl*>( ctl ) )
                textCtl->SetValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );         // wxStaticText
        }
    }
}


void PANEL_SETUP_LAYERS::showSelectedLayerCheckBoxes( LSET enabledLayers )
{
    // The check boxes
    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        setLayerCheckBox( layer, enabledLayers[layer] );
    }
}


void PANEL_SETUP_LAYERS::showLayerTypes()
{
    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID cu_layer = *seq;

        wxChoice* ctl = getChoice( cu_layer );
        ctl->SetSelection( m_pcb->GetLayerType( cu_layer ) );
    }
}


LSET PANEL_SETUP_LAYERS::GetUILayerMask()
{
    LSET layerMaskResult;

    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        wxCheckBox*  ctl = getCheckBox( layer );

        if( !ctl || ctl->GetValue() )
            layerMaskResult.set( layer );
    }

    return layerMaskResult;
}


void PANEL_SETUP_LAYERS::setLayerCheckBox( LAYER_NUM aLayer, bool isChecked )
{
    PANEL_SETUP_LAYERS_CTLs ctl = getCTLs( aLayer );

    if( !ctl.checkbox )
        return;

    ctl.checkbox->SetValue( isChecked );
}


void PANEL_SETUP_LAYERS::setCopperLayerCheckBoxes( int copperCount )
{
    if( copperCount > 0 )
    {
        setLayerCheckBox( F_Cu, true );
        --copperCount;
    }

    if( copperCount > 0 )
    {
        setLayerCheckBox( B_Cu, true );
        --copperCount;
    }

    for( LSEQ seq = LSET::InternalCuMask().Seq();  seq;  ++seq, --copperCount )
    {
        PCB_LAYER_ID layer = *seq;
        bool     state = copperCount > 0;

#ifdef HIDE_INACTIVE_LAYERS
        // This code hides inactive copper layers, or redisplays hidden layers which are now needed.
        PANEL_SETUP_LAYERS_CTLs ctl = getCTLs( layer );

        // Inner layers should have a checkbox
        wxASSERT( ctl.checkbox );

        ctl.name->Show( state );
        ctl.checkbox->Show( state );
        ctl.choice->Show( state );
#endif

        setLayerCheckBox( layer, state );
    }

#ifdef HIDE_INACTIVE_LAYERS
    // Send an size event to force sizers to be updated,
    // because the number of copper layers can have changed.
    wxSizeEvent evt_size( m_LayersListPanel->GetSize() );
    m_LayersListPanel->GetEventHandler()->ProcessEvent( evt_size );
#endif
}


void PANEL_SETUP_LAYERS::OnCheckBox( wxCommandEvent& event )
{
    m_enabledLayers = GetUILayerMask();
}


void PANEL_SETUP_LAYERS::DenyChangeCheckBox( wxCommandEvent& event )
{
    wxObject* source = event.GetEventObject();
    wxString msg;

    for( LSEQ seq = LSET::AllCuMask().Seq(); seq; ++seq )
    {
        wxCheckBox* copper = getCheckBox( *seq );

        if( source == copper )
        {
            DisplayError( this,
                    _( "Use the Physical Stackup page to change the number of copper layers." ) );

            copper->SetValue( true );
            return;
        }
    }
}


bool PANEL_SETUP_LAYERS::TransferDataFromWindow()
{
    if( !testLayerNames() )
        return false;

    wxASSERT( m_physicalStackup );

    // Make sure we have the latest copper layer count
    SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );

    wxString msg;
    bool     modified = false;

    // Check for removed layers with items which will get deleted from the board.
    LSEQ removedLayers = getRemovedLayersWithItems();

    // Check for non-copper layers in use in footprints, and therefore not removable.
    LSEQ notremovableLayers = getNonRemovableLayers();

    if( !notremovableLayers.empty() )
    {
        for( unsigned int ii = 0; ii < notremovableLayers.size(); ii++ )
            msg << m_pcb->GetLayerName( notremovableLayers[ii] ) << "\n";

        if( !IsOK( this, wxString::Format( _( "Footprints have some items on removed layers:\n"
                                              "%s\n"
                                              "These items will be no longer accessible\n"
                                              "Do you wish to continue?" ), msg ) ) )
        {
            return false;
        }
    }

    if( !removedLayers.empty()
            && !IsOK( this, _( "Items have been found on removed layers. This operation will "
                               "delete all items from removed layers and cannot be undone.\n"
                               "Do you wish to continue?" ) ) )
    {
        return false;
    }

    // Delete all objects on layers that have been removed.  Leaving them in copper layers
    // can (will?) result in DRC errors and it pollutes the board file with cruft.
    bool hasRemovedBoardItems = false;

    if( !removedLayers.empty() )
    {
        PCB_LAYER_COLLECTOR collector;

        for( PCB_LAYER_ID layer_id : removedLayers )
        {
            collector.SetLayerId( layer_id );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::BoardLevelItems );

            // Bye-bye items on removed layer.
            for( int i = 0; i < collector.GetCount(); i++ )
            {
                BOARD_ITEM* item = collector[i];
                LSET        layers = item->GetLayerSet();

                layers.reset( layer_id );
                hasRemovedBoardItems = true;
                modified = true;

                if( layers.any() )
                {
                    item->SetLayerSet( layers );
                }
                else
                {
                    m_pcb->Remove( item );
                    delete item;
                }
            }
        }
    }

    m_enabledLayers = GetUILayerMask();

    if( m_enabledLayers != m_pcb->GetEnabledLayers() )
    {
        m_pcb->SetEnabledLayers( m_enabledLayers );

        /* Ensure enabled layers are also visible
         * This is mainly to avoid mistakes if some enabled
         * layers are not visible when exiting this dialog
         */
        m_pcb->SetVisibleLayers( m_enabledLayers );

        modified = true;
    }

    for( LSEQ seq = LSET::AllLayersMask().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID  layer = *seq;

        if( m_enabledLayers[layer] )
        {
            const wxString& newLayerName = GetLayerName( layer );

            if( m_pcb->GetLayerName( layer ) != newLayerName )
            {
                m_pcb->SetLayerName( layer, newLayerName );
                modified = true;
            }

            // Only copper layers have a definable type.
            if( LSET::AllCuMask().Contains( layer ) )
            {
                LAYER_T t = (LAYER_T) getLayerTypeIndex( layer );

                if( m_pcb->GetLayerType( layer ) != t )
                {
                    m_pcb->SetLayerType( layer, t );
                    modified = true;
                }
            }
        }
    }

    for( LSEQ seq = LSET::UserDefinedLayers().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID    layer = *seq;
        const wxString& newLayerName = GetLayerName( layer );

        if( m_enabledLayers[ layer ] && m_pcb->GetLayerName( layer ) != newLayerName )
        {
            m_pcb->SetLayerName( layer, newLayerName );
            modified = true;
        }
    }

    // If some board items are deleted: Rebuild the connectivity,
    // because it is likely some tracks and vias were removed
    if( hasRemovedBoardItems )
    {
        // Rebuild list of nets (full ratsnest rebuild)
        m_pcb->BuildConnectivity();
        m_frame->Compile_Ratsnest( true );
    }

    if( modified )
        m_frame->OnModify();

    return true;
}


int PANEL_SETUP_LAYERS::getLayerTypeIndex( LAYER_NUM aLayer )
{
    wxChoice* ctl = getChoice( aLayer );
    int ret = ctl->GetCurrentSelection(); // Indices must have same sequence as LAYER_T
    return ret;
}


wxString PANEL_SETUP_LAYERS::GetLayerName( LAYER_NUM aLayer )
{
    wxControl* control = getName( aLayer );

    if( auto textCtl = dynamic_cast<wxTextCtrl*>( control ) )
        return textCtl->GetValue().Trim();
    else
        return control->GetLabel();
}


static bool hasOneOf( const wxString& str, const wxString& chars )
{
    for( unsigned i=0; i<chars.Len();  ++i )
    {
        if( str.Find( chars[i] ) != wxNOT_FOUND )
            return true;
    }

    return false;
}


bool PANEL_SETUP_LAYERS::testLayerNames()
{
    std::vector<wxString>    names;
    wxTextCtrl*  ctl;

    for( LSEQ seq = LSET::AllLayersMask().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        // we _can_ rely on m_enabledLayers being current here:

        if( !m_enabledLayers[layer] )
            continue;

        wxString name = GetLayerName( layer );

        ctl = (wxTextCtrl*) getName( layer );

        // Check name for legality:
        // 1) Cannot be blank.
        // 2) Cannot have blanks.
        // 3) Cannot have " chars
        // 4) Cannot be 'signal'
        // 5) Must be unique.
        // 6) Cannot have illegal chars in filenames ( some filenames are built from layer names )
        //    like : % $ \ " / :
        wxString badchars = wxFileName::GetForbiddenChars( wxPATH_DOS );
        badchars.Append( '%' );

        if( !name )
        {
            m_parentDialog->SetError( _( "Layer must have a name." ), this, ctl );
            return false;
        }

        if( hasOneOf( name, badchars ) )
        {
            wxString msg = wxString::Format(_( "%s are forbidden in layer names." ), badchars );
            m_parentDialog->SetError( msg, this, ctl );
            return false;
        }

        if( name == wxT( "signal" ) )
        {
            m_parentDialog->SetError( _( "Layer name \"signal\" is reserved." ), this, ctl );
            return false;
        }

        for( const wxString& existingName : names )
        {
            if( name == existingName )
            {
                wxString msg = wxString::Format(_( "Layer name '%s' already in use." ), name );
                m_parentDialog->SetError( msg, this, ctl );
                return false;
            }
        }

        names.push_back( name );
    }

    return true;
}


LSEQ PANEL_SETUP_LAYERS::getRemovedLayersWithItems()
{
    LSEQ removedLayers;
    LSET newLayers = GetUILayerMask();
    LSET curLayers = m_pcb->GetEnabledLayers();

    if( newLayers == curLayers ) // Return an empty list if no change
        return removedLayers;

    PCB_LAYER_COLLECTOR collector;
    LSEQ newLayerSeq = newLayers.Seq();

    for( PCB_LAYER_ID layer_id : curLayers.Seq() )
    {
        if( !alg::contains( newLayerSeq, layer_id ) )
        {
            collector.SetLayerId( layer_id );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::BoardLevelItems );

            if( collector.GetCount() != 0 )
                removedLayers.push_back( layer_id );
        }
    }

    return removedLayers;
}


LSEQ PANEL_SETUP_LAYERS::getNonRemovableLayers()
{
    // Build the list of non-copper layers in use in footprints.
    LSEQ inUseLayers;
    LSET newLayers = GetUILayerMask();
    LSET curLayers = m_pcb->GetEnabledLayers();

    if( newLayers == curLayers ) // Return an empty list if no change
        return inUseLayers;

    PCB_LAYER_COLLECTOR collector;
    LSEQ newLayerSeq = newLayers.Seq();

    for( auto layer_id : curLayers.Seq() )
    {
        if( IsCopperLayer( layer_id ) ) // Copper layers are not taken into account here
            continue;

        if( !alg::contains( newLayerSeq, layer_id ) )
        {
            collector.SetLayerId( layer_id );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::FootprintItems );

            if( collector.GetCount() != 0 )
                inUseLayers.push_back( layer_id );
        }
    }

    return inUseLayers;
}


void PANEL_SETUP_LAYERS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD* savedBoard = m_pcb;

    m_pcb = aBoard;
    TransferDataToWindow();

    m_pcb = savedBoard;
}


bool PANEL_SETUP_LAYERS::CheckCopperLayerCount( BOARD* aWorkingBoard, BOARD* aImportedBoard )
{
    /*
     * This function warns users if they are going to delete inner copper layers because
     * they're importing settings from a board with less copper layers than the board
     * already loaded. We want to return "true" as default on the assumption no layer will
     * actually be deleted.
     */
    bool okToDeleteCopperLayers = true;

    // Get the number of copper layers in the loaded board and the "import settings" board
    int currNumLayers = aWorkingBoard->GetCopperLayerCount();
    int newNumLayers  = aImportedBoard->GetCopperLayerCount();

    if( newNumLayers < currNumLayers )
    {
        wxString msg = wxString::Format( _( "Imported settings have fewer copper layers than "
                                            "the current board (%i instead of %i).\n\n"
                                            "Continue and delete the extra inner copper layers "
                                            "from the current board?" ),
                                         newNumLayers,
                                         currNumLayers );

        wxMessageDialog dlg( this, msg, _( "Inner Layers To Be Deleted" ),
                             wxICON_WARNING | wxSTAY_ON_TOP | wxYES | wxNO | wxNO_DEFAULT );

        if( wxID_NO == dlg.ShowModal() )
            okToDeleteCopperLayers = false;
    }

    return okToDeleteCopperLayers;
}


void PANEL_SETUP_LAYERS::addUserDefinedLayer( wxCommandEvent& aEvent )
{
    wxArrayString headers;
    headers.Add( _( "Layers" ) );

    // Build the available user-defined layers list:
    std::vector<wxArrayString> list;

    for( LSEQ seq = LSET::UserDefinedLayers().Seq();  seq;  ++seq )
    {
        wxCheckBox* checkBox = getCheckBox( *seq );

        if( checkBox && checkBox->IsShown() )
            continue;

        wxArrayString available_user_layer;
        available_user_layer.Add( LayerName( *seq ) );

        list.emplace_back( available_user_layer );
    }

    if( list.empty() )
    {
        DisplayErrorMessage( m_parentDialog,
                             _( "All user-defined layers have already been added." ) );
        return;
    }

    EDA_LIST_DIALOG dlg( m_parentDialog, _( "Add User-defined Layer" ), headers, list );
    dlg.SetListLabel( _( "Select layer to add:" ) );
    dlg.HideFilter();

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetTextSelection().IsEmpty() )
        return;

    LSEQ seq;

    for( seq = LSET::UserDefinedLayers().Seq();  seq;  ++seq )
    {
        if( LayerName( *seq ) == dlg.GetTextSelection() )
            break;
    }

    wxCHECK( *seq >= User_1 && *seq <= User_9, /* void */ );

    LSET newLayer( *seq );

    m_enabledLayers |= newLayer;

    PANEL_SETUP_LAYERS_CTLs ctl = getCTLs( *seq );

    // All user-defined layers should have a checkbox
    wxASSERT( ctl.checkbox );

    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ctl.name );

    wxCHECK( textCtrl, /* void */ );
    textCtrl->ChangeValue( LSET::Name( *seq ) );
    ctl.name->Show( true );
    ctl.checkbox->Show( true );
    ctl.choice->Show( true );

    wxSizeEvent evt_size( m_LayersListPanel->GetSize() );
    m_LayersListPanel->GetEventHandler()->ProcessEvent( evt_size );

    setLayerCheckBox( *seq, true );
}


