/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Kicad Developers, see change_log.txt for contributors.
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


/* functions relatives to the design rules editor
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "dialog_layers_setup_base.h"

#include "class_board_design_settings.h"


/**
 * Struct CTLs
 * holds the 3 ui control pointers for a single board layer.
 */
struct CTLs
{
    CTLs( wxControl* aName, wxCheckBox* aCheckBox, wxControl* aChoiceOrDesc )
    {
        name     = aName;
        checkbox = aCheckBox;
        choice   = aChoiceOrDesc;
    }

    wxControl*      name;
    wxCheckBox*     checkbox;
    wxControl*      choice;
};


class DIALOG_LAYERS_SETUP : public DIALOG_LAYERS_SETUP_BASE
{
private:
    static wxPoint      s_LastPos;
    static wxSize       s_LastSize;

    WinEDA_PcbFrame*    m_Parent;

    int                 m_CopperLayerCount;
    int                 m_EnabledLayers;

    BOARD*              m_Pcb;

    void init();

    void setLayerCheckBox( int layer, bool isChecked );
    void setCopperLayerCheckBoxes( int copperCount );

    void showCopperChoice( int copperCount );
    void showBoardLayerNames();
    void showSelectedLayerCheckBoxes( int enableLayerMask );
    void showLayerTypes();
    void showPresets( int enabledLayerMask );

    /** return the selected layer mask within the UI checkboxes */
    int getUILayerMask();
    wxString getLayerName( int layer );
    int getLayerTypeIndex( int layer );


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
    CTLs getCTLs( int aLayerNumber );

    wxControl* getName( int aLayer )
    {
        return getCTLs( aLayer ).name;
    }

    wxCheckBox* getCheckBox( int aLayer )
    {
        return getCTLs( aLayer ).checkbox;
    }

    wxChoice* getChoice( int aLayer )
    {
        return (wxChoice*) getCTLs( aLayer ).choice;
    }


public:
    DIALOG_LAYERS_SETUP( WinEDA_PcbFrame* parent );
    ~DIALOG_LAYERS_SETUP( ) { };

    bool Show( bool show );     // overload stock function

};


// We want our dialog to remember its previous screen position
wxPoint DIALOG_LAYERS_SETUP::s_LastPos( -1, -1 );
wxSize  DIALOG_LAYERS_SETUP::s_LastSize;


// Layer bit masks for each defined "Preset Layer Grouping"
static const int presets[] =
{
#define FRONT_AUX   (SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CMP  | ADHESIVE_LAYER_CMP | SOLDERPASTE_LAYER_CMP)
#define BACK_AUX    (SILKSCREEN_LAYER_CU  | SOLDERMASK_LAYER_CU   | ADHESIVE_LAYER_CU  | SOLDERPASTE_LAYER_CU)

    0,  // shift the array index up by one, matches with "Custom".

    // "Two layers, parts on Front only"
    EDGE_LAYER | CMP_LAYER | CUIVRE_LAYER | FRONT_AUX,

    // "Two layers, parts on Back only",
    EDGE_LAYER | CMP_LAYER | CUIVRE_LAYER | BACK_AUX,

    // "Two layers, parts on Front and Back",
    EDGE_LAYER | CMP_LAYER | CUIVRE_LAYER | BACK_AUX | FRONT_AUX,

    // "Four layers, parts on Front only"
    EDGE_LAYER | CMP_LAYER | CUIVRE_LAYER | LAYER_2 | LAYER_3 | FRONT_AUX,

    // "Four layers, parts on Front and Back"
    EDGE_LAYER | CMP_LAYER | CUIVRE_LAYER | LAYER_2 | LAYER_3 | FRONT_AUX | BACK_AUX,

    //  "All layers on",
    ALL_LAYERS,
};


CTLs DIALOG_LAYERS_SETUP::getCTLs( int aLayerNumber )
{
#define RETCOP(x)    return CTLs( x##Name, x##CheckBox, x##Choice );
#define RETAUX(x)    return CTLs( x##Name, x##CheckBox, x##StaticText );

    switch( aLayerNumber )
    {
    case ADHESIVE_N_CMP:    RETAUX( m_AdhesFront );
    case SOLDERPASTE_N_CMP: RETAUX( m_SoldPFront );
    case SILKSCREEN_N_CMP:  RETAUX( m_SilkSFront );
    case SOLDERMASK_N_CMP:  RETAUX( m_MaskFront );
    case LAYER_N_FRONT:     RETCOP( m_Front );
    case LAYER_N_2:         RETCOP( m_Inner2 );
    case LAYER_N_3:         RETCOP( m_Inner3 );
    case LAYER_N_4:         RETCOP( m_Inner4 );
    case LAYER_N_5:         RETCOP( m_Inner5 );
    case LAYER_N_6:         RETCOP( m_Inner6 );
    case LAYER_N_7:         RETCOP( m_Inner7 );
    case LAYER_N_8:         RETCOP( m_Inner8 );
    case LAYER_N_9:         RETCOP( m_Inner9 );
    case LAYER_N_10:        RETCOP( m_Inner10 );
    case LAYER_N_11:        RETCOP( m_Inner11 );
    case LAYER_N_12:        RETCOP( m_Inner12 );
    case LAYER_N_13:        RETCOP( m_Inner13 );
    case LAYER_N_14:        RETCOP( m_Inner14 );
    case LAYER_N_15:        RETCOP( m_Inner15 );
    case LAYER_N_BACK:      RETCOP( m_Back );
    case SOLDERMASK_N_CU:   RETAUX( m_MaskBack );
    case SILKSCREEN_N_CU:   RETAUX( m_SilkSBack );
    case SOLDERPASTE_N_CU:  RETAUX( m_SoldPBack );
    case ADHESIVE_N_CU:     RETAUX( m_AdhesBack );
    case EDGE_N:            RETAUX( m_PCBEdges );
    case ECO2_N:            RETAUX( m_Eco2 );
    case ECO1_N:            RETAUX( m_Eco1 );
    case COMMENT_N:         RETAUX( m_Comments );
    case DRAW_N:            RETAUX( m_Drawings );
    default:
        // wxDEBUGMSG( "bad layer id" );
        return CTLs( 0, 0, 0 );
    }

#undef RETCOP
#undef RETAUX
}


/***********************************************************************************/
DIALOG_LAYERS_SETUP::DIALOG_LAYERS_SETUP( WinEDA_PcbFrame* parent ) :
    DIALOG_LAYERS_SETUP_BASE( parent )
/***********************************************************************************/
{
    m_Parent    = parent;
    m_Pcb       = m_Parent->GetBoard();

    init();

    SetAutoLayout( true );
    Layout();

    Center();

    m_sdbSizer2OK->SetFocus();
}


bool DIALOG_LAYERS_SETUP::Show( bool show )
{
    bool ret;

    if( show )
    {
        if( s_LastPos.x != -1 )
        {
            SetSize( s_LastPos.x, s_LastPos.y, s_LastSize.x, s_LastSize.y, 0 );
        }
        ret = DIALOG_LAYERS_SETUP_BASE::Show( show );
    }
    else
    {
        // Save the dialog's position before hiding
        s_LastPos  = GetPosition();
        s_LastSize = GetSize();

        ret = DIALOG_LAYERS_SETUP_BASE::Show( show );
    }

    return ret;
}


void DIALOG_LAYERS_SETUP::showCopperChoice( int copperCount )
{
    static const int copperCounts[] = { 2,4,6,8,10,12,14,16 };

    D(printf("boardsCopperCount=%d\n", copperCount );)

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
    // BOARD::GetDefaultLayerName() for non-coppers.

    for( int layer=0; layer<NB_LAYERS;  ++layer )
    {
        wxControl*  ctl = getName( layer );

        wxASSERT( ctl );

        if( ctl )
        {
            wxString lname = m_Pcb->GetLayerName( layer );

            D(printf("layerName[%d]=%s\n", layer, CONV_TO_UTF8( lname ) );)

            if( ctl->IsKindOf( CLASSINFO(wxTextCtrl) ) )
                ((wxTextCtrl*)ctl)->SetValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );     // wxStaticText
        }
    }
}


void DIALOG_LAYERS_SETUP::showSelectedLayerCheckBoxes( int enabledLayers )
{
    for( int layer=0;  layer<NB_LAYERS;  ++layer )
    {
        setLayerCheckBox( layer, (1<<layer) & enabledLayers  );
    }
}


void DIALOG_LAYERS_SETUP::showPresets( int enabledLayers )
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
    for( int copperLayer =  FIRST_COPPER_LAYER;
             copperLayer <= LAST_COPPER_LAYER; ++copperLayer )
    {
        wxChoice* ctl = getChoice( copperLayer );
        ctl->SetSelection( m_Pcb->GetLayerType( copperLayer ) );
    }
}


/********************************************************************/
void DIALOG_LAYERS_SETUP::init()
/********************************************************************/
{
    m_CopperLayerCount = m_Pcb->GetCopperLayerCount();
    showCopperChoice( m_CopperLayerCount );

    showBoardLayerNames();

    m_EnabledLayers = m_Pcb->GetEnabledLayers();
    showSelectedLayerCheckBoxes( m_EnabledLayers );
    showPresets( m_EnabledLayers );

    showLayerTypes();

    // @todo overload a layout function so we can reposition the column titles,
    // which should probably not go in a sizer of their own so that we do not have
    // to fight to position them, Dick.  Will work this out next.


    // Adjust the vertical scroll rate so our list scrolls always one full line each time.
//    m_LayersListPanel->SetScrollRate( 0, m_textCtrl1[0]->GetSize().y );
}


int DIALOG_LAYERS_SETUP::getUILayerMask()
{
    int layerMaskResult = 0;

    for( int layer=0;  layer<NB_LAYERS;  ++layer )
    {
        wxCheckBox*  ctl = getCheckBox( layer );
        if( ctl->GetValue() )
        {
            layerMaskResult |= (1 << layer);
        }
    }

    return layerMaskResult;
}


void DIALOG_LAYERS_SETUP::setLayerCheckBox( int aLayer, bool isChecked )
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

    int layer;
    for( layer=LAYER_N_2;  copperCount > 0;  ++layer, --copperCount )
    {
        setLayerCheckBox( layer, true );
    }

    for( ;  layer < NB_COPPER_LAYERS-1;  ++layer )
    {
        setLayerCheckBox( layer, false );
    }
}


void DIALOG_LAYERS_SETUP::OnCheckBox( wxCommandEvent& event )
{
    m_EnabledLayers = getUILayerMask();

    showPresets( m_EnabledLayers );
}


void DIALOG_LAYERS_SETUP::DenyChangeCheckBox( wxCommandEvent& event )
{
    // user may not change copper layer checkboxes from anything other than
    // the one place, the drop down m_CopperLayersChoice control.

    // I tried to simply diable the copper CheckBoxes but they look like crap,
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
    }
}


void DIALOG_LAYERS_SETUP::OnCopperLayersChoice( wxCommandEvent& event )
{
    m_CopperLayerCount = m_CopperLayersChoice->GetCurrentSelection() * 2 + 2;

    setCopperLayerCheckBoxes( m_CopperLayerCount );

//    event.Skip();
}



/*****************************************************************/
void DIALOG_LAYERS_SETUP::OnCancelButtonClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( 0 );
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

        for( int layer =  FIRST_COPPER_LAYER;
                 layer <= LAST_COPPER_LAYER; ++layer )
        {
            if( (1<<layer) & m_EnabledLayers )
            {
                name = getLayerName( layer );

                m_Pcb->SetLayerName( layer, name );

                LAYER_T t = (LAYER_T) getLayerTypeIndex(layer);

                m_Pcb->SetLayerType( layer, t );
            }
        }

        m_Parent->ReCreateLayerBox( NULL );

        EndModal( wxID_OK );
    }
}

int DIALOG_LAYERS_SETUP::getLayerTypeIndex( int aLayer )
{
    wxChoice*  ctl =  getChoice( aLayer );

    int ret = ctl->GetCurrentSelection();   // indices must have same sequence as LAYER_T

    return ret;
}

wxString DIALOG_LAYERS_SETUP::getLayerName( int aLayer )
{
    wxString ret;

    wxASSERT( aLayer >= FIRST_COPPER_LAYER && aLayer <= LAST_COPPER_LAYER );

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

    for( int layer=0;  layer<=LAST_COPPER_LAYER;  ++layer )
    {
        // we _can_ rely on m_EnabledLayers being current here:
        if( !(m_EnabledLayers & (1<<layer)) )
            continue;

        wxString name = getLayerName( layer );

        //D(printf("name[%d]=%s\n", layer, CONV_TO_UTF8(name) );)

        ctl = (wxTextCtrl*) getName( layer );

        // check name for legality.
        // 1) cannot be blank.
        // 2) cannot have blanks.
        // 3) cannot have " chars
        // 4) cannot be 'signal'
        // 5) must be unique.

        static const wxString badchars( wxT("%$\" ") );

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



void DisplayDialogLayerSetup( WinEDA_PcbFrame* parent )
{
    DIALOG_LAYERS_SETUP frame( parent );

    frame.ShowModal();
    frame.Destroy();
}

