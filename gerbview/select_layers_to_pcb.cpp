/**
 * @file select_layers_to_pcb.cpp
 * @brief Dialog to choose equivalence between gerber layers and pcb layers
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerbview_settings.h>
#include <kiface_i.h>
#include <layers_id_colors_and_visibility.h>

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

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        // Specify the default value for each member of these arrays.
        m_buttonTable[ii]       = -1;
        m_layersLookUpTable[ii] = UNSELECTED_LAYER;
    }

    // Ensure we have:
    //    At least 2 copper layers and less than max pcb copper layers count
    //    Even number of layers because a board *must* have even layers count
    normalizeBrdLayersCount();

    int idx = ( m_exportBoardCopperLayersCount / 2 ) - 1;
    m_comboCopperLayersCount->SetSelection( idx );

    m_gerberActiveLayersCount      = 0;
    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    for( unsigned ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        if( images->GetGbrImage( ii ) == NULL )
            break;

        m_buttonTable[m_gerberActiveLayersCount] = ii;
        m_gerberActiveLayersCount++;
    }

    if( m_gerberActiveLayersCount <= GERBER_DRAWLAYERS_COUNT / 2 ) // Only one list is enough
        m_staticlineSep->Hide();

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

        if( ii == GERBER_DRAWLAYERS_COUNT / 2 )
            flexColumnBoxSizer = m_flexRightColumnBoxSizer;

        // Provide a text string to identify the Gerber layer
        msg.Printf( _( "Layer %d" ), m_buttonTable[ii] + 1 );

        label = new wxStaticText( this, wxID_STATIC, msg );
        flexColumnBoxSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        /* Add file name and extension without path.  */
        wxFileName fn( images->GetGbrImage( ii )->m_FileName );
        label = new wxStaticText( this, wxID_STATIC, fn.GetFullName() );
        flexColumnBoxSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        // Provide a button for this layer (which will invoke a child dialog box)
        item_ID          = ID_BUTTON_0 + ii;
        wxButton * Button = new wxButton( this, item_ID, wxT( "..." ), wxDefaultPosition,
                                          wxDefaultSize, wxBU_EXACTFIT );

        flexColumnBoxSizer->Add( Button, 0, wxALIGN_CENTER_VERTICAL | wxALL );

        // Provide another text string to specify which Pcbnew layer that this
        // Gerber layer is mapped to.  All layers initially default to
        // "Do NotExport" (which corresponds to UNSELECTED_LAYER).  Whenever
        // a layer is set to "Do Not Export" it's displayed in blue.  When a
        // user selects a specific KiCad layer to map to, it's displayed in
        // magenta which indicates it will be exported.
        item_ID = ID_TEXT_0 + ii;

        // All layers default to "Do Not Export" displayed in blue
        msg  = _( "Do not export" );
        text = new wxStaticText( this, item_ID, msg );
        text->SetForegroundColour( *wxBLUE );

        // When the first of these text strings is being added, determine what
        // size is necessary to to be able to display any possible string
        // without it being truncated. Then specify that size as the minimum
        // size for all of these text strings. (If this minimum size is not
        // determined in this fashion, then it is possible for the display of
        // one or more of these strings to be truncated after different Pcbnew
        // layers are selected.)

        if( ii == 0 )
        {
            goodSize = text->GetSize();

            for( LAYER_NUM jj = 0; jj < GERBER_DRAWLAYERS_COUNT; ++jj )
            {
                text->SetLabel( GetPCBDefaultLayerName( jj ) );

                if( goodSize.x < text->GetSize().x )
                    goodSize.x = text->GetSize().x;
            }
            text->SetLabel( msg ); // Reset label to default text
        }

        text->SetMinSize( goodSize );
        flexColumnBoxSizer->Add( text, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        m_layersList[ii] = text;
    }

    std::vector<int> gerber2KicadMapping;

    // See how many of the loaded Gerbers have Altium file extensions
    int numAltiumGerbers = findNumAltiumGerbersLoaded( gerber2KicadMapping );

    if( numAltiumGerbers > 0 )
    {
        // See if the user wants to map the Altium Gerbers to known KiCad PCB layers
        int returnVal = wxMessageBox(
                _( "Gerbers with known layers: " + wxString::Format( wxT( "%i" ), numAltiumGerbers )
                        + "\n\nAssign to matching KiCad PCB layers?" ),
                _( "Automatic Layer Assignment" ), wxOK | wxCANCEL | wxOK_DEFAULT );

        if( returnVal == wxOK )
        {
            for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
            {
                int currLayer = gerber2KicadMapping[ii];

                // Default to "Do Not Export" for unselected or undefined layer
                if( ( currLayer == UNSELECTED_LAYER ) || ( currLayer == UNDEFINED_LAYER ) )
                {
                    m_layersList[ii]->SetLabel( _( "Do not export" ) );
                    m_layersList[ii]->SetForegroundColour( *wxBLUE );

                    // Set the layer internally to unselected
                    m_layersLookUpTable[ii] = UNSELECTED_LAYER;
                }
                else
                {
                    m_layersList[ii]->SetLabel( GetPCBDefaultLayerName( currLayer ) );
                    m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );

                    // Set the layer internally to the matching KiCad layer
                    m_layersLookUpTable[ii] = currLayer;
                }
            }
        }
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
    wxString  msg;
    int       ii;
    LAYER_NUM layer;
    for( ii = 0, layer = 0; ii < m_gerberActiveLayersCount; ii++, ++layer )
    {
        m_layersLookUpTable[ii] = UNSELECTED_LAYER;
        m_layersList[ii]->SetLabel( _( "Do not export" ) );
        m_layersList[ii]->SetForegroundColour( *wxBLUE );
        m_buttonTable[ii] = ii;
    }
}


/* Stores the current layers selection in config
 */
void LAYERS_MAP_DIALOG::OnStoreSetup( wxCommandEvent& event )
{
    auto config = static_cast<GERBVIEW_SETTINGS*>( Kiface().KifaceSettings() );
    config->m_BoardLayersCount = m_exportBoardCopperLayersCount;

    config->m_GerberToPcbLayerMapping.clear();

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        config->m_GerberToPcbLayerMapping.push_back( m_layersLookUpTable[ii] );
    }
}

void LAYERS_MAP_DIALOG::OnGetSetup( wxCommandEvent& event )
{
    auto config = static_cast<GERBVIEW_SETTINGS*>( Kiface().KifaceSettings() );

    m_exportBoardCopperLayersCount = config->m_BoardLayersCount;
    normalizeBrdLayersCount();

    int idx = ( m_exportBoardCopperLayersCount / 2 ) - 1;
    m_comboCopperLayersCount->SetSelection( idx );

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        m_layersLookUpTable[ii] = config->m_GerberToPcbLayerMapping[ ii ];
    }

    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        LAYER_NUM layer = m_layersLookUpTable[ii];
        if( layer == UNSELECTED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Do not export" ) );
            m_layersList[ii]->SetForegroundColour( *wxBLUE );
        }
        else if( layer == UNDEFINED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Hole data" ) );
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
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

    if( jj != UNSELECTED_LAYER && jj != UNDEFINED_LAYER && !IsValidLayer( jj ) )
        jj = B_Cu;  // (Defaults to "Copper" layer.)

    jj = m_Parent->SelectPCBLayer( jj, m_exportBoardCopperLayersCount );

    if( jj != UNSELECTED_LAYER && jj != UNDEFINED_LAYER && !IsValidLayer( jj ) )
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
        else if( jj == UNDEFINED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Hole data" ) );

            // Change the text color to fuchsia (to highlight
            // that this layer *is* being exported)
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
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
        _("Exported board does not have enough copper layers to handle selected inner layers") );
        return;
    }

    EndModal( wxID_OK );
}

int LAYERS_MAP_DIALOG::findNumAltiumGerbersLoaded( std::vector<int>& aGerber2KicadMapping )
{
    // The next comment preserves initializer formatting below it
    // clang-format off
    // This map contains the known Altium file extensions for Gerbers that we care about,
    // along with their corresponding KiCad layer
    std::map<wxString, PCB_LAYER_ID> altiumExt{
        { "GTL", F_Cu },      // Top copper
        { "G1", In1_Cu },     // Inner layers 1 - 30
        { "G2", In2_Cu },
        { "G3", In3_Cu },
        { "G4", In4_Cu },
        { "G5", In5_Cu },
        { "G6", In6_Cu },
        { "G7", In7_Cu },
        { "G8", In8_Cu },
        { "G9", In9_Cu },
        { "G10", In10_Cu },
        { "G11", In11_Cu },
        { "G12", In12_Cu },
        { "G13", In13_Cu },
        { "G14", In14_Cu },
        { "G15", In15_Cu },
        { "G16", In16_Cu },
        { "G17", In17_Cu },
        { "G18", In18_Cu },
        { "G19", In19_Cu },
        { "G20", In20_Cu },
        { "G21", In21_Cu },
        { "G22", In22_Cu },
        { "G23", In23_Cu },
        { "G24", In24_Cu },
        { "G25", In25_Cu },
        { "G26", In26_Cu },
        { "G27", In27_Cu },
        { "G28", In28_Cu },
        { "G29", In29_Cu },
        { "G30", In30_Cu },
        { "GBL", B_Cu },      // Bottom copper
        { "GTP", F_Paste },   // Paste top
        { "GBP", B_Paste },   // Paste bottom
        { "GTO", F_SilkS },   // Silkscreen top
        { "GBO", B_SilkS },   // Silkscreen bottom
        { "GTS", F_Mask },    // Soldermask top
        { "GBS", B_Mask },    // Soldermask bottom
        { "GM1", Eco1_User }, // Altium mechanical layer 1
        { "GM2", Eco2_User }, // Altium mechanical layer 2
        { "GKO", Edge_Cuts }  // PCB Outline
    };
    // clang-format on

    std::map<wxString, PCB_LAYER_ID>::iterator it;

    int numAltiumMatches = 0; // Assume we won't find Altium Gerbers

    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    // Loop through all loaded Gerbers looking for any with Altium specific extensions
    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        // Get file name of Gerber loaded on this layer.
        wxFileName fn( images->GetGbrImage( ii )->m_FileName );

        // Get uppercase version of file extension
        wxString FileExt = fn.GetExt();
        FileExt.MakeUpper();

        // Check for matching Altium Gerber file extension we'll handle
        it = altiumExt.find( FileExt );

        if( it != altiumExt.end() )
        {
            // We got a match, so store the KiCad layer number
            aGerber2KicadMapping.push_back( it->second );
            numAltiumMatches++;
        }
        else
        {
            // If there's no Altium match, then note the layer as unselected
            aGerber2KicadMapping.push_back( UNSELECTED_LAYER );
        }
    }

    // Return number of Altium Gerbers we found.  Each index in the passed vector corresponds to
    // a loaded Gerber layer, and the entry will contain the index to the matching
    // KiCad layer for Altium Gerbers, or "UNSELECTED_LAYER" for the rest.
    return numAltiumMatches;
}
