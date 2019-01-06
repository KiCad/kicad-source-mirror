/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <macros.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <view/view.h>
#include <invoke_pcb_dialog.h>
#include <class_board.h>
#include <collectors.h>
#include <panel_setup_layers.h>


// some define to choose how copper layers widgets are shown

// if defined, display only active copper layers
// if not displays always 1=the full set (32 copper layers)
#define HIDE_INACTIVE_LAYERS


static LSEQ dlg_layers()
{
    // layers that are put out into the dialog UI, coordinate with wxformbuilder and
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
    };

    return LSEQ( layers, layers + arrayDim( layers ) );
}


// Layer bit masks for each defined "Preset Layer Grouping"
static const LSET presets[] =
{
    LSET(),     // shift the array index up by one, matches with "Custom".

    // "Two layers, parts on Front only"
    LSET( 2, F_Cu, B_Cu ) | LSET::FrontTechMask() | LSET::UserMask(),

    // "Two layers, parts on Back only",
    LSET( 2, F_Cu, B_Cu ) | LSET::BackTechMask() | LSET::UserMask(),

    // "Two layers, parts on Front and Back",
    LSET( 2, F_Cu, B_Cu ) | LSET::FrontTechMask() | LSET::BackTechMask() | LSET::UserMask(),

    // "Four layers, parts on Front only"
    LSET( 4, F_Cu, B_Cu, In1_Cu, In2_Cu ) | LSET::FrontTechMask() | LSET::UserMask(),

    // "Four layers, parts on Front and Back"
    LSET( 4, F_Cu, B_Cu, In1_Cu, In2_Cu ) | LSET::FrontTechMask() | LSET::BackTechMask() |
        LSET::UserMask(),

    //  "All layers on",
    LSET().set(),
};


PANEL_SETUP_LAYERS::PANEL_SETUP_LAYERS( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_LAYERS_BASE( aParent->GetTreebook() ),
        m_Parent( aParent ), m_frame( aFrame ),
        m_pcbThickness( aFrame, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits, true )
{
    m_pcb = aFrame->GetBoard();

    m_LayersListPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOX) );
}


PANEL_SETUP_LAYERS_CTLs PANEL_SETUP_LAYERS::getCTLs( LAYER_NUM aLayerNumber )
{
#define RETURN_COPPER(x) return PANEL_SETUP_LAYERS_CTLs( x##Name, x##CheckBox, x##Choice )
#define RETURN_AUX(x)    return PANEL_SETUP_LAYERS_CTLs( x##Name, x##CheckBox, x##StaticText )

    switch( aLayerNumber )
    {
    case F_CrtYd:               RETURN_AUX( m_CrtYdFront );
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
    case B_CrtYd:               RETURN_AUX( m_CrtYdBack );

    case Edge_Cuts:             RETURN_AUX( m_PCBEdges );
    case Margin:                RETURN_AUX( m_Margin );
    case Eco2_User:             RETURN_AUX( m_Eco2 );
    case Eco1_User:             RETURN_AUX( m_Eco1 );
    case Cmts_User:             RETURN_AUX( m_Comments );
    case Dwgs_User:             RETURN_AUX( m_Drawings );
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

    showCopperChoice( m_pcb->GetCopperLayerCount() );
    setCopperLayerCheckBoxes( m_pcb->GetCopperLayerCount() );
    m_pcbThickness.SetValue( m_pcb->GetDesignSettings().GetBoardThickness() );

    showBoardLayerNames();
    showSelectedLayerCheckBoxes( m_enabledLayers );
    showPresets( m_enabledLayers );
    showLayerTypes();
    setMandatoryLayerCheckBoxes();

    return true;
}


void PANEL_SETUP_LAYERS::setMandatoryLayerCheckBoxes()
{
    for( int layer : { F_CrtYd, B_CrtYd, Edge_Cuts, Margin } )
        setLayerCheckBox( layer, true );
}


void PANEL_SETUP_LAYERS::showCopperChoice( int copperCount )
{
    if( copperCount > MAX_CU_LAYERS )
        copperCount = MAX_CU_LAYERS;

    if( copperCount < 2 )
        copperCount = 2;

    for( int lyrCnt = 2; lyrCnt <= MAX_CU_LAYERS; lyrCnt += 2 )
    {
        // note this will change a one layer board to 2:
        if( copperCount <= lyrCnt )
        {
            int idx = lyrCnt/2 - 1;
            m_CopperLayersChoice->SetSelection(idx);
            break;
        }
    }
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

            if( dynamic_cast<wxTextCtrl*>( ctl ) )
                dynamic_cast<wxTextCtrl*>( ctl )->SetValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );                                  // wxStaticText
        }
    }
}


void PANEL_SETUP_LAYERS::showSelectedLayerCheckBoxes( LSET enabledLayers )
{
    // the check boxes
    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        setLayerCheckBox( layer, enabledLayers[layer] );
    }
}


void PANEL_SETUP_LAYERS::showPresets( LSET enabledLayers )
{
    int presetsNdx = 0;     // the "Custom" setting, matches nothing

    for( unsigned i=1; i<arrayDim( presets );  ++i )
    {
        if( enabledLayers == presets[i] )
        {
            presetsNdx = i;
            break;
        }
    }

    m_PresetsChoice->SetSelection( presetsNdx );
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


LSET PANEL_SETUP_LAYERS::getUILayerMask()
{
    LSET layerMaskResult;

    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;
        wxCheckBox*  ctl = getCheckBox( layer );

        if( ctl->GetValue() )
            layerMaskResult.set( layer );
    }

    return layerMaskResult;
}


void PANEL_SETUP_LAYERS::setLayerCheckBox( LAYER_NUM aLayer, bool isChecked )
{
    PANEL_SETUP_LAYERS_CTLs ctl = getCTLs( aLayer );

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
        // This code hides non-active copper layers, or redisplays hidden
        // layers which are now needed.
        PANEL_SETUP_LAYERS_CTLs ctl = getCTLs( layer );

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
    m_enabledLayers = getUILayerMask();

    showPresets( m_enabledLayers );
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
            wxString controlLabel = m_staticTextCopperLayers->GetLabel();
            // knock the ':' off the end
            controlLabel = controlLabel.substr( 0, controlLabel.size() - 1 );

            msg.Printf( _( "Use the \"%s\" control to change the number of copper layers." ),
                        controlLabel );
            DisplayError( this, msg );

            copper->SetValue( true );
            return;
        }
    }

    for( int layer : { F_CrtYd, B_CrtYd, Edge_Cuts, Margin } )
    {
        wxCheckBox* mandatory = getCheckBox( layer );

        if( source == mandatory )
        {
            msg.Printf( _( "The %s layer is mandatory." ), getLayerName( layer ) );
            DisplayError( this, msg );
            mandatory->SetValue( true );
            return;
        }
    }
}


void PANEL_SETUP_LAYERS::OnPresetsChoice( wxCommandEvent& event )
{
    int presetNdx = m_PresetsChoice->GetCurrentSelection();

    if( presetNdx <= 0 )        // the Custom setting controls nothing.
        return;

    if( presetNdx < (int) arrayDim(presets) )
    {
        m_enabledLayers = presets[ presetNdx ];
        LSET copperSet = m_enabledLayers & LSET::AllCuMask();
        int copperCount = copperSet.count();

        showCopperChoice( copperCount );
        showSelectedLayerCheckBoxes( m_enabledLayers );
        setCopperLayerCheckBoxes( copperCount );
    }

    // Ensure mandatory layers are activated
    setMandatoryLayerCheckBoxes();
}


void PANEL_SETUP_LAYERS::OnCopperLayersChoice( wxCommandEvent& event )
{
    setCopperLayerCheckBoxes( m_CopperLayersChoice->GetCurrentSelection() * 2 + 2 );
    m_enabledLayers = getUILayerMask();
    showPresets( m_enabledLayers );
}


bool PANEL_SETUP_LAYERS::TransferDataFromWindow()
{
    if( !testLayerNames() )
        return false;

    wxString msg;

    int thickness = m_pcbThickness.GetValue();

    // Check for removed layers with items which will get deleted from the board.
    LSEQ removedLayers = getRemovedLayersWithItems();

    // Check for non copper layers in use in footprints, and therefore not removable.
    LSEQ notremovableLayers = getNonRemovableLayers();

    if( !notremovableLayers.empty() )
    {
        for( unsigned int ii = 0; ii < notremovableLayers.size(); ii++ )
            msg << m_pcb->GetLayerName( notremovableLayers[ii] ) << "\n";

        if( !IsOK( this, wxString::Format( _( "Footprints have some items on removed layers:\n"
                                              "%s\n"
                                              "These items will be no longer accessible\n"
                                              "Do you wish to continue?" ), msg ) ) )
            return false;
    }

    if( !removedLayers.empty() &&
        !IsOK( this, _( "Items have been found on removed layers. This operation will delete "
                        "all items from removed layers and cannot be undone. Do you wish to "
                        "continue?" ) ) )
        return false;

    // Delete all objects on layers that have been removed.  Leaving them in copper layers
    // can (will?) result in DRC errors and it pollutes the board file with cruft.
    bool hasRemovedBoardItems = false;

    if( !removedLayers.empty() )
    {
        PCB_LAYER_COLLECTOR collector;

        for( auto layer_id : removedLayers )
        {
            collector.SetLayerId( layer_id );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::BoardLevelItems );

            // Bye-bye items on on removed layer.
            if( collector.GetCount() != 0 )
            {
                hasRemovedBoardItems = true;

                for( int i = 0; i < collector.GetCount(); i++ )
                {
                    BOARD_ITEM* item = collector[i];
                    m_pcb->Remove( item );
                    delete item;
                }
            }
        }
    }

    m_enabledLayers = getUILayerMask();

    if( m_enabledLayers != m_pcb->GetEnabledLayers() )
    {
        m_pcb->SetEnabledLayers( m_enabledLayers );

        /* Ensure enabled layers are also visible
         * This is mainly to avoid mistakes if some enabled
         * layers are not visible when exiting this dialog
         */
        m_pcb->SetVisibleLayers( m_enabledLayers );
    }

    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID  layer = *seq;

        if( m_enabledLayers[layer] )
        {
            m_pcb->SetLayerName( layer, getLayerName( layer ) );
            LAYER_T t = (LAYER_T) getLayerTypeIndex( layer );
            m_pcb->SetLayerType( layer, t );
        }
    }

    m_pcb->GetDesignSettings().SetBoardThickness( thickness );

    // If some board items are deleted: rebuild the connectivity,
    // because it is likely some tracks and vias where removed
    if( hasRemovedBoardItems )
    {
        // Rebuild list of nets (full ratsnest rebuild)
        m_frame->Compile_Ratsnest( NULL, true );
        m_pcb->BuildConnectivity();
    }

    return true;
}


int PANEL_SETUP_LAYERS::getLayerTypeIndex( LAYER_NUM aLayer )
{
    wxChoice*  ctl =  getChoice( aLayer );
    int ret = ctl->GetCurrentSelection();   // indices must have same sequence as LAYER_T
    return ret;
}


wxString PANEL_SETUP_LAYERS::getLayerName( LAYER_NUM aLayer )
{
    wxControl* control = getName( aLayer );

    if( dynamic_cast<wxTextCtrl*>( control ) )
        return static_cast<wxTextCtrl*>( control )->GetValue().Trim();
    else
        return static_cast<wxStaticText*>( control )->GetLabel();
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

    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        // we _can_ rely on m_enabledLayers being current here:
        if( !m_enabledLayers[layer] )
            continue;

        wxString name = getLayerName( layer );

        ctl = (wxTextCtrl*) getName( layer );

        // check name for legality.
        // 1) cannot be blank.
        // 2) cannot have blanks.
        // 3) cannot have " chars
        // 4) cannot be 'signal'
        // 5) must be unique.
        // 6) cannot have illegal chars in filenames ( some filenames are built from layer names )
        //    like : % $ \ " / :
        wxString badchars = wxFileName::GetForbiddenChars( wxPATH_DOS );
        badchars.Append( '%' );

        if( !name )
        {
            m_Parent->SetError( _( "Layer must have a name." ), this, ctl );
            return false;
        }

        if( hasOneOf( name, badchars ) )
        {
            auto msg = wxString::Format(_( "\"%s\" are forbidden in layer names." ), badchars );
            m_Parent->SetError( msg, this, ctl );
            return false;
        }

        if( name == wxT( "signal" ) )
        {
            m_Parent->SetError( _( "Layer name \"signal\" is reserved." ), this, ctl );
            return false;
        }

        for( const wxString& existingName : names )
        {
            if( name == existingName )
            {
                auto msg = wxString::Format(_( "Layer name \"%s\" is already in use." ), name );
                m_Parent->SetError( msg, this, ctl );
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
    LSET newLayers = getUILayerMask();
    LSET curLayers = m_pcb->GetEnabledLayers();

    if( newLayers == curLayers )    // return a empty list if no change
        return removedLayers;

    PCB_LAYER_COLLECTOR collector;
    LSEQ newLayerSeq = newLayers.Seq();

    for( auto layer_id : curLayers.Seq() )
    {
        if( std::find( newLayerSeq.begin(), newLayerSeq.end(), layer_id ) == newLayerSeq.end() )
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
     //Build the list of non copper layers in use in footprints.
    LSEQ inUseLayers;
    LSET newLayers = getUILayerMask();
    LSET curLayers = m_pcb->GetEnabledLayers();

    if( newLayers == curLayers )    // return a empty list if no change
        return inUseLayers;

    PCB_LAYER_COLLECTOR collector;
    LSEQ newLayerSeq = newLayers.Seq();

    for( auto layer_id : curLayers.Seq() )
    {
        if( IsCopperLayer( layer_id ) ) // Copper layers are not taken in account here
            continue;

        if( std::find( newLayerSeq.begin(), newLayerSeq.end(), layer_id ) == newLayerSeq.end() )
        {
            collector.SetLayerId( layer_id );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::ModuleItems );

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
