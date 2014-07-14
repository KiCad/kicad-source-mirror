/**
 * @file select_layers_to_pcb.cpp
 * @brief Dialog to choose equivalence between gerber layers and pcb layers
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <kiface_i.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <class_GERBER.h>

#include <select_layers_to_pcb.h>

// Imported function
extern const wxString GetPCBDefaultLayerName( LAYER_NUM aLayerNumber );

enum swap_layer_id {
    ID_LAYERS_MAP_DIALOG = ID_GERBER_END_LIST,
    ID_BUTTON_0,
    ID_TEXT_0 = ID_BUTTON_0 + GERBER_DRAWLAYERS_COUNT
};


/*
 * This dialog shows the gerber files loaded, and allows user to choose:
 *   what gerber file and what board layer are used
 *   the number of copper layers
 */

int LAYERS_MAP_DIALOG::m_exportBoardCopperLayersCount = 2;


BEGIN_EVENT_TABLE( LAYERS_MAP_DIALOG, LAYERS_MAP_DIALOG_BASE )
    EVT_COMMAND_RANGE( ID_BUTTON_0, ID_BUTTON_0 + GERBER_DRAWLAYERS_COUNT-1,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       LAYERS_MAP_DIALOG::OnSelectLayer )
END_EVENT_TABLE()


LAYERS_MAP_DIALOG::LAYERS_MAP_DIALOG( GERBVIEW_FRAME* parent ) :
    LAYERS_MAP_DIALOG_BASE( parent )
{
    m_Parent = parent;
    initDialog();

    // Resize the dialog
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void LAYERS_MAP_DIALOG::initDialog()
{
    wxStaticText* label;
    wxStaticText* text;
    int           item_ID;
    wxString      msg;
    wxSize        goodSize;

    m_flexRightColumnBoxSizer = NULL;

    // Experimentation has shown that buttons in the Windows version can be 20
    // pixels wide and 20 pixels high, but that they need to be 26 pixels wide
    // and 26 pixels high in the Linux version. (And although the dimensions
    // of those buttons could be set to 26 pixels wide and 26 pixels high in
    // both of those versions, that would result in a dialog box which would
    // be excessively high in the Windows version.)
#ifdef __WINDOWS__
    int w = 20;
    int h = 20;
#else
    int w = 26;
    int h = 26;
#endif

    // As currently implemented, the dimensions of the buttons in the Mac
    // version are also 26 pixels wide and 26 pixels high. If appropriate,
    // the above code should be modified as required in the event that those
    // buttons should be some other size in that version.
    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        // Specify the default value for each member of these arrays.
        m_buttonTable[ii] = -1;
        m_layersLookUpTable[ii] = UNSELECTED_LAYER;
    }

    // Ensure we have:
    //    at least 2 copper layers and less than max pacb copper layers count
    //    and even layers count because a board *must* have even layers count
    normalizeBrdLayersCount();

    int idx = ( m_exportBoardCopperLayersCount / 2 ) - 1;
    m_comboCopperLayersCount->SetSelection( idx );

    LAYER_NUM pcb_layer_num = 0;
    m_gerberActiveLayersCount = 0;
    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        if( g_GERBER_List[ii] == NULL )
            break;

        if( (pcb_layer_num == m_exportBoardCopperLayersCount - 1)
           && (m_exportBoardCopperLayersCount > 1) )
            pcb_layer_num = F_Cu;

        m_buttonTable[m_gerberActiveLayersCount] = ii;
        m_layersLookUpTable[ii]  = pcb_layer_num;
        m_gerberActiveLayersCount++;
        ++pcb_layer_num;
    }

    if( m_gerberActiveLayersCount <= GERBER_DRAWLAYERS_COUNT/2 )    // Only one list is enough
    {
        m_staticlineSep->Hide();
    }
    else        // Add the second list of gerber files
    {
        m_flexRightColumnBoxSizer = new wxFlexGridSizer( 16, 4, 0, 0 );
        for( int ii = 0; ii < 4; ii++ )
            m_flexRightColumnBoxSizer->AddGrowableCol( ii );
        m_flexRightColumnBoxSizer->SetFlexibleDirection( wxBOTH );
        m_flexRightColumnBoxSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
        sbSizerLayersTable->Add( m_flexRightColumnBoxSizer, 1, wxEXPAND, 5 );
    }

    wxFlexGridSizer* flexColumnBoxSizer = m_flexLeftColumnBoxSizer;
    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        // Each Gerber layer has an associated static text string (to
        // identify that layer), a button (for invoking a child dialog
        // box to change which Pcbnew layer that the Gerber layer is
        // mapped to), and a second static text string (to depict which
        // Pcbnew layer that the Gerber layer has been mapped to). Each
        // of those items are placed into the left hand column, middle
        // column, and right hand column (respectively) of the Flexgrid
        // sizer, and the color of the second text string is set to
        // fuchsia or blue (to respectively indicate whether the Gerber
        // layer has been mapped to a Pcbnew layer or is not being
        // exported at all).  (Experimentation has shown that if a text
        // control is used to depict which Pcbnew layer that each Gerber
        // layer is mapped to (instead of a static text string), then
        // those controls do not behave in a fully satisfactory manner
        // in the Linux version. Even when the read-only attribute is
        // specified for all of those controls, they can still be selected
        // when the arrow keys or Tab key is used to step through all of
        // the controls within the dialog box, and directives to set the
        // foreground color of the text of each such control to blue (to
        // indicate that the text is of a read-only nature) are disregarded.
        // Specify a FlexGrid sizer with an appropriate number of rows
        // and three columns.  If nb_items < 16, then the number of rows
        // is nb_items; otherwise, the number of rows is 16 (with two
        // separate columns of controls being used if nb_items > 16).

        if( ii == GERBER_DRAWLAYERS_COUNT/2 )
            flexColumnBoxSizer = m_flexRightColumnBoxSizer;

        // Provide a text string to identify the Gerber layer
        msg.Printf( _( "Layer %d" ), m_buttonTable[ii] + 1 );

        label = new wxStaticText( this, wxID_STATIC, msg, wxDefaultPosition,
                                  wxDefaultSize, wxALIGN_RIGHT );
        flexColumnBoxSizer->Add( label, 0,
                                 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL |
                                 wxRIGHT | wxLEFT, 5 );

        /* Add file name and extension without path. */
        wxFileName fn( g_GERBER_List[ii]->m_FileName );
        label = new wxStaticText( this, wxID_STATIC, fn.GetFullName(),
                                  wxDefaultPosition, wxDefaultSize );
        flexColumnBoxSizer->Add( label, 0,
                                 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL |
                                 wxRIGHT | wxLEFT, 5 );

        // Provide a button for this layer (which will invoke a child dialog box)
        item_ID = ID_BUTTON_0 + ii;
        wxButton * Button = new wxButton( this, item_ID, wxT( "..." ),
                                        wxDefaultPosition, wxSize( w, h ), 0 );

        flexColumnBoxSizer->Add( Button, 0,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

        // Provide another text string to specify which Pcbnew layer that this
        // Gerber layer is initially mapped to, and set the initial text to
        // specify the appropriate Pcbnew layer, and set the foreground color
        // of the text to fuchsia (to indicate that the layer is being exported).
        item_ID = ID_TEXT_0 + ii;

        // When the first of these text strings is being added, determine what
        // size is necessary to to be able to display any possible string
        // without it being truncated. Then specify that size as the minimum
        // size for all of these text strings. (If this minimum size is not
        // determined in this fashion, then it is possible for the display of
        // one or more of these strings to be truncated after different Pcbnew
        // layers are selected.)
        if( ii == 0 )
        {
            msg  = _( "Do not export" );
            text = new wxStaticText( this, item_ID, msg, wxDefaultPosition,
                                     wxDefaultSize, 0 );
            goodSize = text->GetSize();

            for( LAYER_NUM jj = 0; jj < GERBER_DRAWLAYERS_COUNT; ++jj )
            {
                text->SetLabel( GetPCBDefaultLayerName( jj ) );
                if( goodSize.x < text->GetSize().x )
                    goodSize.x = text->GetSize().x;
            }

            msg = GetPCBDefaultLayerName( m_layersLookUpTable[m_buttonTable[ii]] );
            text->SetLabel( msg );
        }
        else
        {
            msg  = GetPCBDefaultLayerName( m_layersLookUpTable[m_buttonTable[ii]] );
            text = new wxStaticText( this, item_ID, msg, wxDefaultPosition,
                                     wxDefaultSize, 0 );
        }
        text->SetMinSize( goodSize );
        flexColumnBoxSizer->Add( text, 1,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT,
                                 5 );

        m_layersList[ii] = text;
    }
}

/* Ensure m_exportBoardCopperLayersCount = 2 to BOARD_COPPER_LAYERS_MAX_COUNT
 * and it is an even value because Boards have always an even layer count
 */
void LAYERS_MAP_DIALOG::normalizeBrdLayersCount()
{
    if( ( m_exportBoardCopperLayersCount & 1 ) )
        m_exportBoardCopperLayersCount++;

    if( m_exportBoardCopperLayersCount > GERBER_DRAWLAYERS_COUNT )
        m_exportBoardCopperLayersCount = GERBER_DRAWLAYERS_COUNT;

    if( m_exportBoardCopperLayersCount < 2 )
        m_exportBoardCopperLayersCount = 2;

}

/*
 * Called when user change the current board copper layers count
 */
void LAYERS_MAP_DIALOG::OnBrdLayersCountSelection( wxCommandEvent& event )
{
    int id = event.GetSelection();
    m_exportBoardCopperLayersCount = (id+1) * 2;
}

/*
 * reset pcb layers selection to the default value
 */
void LAYERS_MAP_DIALOG::OnResetClick( wxCommandEvent& event )
{
    wxString msg;
    int ii;
    LAYER_NUM layer;
    for( ii = 0, layer = 0; ii < m_gerberActiveLayersCount; ii++, ++layer )
    {
        if( (layer == m_exportBoardCopperLayersCount - 1)
           && (m_exportBoardCopperLayersCount > 1) )
            layer = F_Cu;
        m_layersLookUpTable[ii] = layer;
        msg = GetPCBDefaultLayerName( layer );
        m_layersList[ii]->SetLabel( msg );
        m_layersList[ii]->SetForegroundColour( wxNullColour );
        m_buttonTable[ii] = ii;
    }
}


/* Stores the current layers selection in config
 */
void LAYERS_MAP_DIALOG::OnStoreSetup( wxCommandEvent& event )
{
    wxConfigBase* config = Kiface().KifaceSettings();
    config->Write( wxT("BrdLayersCount"), m_exportBoardCopperLayersCount );

    wxString key;
    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        key.Printf( wxT("GbrLyr%dToPcb"), ii );
        config->Write( key, m_layersLookUpTable[ii] );
    }
}

void LAYERS_MAP_DIALOG::OnGetSetup( wxCommandEvent& event )
{
    wxConfigBase* config = Kiface().KifaceSettings();

    config->Read( wxT("BrdLayersCount"), &m_exportBoardCopperLayersCount );
    normalizeBrdLayersCount();

    int idx = ( m_exportBoardCopperLayersCount / 2 ) - 1;
    m_comboCopperLayersCount->SetSelection( idx );

    wxString key;
    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        key.Printf( wxT("GbrLyr%dToPcb"), ii );
        int ilayer;
        config->Read( key, &ilayer);
        m_layersLookUpTable[ii] = ilayer;
    }

    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        LAYER_NUM layer = m_layersLookUpTable[ii];
        if( layer == UNSELECTED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Do not export" ) );
            m_layersList[ii]->SetForegroundColour( *wxBLUE );
        }
        else
        {
            m_layersList[ii]->SetLabel( GetPCBDefaultLayerName( layer ) );
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
    }
}

void LAYERS_MAP_DIALOG::OnSelectLayer( wxCommandEvent& event )
{
    int ii;

    ii = event.GetId() - ID_BUTTON_0;

    if( (ii < 0) || (ii >= GERBER_DRAWLAYERS_COUNT) )
    {
        wxFAIL_MSG( wxT("Bad layer id") );
        return;
    }

    LAYER_NUM jj = m_layersLookUpTable[m_buttonTable[ii]];

    if( jj != UNSELECTED_LAYER && !IsValidLayer( jj ) )
        jj = B_Cu;  // (Defaults to "Copper" layer.)

    jj = m_Parent->SelectPCBLayer( jj, m_exportBoardCopperLayersCount, true );

    if( jj != UNSELECTED_LAYER && !IsValidLayer( jj ) )
        return;

    if( jj != m_layersLookUpTable[m_buttonTable[ii]] )
    {
        m_layersLookUpTable[m_buttonTable[ii]] = jj;

        if( jj == UNSELECTED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Do not export" ) );

            // Change the text color to blue (to highlight
            // that this layer is *not* being exported)
            m_layersList[ii]->SetForegroundColour( *wxBLUE );
        }
        else
        {
            m_layersList[ii]->SetLabel( GetPCBDefaultLayerName( jj ) );

            // Change the text color to fuchsia (to highlight
            // that this layer *is* being exported)
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
    }
}


void LAYERS_MAP_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void LAYERS_MAP_DIALOG::OnOkClick( wxCommandEvent& event )
{
    /* Make some test about copper layers:
     * Board must have enough copper layers to handle selected internal layers
     */
    normalizeBrdLayersCount();

    int inner_layer_max = 0;
    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
            if( m_layersLookUpTable[ii] < F_Cu )
            {
                if( m_layersLookUpTable[ii ] > inner_layer_max )
                    inner_layer_max = m_layersLookUpTable[ii];
            }
    }

    // inner_layer_max must be less than  (or equal to) the number of
    // internal copper layers
    // internal copper layers = m_exportBoardCopperLayersCount-2
    if( inner_layer_max > m_exportBoardCopperLayersCount-2 )
    {
        wxMessageBox(
        _("The exported board has not enough copper layers to handle selected inner layers") );
        return;
    }
    EndModal( wxID_OK );
}
