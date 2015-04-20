/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 - 2015 KiCad Developers, see change_log.txt for contributors.
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
#include <invoke_pcb_dialog.h>

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


static LSEQ dlg_layers()
{
    // layers that are put out into the dialog UI, coordinate with wxformbuilder and
    // getCTLs( LAYER_NUM aLayerNumber )
    static const LAYER_ID layers[] = {
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

    return LSEQ( layers, layers + DIM( layers ) );
}


class DIALOG_LAYERS_SETUP : public DIALOG_LAYERS_SETUP_BASE
{
public:
    DIALOG_LAYERS_SETUP( wxTopLevelWindow* aCaller, BOARD* aBoard );

private:
    int             m_copperLayerCount;
    LSET            m_enabledLayers;

    BOARD*          m_pcb;

    wxStaticText*   m_nameStaticText;
    wxStaticText*   m_enabledStaticText;
    wxStaticText*   m_typeStaticText;

    void setLayerCheckBox( LAYER_NUM layer, bool isChecked );
    void setCopperLayerCheckBoxes( int copperCount );

    void showCopperChoice( int copperCount );
    void showBoardLayerNames();
    void showSelectedLayerCheckBoxes( LSET enableLayerMask );
    void showLayerTypes();
    void showPresets( LSET enabledLayerMask );

    /** return the selected layer mask within the UI checkboxes */
    LSET getUILayerMask();
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

        txtz = m_nameStaticText->GetSize();
        m_nameStaticText->Move( offset + (widths[0] - txtz.x)/2, 5 );
        offset += widths[0];

        txtz = m_enabledStaticText->GetSize();
        m_enabledStaticText->Move( offset + (widths[1] - txtz.x)/2, 5 );
        offset += widths[1];

        txtz = m_typeStaticText->GetSize();
        m_typeStaticText->Move( offset + (widths[2] - txtz.x)/2, 5 );
    }

    void OnSize( wxSizeEvent& event );
};


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
    LSET( 4, F_Cu, B_Cu, In1_Cu, In2_Cu ) | LSET::FrontTechMask() | LSET::BackTechMask() | LSET::UserMask(),

    //  "All layers on",
    LSET().set(),
};


CTLs DIALOG_LAYERS_SETUP::getCTLs( LAYER_NUM aLayerNumber )
{
#define RETCOP(x)    return CTLs( x##Name, x##CheckBox, x##Choice, x##Panel );
#define RETAUX(x)    return CTLs( x##Name, x##CheckBox, x##StaticText, x##Panel );

    switch( aLayerNumber )
    {
    case F_CrtYd:               RETAUX( m_CrtYdFront );
    case F_Fab:                 RETAUX( m_FabFront );
    case F_Adhes:               RETAUX( m_AdhesFront );
    case F_Paste:               RETAUX( m_SoldPFront );
    case F_SilkS:               RETAUX( m_SilkSFront );
    case F_Mask:                RETAUX( m_MaskFront );
    case F_Cu:                  RETCOP( m_Front );

    case In1_Cu:                RETCOP( m_In1 );
    case In2_Cu:                RETCOP( m_In2 );
    case In3_Cu:                RETCOP( m_In3 );
    case In4_Cu:                RETCOP( m_In4 );
    case In5_Cu:                RETCOP( m_In5 );
    case In6_Cu:                RETCOP( m_In6 );
    case In7_Cu:                RETCOP( m_In7 );
    case In8_Cu:                RETCOP( m_In8 );
    case In9_Cu:                RETCOP( m_In9 );
    case In10_Cu:               RETCOP( m_In10 );
    case In11_Cu:               RETCOP( m_In11 );
    case In12_Cu:               RETCOP( m_In12 );
    case In13_Cu:               RETCOP( m_In13 );
    case In14_Cu:               RETCOP( m_In14 );
    case In15_Cu:               RETCOP( m_In15 );

    case In16_Cu:               RETCOP( m_In16 );
    case In17_Cu:               RETCOP( m_In17 );
    case In18_Cu:               RETCOP( m_In18 );
    case In19_Cu:               RETCOP( m_In19 );
    case In20_Cu:               RETCOP( m_In20 );
    case In21_Cu:               RETCOP( m_In21 );
    case In22_Cu:               RETCOP( m_In22 );
    case In23_Cu:               RETCOP( m_In23 );
    case In24_Cu:               RETCOP( m_In24 );
    case In25_Cu:               RETCOP( m_In25 );
    case In26_Cu:               RETCOP( m_In26 );
    case In27_Cu:               RETCOP( m_In27 );
    case In28_Cu:               RETCOP( m_In28 );
    case In29_Cu:               RETCOP( m_In29 );
    case In30_Cu:               RETCOP( m_In30 );

    case B_Cu:                  RETCOP( m_Back );
    case B_Mask:                RETAUX( m_MaskBack );
    case B_SilkS:               RETAUX( m_SilkSBack );
    case B_Paste:               RETAUX( m_SoldPBack );
    case B_Adhes:               RETAUX( m_AdhesBack );
    case B_Fab:                 RETAUX( m_FabBack );
    case B_CrtYd:               RETAUX( m_CrtYdBack );

    case Edge_Cuts:             RETAUX( m_PCBEdges );
    case Margin:                RETAUX( m_Margin );
    case Eco2_User:             RETAUX( m_Eco2 );
    case Eco1_User:             RETAUX( m_Eco1 );
    case Cmts_User:             RETAUX( m_Comments );
    case Dwgs_User:             RETAUX( m_Drawings );
    default:
        wxASSERT_MSG( 0, wxT( "bad layer id" ) );
        return CTLs( 0, 0, 0 );
    }

#undef RETCOP
#undef RETAUX
}


DIALOG_LAYERS_SETUP::DIALOG_LAYERS_SETUP( wxTopLevelWindow* aParent, BOARD* aBoard ) :
    DIALOG_LAYERS_SETUP_BASE( aParent )
{
    m_pcb = aBoard;

    m_copperLayerCount = m_pcb->GetCopperLayerCount();
    showCopperChoice( m_copperLayerCount );
    setCopperLayerCheckBoxes( m_copperLayerCount );
    m_staticTextBrdThicknessUnit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    PutValueInLocalUnits( *m_textCtrlBrdThickness,
                          m_pcb->GetDesignSettings().GetBoardThickness() );

    showBoardLayerNames();

    m_enabledLayers = m_pcb->GetEnabledLayers();
    showSelectedLayerCheckBoxes( m_enabledLayers );
    showPresets( m_enabledLayers );

    showLayerTypes();

    SetAutoLayout( true );

    // these 3 controls are handled outside wxformbuilder so that we can add
    // them without a sizer.  Then we position them manually based on the column
    // widths from m_LayerListFlexGridSizer->GetColWidths()
    m_nameStaticText = new wxStaticText( m_TitlePanel, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );

    m_enabledStaticText = new wxStaticText( m_TitlePanel, wxID_ANY, _("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );

    m_typeStaticText = new wxStaticText( m_TitlePanel, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );

    // set the height of the title panel to be the size of any wxStaticText object
    // plus 10 so we can have a border of 5 on both top and bottom.
    m_TitlePanel->SetMinSize( wxSize( -1, m_AdhesFrontName->GetSize().y+10 ) );

    m_LayersListPanel->ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );

    Layout();
    Fit();
    moveTitles();

    Center();

    m_sdbSizerOK->SetFocus();
    m_sdbSizerOK->SetDefault();
}

void DIALOG_LAYERS_SETUP::OnSize( wxSizeEvent& event )
{
    moveTitles();
    event.Skip();
}

void DIALOG_LAYERS_SETUP::showCopperChoice( int copperCount )
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


void DIALOG_LAYERS_SETUP::showBoardLayerNames()
{
    // Establish all the board's layer names into the dialog presentation, by
    // obtaining them from BOARD::GetLayerName() which calls
    // BOARD::GetStandardLayerName() for non-coppers.

    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        wxControl*  ctl = getName( layer );

        wxASSERT( ctl );

        if( ctl )
        {
            wxString lname = m_pcb->GetLayerName( layer );

            //D(printf("layerName[%d]=%s\n", layer, TO_UTF8( lname ) );)

            if( ctl->IsKindOf( CLASSINFO(wxTextCtrl) ) )
                ((wxTextCtrl*)ctl)->SetValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );     // wxStaticText
        }
    }
}


void DIALOG_LAYERS_SETUP::showSelectedLayerCheckBoxes( LSET enabledLayers )
{
    // the check boxes
    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        setLayerCheckBox( layer, enabledLayers[layer] );
    }
}


void DIALOG_LAYERS_SETUP::showPresets( LSET enabledLayers )
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
    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        LAYER_ID cu_layer = *seq;

        wxChoice* ctl = getChoice( cu_layer );
        ctl->SetSelection( m_pcb->GetLayerType( cu_layer ) );
    }
}


LSET DIALOG_LAYERS_SETUP::getUILayerMask()
{
    LSET layerMaskResult;

    for( LSEQ seq = dlg_layers();  seq;  ++seq )
    {
        LAYER_ID    layer = *seq;
        wxCheckBox* ctl = getCheckBox( layer );

        if( ctl->GetValue() )
        {
            layerMaskResult.set( layer );
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
        LAYER_ID layer = *seq;
        bool     state = copperCount > 0;

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
    wxSizeEvent evt_size( m_LayersListPanel->GetSize() );
    m_LayersListPanel->GetEventHandler()->ProcessEvent( evt_size );
#endif
}


void DIALOG_LAYERS_SETUP::OnCheckBox( wxCommandEvent& event )
{
    m_enabledLayers = getUILayerMask();

    showPresets( m_enabledLayers );
}


void DIALOG_LAYERS_SETUP::DenyChangeCheckBox( wxCommandEvent& event )
{
    // user may not change copper layer checkboxes from anything other than
    // either presets choice or the copper layer choice controls.

    // I tried to simply disable the copper CheckBoxes but they look like crap,
    // so leave them enabled and reverse the user's attempt to toggle them.

    setCopperLayerCheckBoxes( m_copperLayerCount );
}


void DIALOG_LAYERS_SETUP::OnPresetsChoice( wxCommandEvent& event )
{
    unsigned presetNdx = m_PresetsChoice->GetCurrentSelection();

    if( presetNdx == 0 )        // the Custom setting controls nothing.
        return;

    if( presetNdx < DIM(presets) )
    {
        m_enabledLayers = presets[ presetNdx ];

        LSET copperSet = m_enabledLayers & LSET::AllCuMask();

        int copperCount = copperSet.count();

        m_copperLayerCount = copperCount;

        showCopperChoice( m_copperLayerCount );

        showSelectedLayerCheckBoxes( m_enabledLayers );

        setCopperLayerCheckBoxes( m_copperLayerCount );
    }
}


void DIALOG_LAYERS_SETUP::OnCopperLayersChoice( wxCommandEvent& event )
{
    m_copperLayerCount = m_CopperLayersChoice->GetCurrentSelection() * 2 + 2;

    setCopperLayerCheckBoxes( m_copperLayerCount );

    m_enabledLayers = getUILayerMask();

    showPresets( m_enabledLayers );
}


void DIALOG_LAYERS_SETUP::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_LAYERS_SETUP::OnOkButtonClick( wxCommandEvent& event )
{
    if( testLayerNames() )
    {
        wxString name;

        m_enabledLayers = getUILayerMask();
        m_pcb->SetEnabledLayers( m_enabledLayers );

        /* Ensure enabled layers are also visible
         * This is mainly to avoid mistakes if some enabled
         * layers are not visible when exiting this dialog
         */
        m_pcb->SetVisibleLayers( m_enabledLayers );

        for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
        {
            LAYER_ID  layer = *seq;

            if( m_enabledLayers[layer] )
            {
                name = getLayerName( layer );

                m_pcb->SetLayerName( layer, name );

                LAYER_T t = (LAYER_T) getLayerTypeIndex( layer );

                m_pcb->SetLayerType( layer, t );
            }
        }

        int thickness = ValueFromTextCtrl( *m_textCtrlBrdThickness );

        // Clamp the value between reasonable values

        thickness = Clamp( Millimeter2iu( 0.1 ), thickness, Millimeter2iu( 3.0 ) );
        m_pcb->GetDesignSettings().SetBoardThickness( thickness );

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

    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        // we _can_ rely on m_enabledLayers being current here:
        if( !m_enabledLayers[layer] )
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

        if( !name )
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


bool InvokeLayerSetup( wxTopLevelWindow* aCaller, BOARD* aBoard )
{
    DIALOG_LAYERS_SETUP dlg( aCaller, aBoard );

    return dlg.ShowModal() == wxID_OK;
}
