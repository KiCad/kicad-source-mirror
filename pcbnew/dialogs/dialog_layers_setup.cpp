/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <macros.h>

#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>

#include <class_board.h>

#include <dialog_layers_setup_base.h>


// some define to choose how copper layers widgets are shown

// if defined, display only active copper layers
// if not displays always 1=the full set (16 layers)
#define HIDE_INACTIVE_LAYERS

// if defined, use the layer manager copper layers order (from FRONT to BACK)
// to display inner layers.
// if not, use the default order (from BACK to FRONT)
#define USE_LAYER_MANAGER_COPPER_LAYERS_ORDER


/**
 * Struct CTLs
 * holds the 3 ui control pointers for a single board layer.
 */
struct CTLs
{
    CTLs( wxControl* aName, wxCheckBox* aCheckBox, wxControl* aChoiceOrDesc, wxPanel * aPanel = NULL)
    {
        name     = aName;
        checkbox = aCheckBox;
        choice   = aChoiceOrDesc;
        panel    = aPanel;
    }

    wxControl*      name;
    wxCheckBox*     checkbox;
    wxControl*      choice;
    wxPanel *       panel;
};


class DIALOG_LAYERS_SETUP : public DIALOG_LAYERS_SETUP_BASE
{
private:
    PCB_EDIT_FRAME*     m_Parent;

    int                 m_CopperLayerCount;
    LAYER_MSK           m_EnabledLayers;

    BOARD*              m_Pcb;

    wxStaticText*       m_NameStaticText;
    wxStaticText*       m_EnabledStaticText;
    wxStaticText*       m_TypeStaticText;


    void setLayerCheckBox( LAYER_NUM layer, bool isChecked );
    void setCopperLayerCheckBoxes( int copperCount );

    void showCopperChoice( int copperCount );
    void showBoardLayerNames();
    void showSelectedLayerCheckBoxes( LAYER_MSK enableLayerMask );
    void showLayerTypes();
    void showPresets( LAYER_MSK enabledLayerMask );

    /** return the selected layer mask within the UI checkboxes */
    LAYER_MSK getUILayerMask();
    wxString getLayerName( LAYER_NUM layer );
    int getLayerTypeIndex( LAYER_NUM layer );


    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOkButtonClick( wxCommandEvent& event );
    void OnCheckBox( wxCommandEvent& event );
    void DenyChangeCheckBox( wxCommandEvent& event );
    void OnPresetsChoice( wxCommandEvent& event );
    void OnCopperLayersChoice( wxCommandEvent& event );

    bool testLayerNames();

    /**
     * Function getCTLs
     * maps \a aLayerNumber to the wx IDs for that layer which are
     * the layer name control ID, checkbox control ID, and choice control ID
     */
    CTLs getCTLs( LAYER_NUM aLayerNumber );

    wxControl* getName( LAYER_NUM aLayer )
    {
        return getCTLs( aLayer ).name;
    }

    wxCheckBox* getCheckBox( LAYER_NUM aLayer )
    {
        return getCTLs( aLayer ).checkbox;
    }

    wxChoice* getChoice( LAYER_NUM aLayer )
    {
        return (wxChoice*) getCTLs( aLayer ).choice;
    }

    void moveTitles()
    {
        wxArrayInt widths = m_LayerListFlexGridSizer->GetColWidths();

        int offset = 0;
        wxSize txtz;

        txtz = m_NameStaticText->GetSize();
        m_NameStaticText->Move( offset + (widths[0] - txtz.x)/2, 5 );
        offset += widths[0];

        txtz = m_EnabledStaticText->GetSize();
        m_EnabledStaticText->Move( offset + (widths[1] - txtz.x)/2, 5 );
        offset += widths[1];

        txtz = m_TypeStaticText->GetSize();
        m_TypeStaticText->Move( offset + (widths[2] - txtz.x)/2, 5 );
    }


public:
    DIALOG_LAYERS_SETUP( PCB_EDIT_FRAME* parent );
    ~DIALOG_LAYERS_SETUP( ) { };

    /**
     * Function Layout
     * overrides the standard Layout() function so that the column titles can
     * be positioned using information in the flexgridsizer.
     */
    bool Layout()
    {
        bool ret = DIALOG_LAYERS_SETUP_BASE::Layout();

        moveTitles();
        return ret;
    }
};


// Layer bit masks for each defined "Preset Layer Grouping"
static const LAYER_MSK presets[] =
{
    NO_LAYERS,  // shift the array index up by one, matches with "Custom".

    // "Two layers, parts on Front only"
    EDGE_LAYER | LAYER_FRONT | LAYER_BACK | FRONT_TECH_LAYERS,

    // "Two layers, parts on Back only",
    EDGE_LAYER | LAYER_FRONT | LAYER_BACK | BACK_TECH_LAYERS,

    // "Two layers, parts on Front and Back",
    EDGE_LAYER | LAYER_FRONT | LAYER_BACK | ALL_TECH_LAYERS,

    // "Four layers, parts on Front only"
    EDGE_LAYER | LAYER_FRONT | LAYER_BACK | LAYER_2 | LAYER_3 | FRONT_TECH_LAYERS,

    // "Four layers, parts on Front and Back"
    EDGE_LAYER | LAYER_FRONT | LAYER_BACK | LAYER_2 | LAYER_3 | ALL_TECH_LAYERS,

    //  "All layers on",
    ALL_LAYERS,
};


CTLs DIALOG_LAYERS_SETUP::getCTLs( LAYER_NUM aLayerNumber )
{
#define RETCOP(x)    return CTLs( x##Name, x##CheckBox, x##Choice, x##Panel );
#define RETAUX(x)    return CTLs( x##Name, x##CheckBox, x##StaticText, x##Panel );

    switch( aLayerNumber )
    {
    case ADHESIVE_N_FRONT:      RETAUX( m_AdhesFront );
    case SOLDERPASTE_N_FRONT:   RETAUX( m_SoldPFront );
    case SILKSCREEN_N_FRONT:    RETAUX( m_SilkSFront );
    case SOLDERMASK_N_FRONT:    RETAUX( m_MaskFront );
    case LAYER_N_FRONT:         RETCOP( m_Front );
#ifdef USE_LAYER_MANAGER_COPPER_LAYERS_ORDER
    case LAYER_N_15:            RETCOP( m_Inner2 );
    case LAYER_N_14:            RETCOP( m_Inner3 );
    case LAYER_N_13:            RETCOP( m_Inner4 );
    case LAYER_N_12:            RETCOP( m_Inner5 );
    case LAYER_N_11:            RETCOP( m_Inner6 );
    case LAYER_N_10:            RETCOP( m_Inner7 );
    case LAYER_N_9:             RETCOP( m_Inner8 );
    case LAYER_N_8:             RETCOP( m_Inner9 );
    case LAYER_N_7:             RETCOP( m_Inner10 );
    case LAYER_N_6:             RETCOP( m_Inner11 );
    case LAYER_N_5:             RETCOP( m_Inner12 );
    case LAYER_N_4:             RETCOP( m_Inner13 );
    case LAYER_N_3:             RETCOP( m_Inner14 );
    case LAYER_N_2:             RETCOP( m_Inner15 );
#else
    case LAYER_N_2:             RETCOP( m_Inner2 );
    case LAYER_N_3:             RETCOP( m_Inner3 );
    case LAYER_N_4:             RETCOP( m_Inner4 );
    case LAYER_N_5:             RETCOP( m_Inner5 );
    case LAYER_N_6:             RETCOP( m_Inner6 );
    case LAYER_N_7:             RETCOP( m_Inner7 );
    case LAYER_N_8:             RETCOP( m_Inner8 );
    case LAYER_N_9:             RETCOP( m_Inner9 );
    case LAYER_N_10:            RETCOP( m_Inner10 );
    case LAYER_N_11:            RETCOP( m_Inner11 );
    case LAYER_N_12:            RETCOP( m_Inner12 );
    case LAYER_N_13:            RETCOP( m_Inner13 );
    case LAYER_N_14:            RETCOP( m_Inner14 );
    case LAYER_N_15:            RETCOP( m_Inner15 );
#endif
    case LAYER_N_BACK:          RETCOP( m_Back );
    case SOLDERMASK_N_BACK:     RETAUX( m_MaskBack );
    case SILKSCREEN_N_BACK:     RETAUX( m_SilkSBack );
    case SOLDERPASTE_N_BACK:    RETAUX( m_SoldPBack );
    case ADHESIVE_N_BACK:       RETAUX( m_AdhesBack );
    case EDGE_N:                RETAUX( m_PCBEdges );
    case ECO2_N:                RETAUX( m_Eco2 );
    case ECO1_N:                RETAUX( m_Eco1 );
    case COMMENT_N:             RETAUX( m_Comments );
    case DRAW_N:                RETAUX( m_Drawings );
    default:
        // wxDEBUGMSG( "bad layer id" );
        return CTLs( 0, 0, 0 );
    }

#undef RETCOP
#undef RETAUX
}


/***********************************************************************************/
DIALOG_LAYERS_SETUP::DIALOG_LAYERS_SETUP( PCB_EDIT_FRAME* parent ) :
    DIALOG_LAYERS_SETUP_BASE( parent )
/***********************************************************************************/
{
    m_Parent    = parent;
    m_Pcb       = m_Parent->GetBoard();

    m_CopperLayerCount = m_Pcb->GetCopperLayerCount();
    showCopperChoice( m_CopperLayerCount );
    setCopperLayerCheckBoxes( m_CopperLayerCount );

    showBoardLayerNames();

    m_EnabledLayers = m_Pcb->GetEnabledLayers();
    showSelectedLayerCheckBoxes( m_EnabledLayers );
    showPresets( m_EnabledLayers );

    showLayerTypes();

    SetAutoLayout( true );

    // these 3 controls are handled outside wxformbuilder so that we can add
    // them without a sizer.  Then we position them manually based on the column
    // widths from m_LayerListFlexGridSizer->GetColWidths()
    m_NameStaticText = new wxStaticText( m_TitlePanel, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );

    m_EnabledStaticText = new wxStaticText( m_TitlePanel, wxID_ANY, _("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );

    m_TypeStaticText = new wxStaticText( m_TitlePanel, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );

    // set the height of the title panel to be the size of any wxStaticText object
    // plus 10 so we can have a border of 5 on both top and bottom.
    m_TitlePanel->SetMinSize( wxSize( -1, m_AdhesFrontName->GetSize().y+10 ) );

    Layout();
    Fit();

    Center();

    m_sdbSizer2OK->SetFocus();
    m_sdbSizer2OK->SetDefault();
}


void DIALOG_LAYERS_SETUP::showCopperChoice( int copperCount )
{
    static const int copperCounts[] = { 2,4,6,8,10,12,14,16 };

    for( unsigned i = 0;  i<sizeof(copperCounts);  ++i )
    {
        // note this will change a one layer board to 2:
        if( copperCount <= copperCounts[i] )
        {
            m_CopperLayersChoice->SetSelection(i);
            break;
        }
    }
}


void DIALOG_LAYERS_SETUP::showBoardLayerNames()
{
    // Establish all the board's layer names into the dialog presentation, by
    // obtaining them from BOARD::GetLayerName() which calls
    // BOARD::GetStandardLayerName() for non-coppers.

    for( LAYER_NUM layer=FIRST_LAYER; layer<NB_PCB_LAYERS;  ++layer )
    {
        wxControl*  ctl = getName( layer );

        wxASSERT( ctl );

        if( ctl )
        {
            wxString lname = m_Pcb->GetLayerName( layer );

            //D(printf("layerName[%d]=%s\n", layer, TO_UTF8( lname ) );)

            if( ctl->IsKindOf( CLASSINFO(wxTextCtrl) ) )
                ((wxTextCtrl*)ctl)->SetValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );     // wxStaticText
        }
    }
}


void DIALOG_LAYERS_SETUP::showSelectedLayerCheckBoxes( LAYER_MSK enabledLayers )
{
    for( LAYER_NUM layer=FIRST_LAYER; layer<NB_PCB_LAYERS; ++layer )
    {
        setLayerCheckBox( layer, GetLayerMask( layer ) & enabledLayers );
    }
}


void DIALOG_LAYERS_SETUP::showPresets( LAYER_MSK enabledLayers )
{
    int presetsNdx = 0;     // the "Custom" setting, matches nothing

    for( unsigned i=1; i<DIM(presets);  ++i )
    {
        if( enabledLayers == presets[i] )
        {
            presetsNdx = i;
            break;
        }
    }

    m_PresetsChoice->SetSelection( presetsNdx );
}


void DIALOG_LAYERS_SETUP::showLayerTypes()
{
    for( LAYER_NUM copperLayer = FIRST_COPPER_LAYER;
             copperLayer <= LAST_COPPER_LAYER; ++copperLayer )
    {
        wxChoice* ctl = getChoice( copperLayer );
        ctl->SetSelection( m_Pcb->GetLayerType( copperLayer ) );
    }
}


LAYER_MSK DIALOG_LAYERS_SETUP::getUILayerMask()
{
    LAYER_MSK layerMaskResult = NO_LAYERS;

    for( LAYER_NUM layer=FIRST_LAYER; layer<NB_PCB_LAYERS; ++layer )
    {
        wxCheckBox*  ctl = getCheckBox( layer );
        if( ctl->GetValue() )
        {
            layerMaskResult |= GetLayerMask( layer );
        }
    }

    return layerMaskResult;
}


void DIALOG_LAYERS_SETUP::setLayerCheckBox( LAYER_NUM aLayer, bool isChecked )
{
    wxCheckBox*  ctl = getCheckBox( aLayer );
    ctl->SetValue(  isChecked );
}


void DIALOG_LAYERS_SETUP::setCopperLayerCheckBoxes( int copperCount )
{
    if( copperCount > 0 )
    {
        setLayerCheckBox( LAYER_N_BACK, true );
        --copperCount;
    }

    if( copperCount > 0 )
    {
        setLayerCheckBox( LAYER_N_FRONT, true );
        --copperCount;
    }
    else
    {
        setLayerCheckBox( LAYER_N_FRONT, false );
    }

    for( LAYER_NUM layer=LAYER_N_2; layer < NB_COPPER_LAYERS-1; ++layer, --copperCount )
    {
        bool state = copperCount > 0;

#ifdef HIDE_INACTIVE_LAYERS
        // This code hides non-active copper layers, or redisplays hidden
        // layers which are now needed.
        CTLs ctl = getCTLs( layer );

        ctl.name->Show( state );
        ctl.checkbox->Show( state );
        ctl.choice->Show( state );

        if( ctl.panel )
            ctl.panel->Show( state );
#endif

        setLayerCheckBox( layer, state );
    }

#ifdef HIDE_INACTIVE_LAYERS
    // Send an size event to force sizers to be updated,
    // because the number of copper layers can have changed.
    wxSizeEvent evt_size(m_LayersListPanel->GetSize() );
    m_LayersListPanel->GetEventHandler()->ProcessEvent( evt_size );
#endif

}


void DIALOG_LAYERS_SETUP::OnCheckBox( wxCommandEvent& event )
{
    m_EnabledLayers = getUILayerMask();

    showPresets( m_EnabledLayers );
}


void DIALOG_LAYERS_SETUP::DenyChangeCheckBox( wxCommandEvent& event )
{
    // user may not change copper layer checkboxes from anything other than
    // either presets choice or the copper layer choice controls.

    // I tried to simply disable the copper CheckBoxes but they look like crap,
    // so leave them enabled and reverse the user's attempt to toggle them.

    setCopperLayerCheckBoxes( m_CopperLayerCount );
}


void DIALOG_LAYERS_SETUP::OnPresetsChoice( wxCommandEvent& event )
{
    unsigned presetNdx = m_PresetsChoice->GetCurrentSelection();

    if( presetNdx == 0 )        // the Custom setting controls nothing.
        return;

    if( presetNdx < DIM(presets) )
    {
        m_EnabledLayers = presets[ presetNdx ];

        int coppersMask = m_EnabledLayers & ALL_CU_LAYERS;

        int copperCount = 0;
        while( coppersMask )
        {
            if( coppersMask & 1 )
                ++copperCount;

            coppersMask >>= 1;
        }

        m_CopperLayerCount = copperCount;

        showCopperChoice( m_CopperLayerCount );

        showSelectedLayerCheckBoxes( m_EnabledLayers );

        setCopperLayerCheckBoxes( m_CopperLayerCount );
    }
}


void DIALOG_LAYERS_SETUP::OnCopperLayersChoice( wxCommandEvent& event )
{
    m_CopperLayerCount = m_CopperLayersChoice->GetCurrentSelection() * 2 + 2;

    setCopperLayerCheckBoxes( m_CopperLayerCount );

    m_EnabledLayers = getUILayerMask();

    showPresets( m_EnabledLayers );
}


/*****************************************************************/
void DIALOG_LAYERS_SETUP::OnCancelButtonClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( wxID_CANCEL );
}


/**************************************************************************/
void DIALOG_LAYERS_SETUP::OnOkButtonClick( wxCommandEvent& event )
/**************************************************************************/
{
    if( testLayerNames() )
    {
        wxString name;

        m_EnabledLayers = getUILayerMask();
        m_Pcb->SetEnabledLayers( m_EnabledLayers );

        /* Ensure enabled layers are also visible
         * This is mainly to avoid mistakes if some enabled
         * layers are not visible when exiting this dialog
         */
        m_Pcb->SetVisibleLayers( m_EnabledLayers );

        for( LAYER_NUM layer = FIRST_COPPER_LAYER;
                 layer <= LAST_COPPER_LAYER; ++layer )
        {
            if( GetLayerMask( layer ) & m_EnabledLayers )
            {
                name = getLayerName( layer );

                m_Pcb->SetLayerName( layer, name );

                LAYER_T t = (LAYER_T) getLayerTypeIndex(layer);

                m_Pcb->SetLayerType( layer, t );
            }
        }

        m_Parent->OnModify();
        m_Parent->ReCreateLayerBox();
        m_Parent->ReFillLayerWidget();

        EndModal( wxID_OK );
    }
}

int DIALOG_LAYERS_SETUP::getLayerTypeIndex( LAYER_NUM aLayer )
{
    wxChoice*  ctl =  getChoice( aLayer );

    int ret = ctl->GetCurrentSelection();   // indices must have same sequence as LAYER_T

    return ret;
}

wxString DIALOG_LAYERS_SETUP::getLayerName( LAYER_NUM aLayer )
{
    wxString ret;

    wxASSERT( IsCopperLayer( aLayer ) );

    wxTextCtrl*  ctl = (wxTextCtrl*) getName( aLayer );

    ret = ctl->GetValue().Trim();

    return ret;
}

static bool hasOneOf( const wxString& str, const wxString& chars )
{
    for( unsigned i=0; i<chars.Len();  ++i )
        if( str.Find( chars[i] ) != wxNOT_FOUND )
            return true;
    return false;
}

bool DIALOG_LAYERS_SETUP::testLayerNames()
{
    std::vector<wxString>    names;

    wxTextCtrl*  ctl;

    for( LAYER_NUM layer=FIRST_LAYER; layer<=LAST_COPPER_LAYER; ++layer )
    {
        // we _can_ rely on m_EnabledLayers being current here:
        if( !(m_EnabledLayers & GetLayerMask( layer )) )
            continue;

        wxString name = getLayerName( layer );

        //D(printf("name[%d]=%s\n", layer, TO_UTF8(name) );)

        ctl = (wxTextCtrl*) getName( layer );

        // check name for legality.
        // 1) cannot be blank.
        // 2) cannot have blanks.
        // 3) cannot have " chars
        // 4) cannot be 'signal'
        // 5) must be unique.
        // 6) cannot have illegal chars in filenames ( some filenames are built from layer names )
        static const wxString badchars( wxT("%$\" /\\") );

        if( name == wxEmptyString )
        {
            DisplayError( this, _("Layer name may not be empty" ) );
            ctl->SetFocus();    // on the bad name
            return false;
        }

        if( hasOneOf( name, badchars ) )
        {
            DisplayError( this, _("Layer name has an illegal character, one of: '") + badchars + wxT("'") );
            ctl->SetFocus();    // on the bad name
            return false;
        }

        if( name == wxT("signal") )
        {
            DisplayError( this, _("'signal' is a reserved layer name") );
            ctl->SetFocus();    // on the bad name
            return false;
        }

        for( std::vector<wxString>::iterator it = names.begin();  it != names.end();  ++it )
        {
            if( name == *it )
            {
                DisplayError( this, _("Layer name is a duplicate of another") );
                ctl->SetFocus();    // on the bad name
                return false;
            }
        }

        names.push_back( name );
    }

    return true;
}


void PCB_EDIT_FRAME::InstallDialogLayerSetup()
{
    DIALOG_LAYERS_SETUP dlg( this );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxLogDebug( wxT( "Current layer selected %d." ), GetActiveLayer() );

    // If the current active layer was removed, find the next avaiable layer to set as the
    // active layer.
    if( !( GetLayerMask( GetActiveLayer() ) & GetBoard()->GetEnabledLayers() ) )
    {
        for( LAYER_NUM i = FIRST_LAYER; i < NB_LAYERS; ++i )
        {
            LAYER_NUM tmp = i;

            if( i >= NB_LAYERS )
                tmp = i - NB_LAYERS;

            if( GetLayerMask( tmp ) & GetBoard()->GetEnabledLayers() )
            {
                wxLogDebug( wxT( "Setting current layer to  %d." ), GetActiveLayer() );
                SetActiveLayer( tmp, true );
                break;
            }
        }
    }
    else
    {
        SetActiveLayer( GetActiveLayer(), true );
    }
}
