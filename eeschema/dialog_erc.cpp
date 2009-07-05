/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_erc.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by:
// Created:     02/07/2000
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////
#include "fctsys.h"

#include "common.h"
#include "class_drawpanel.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"
#include "bitmaps.h"

#include "protos.h"

#include "dialog_erc.h"


BEGIN_EVENT_TABLE( DIALOG_ERC, DIALOG_ERC_BASE )
	EVT_COMMAND_RANGE(ID_MATRIX_0,
					ID_MATRIX_0 + (PIN_NMAX * PIN_NMAX) - 1,
					wxEVT_COMMAND_BUTTON_CLICKED,
					DIALOG_ERC::ChangeErrorLevel)
END_EVENT_TABLE()


DIALOG_ERC::DIALOG_ERC( WinEDA_SchematicFrame* parent )
    : DIALOG_ERC_BASE(parent)
{
	m_Parent = parent;
	Init();

	GetSizer()->SetSizeHints(this);

}

void DIALOG_ERC::Init()
{
    SetFocus();

	m_Initialized = FALSE;
	for( int ii = 0; ii < PIN_NMAX; ii++ )
		for( int jj = 0; jj < PIN_NMAX; jj++ )
			m_ButtonList[ii][jj] = NULL;

    m_WriteResultOpt->SetValue( WriteFichierERC );

	wxString num;
	num.Printf(wxT("%d"), g_EESchemaVar.NbErrorErc);
	m_TotalErrCount->SetLabel(num);

	num.Printf(wxT("%d"), g_EESchemaVar.NbErrorErc-g_EESchemaVar.NbWarningErc);
	m_LastErrCount->SetLabel(num);

	num.Printf(wxT("%d"), g_EESchemaVar.NbWarningErc);
	m_LastWarningCount->SetLabel(num);

    DisplayERC_MarkersList( );

	// Init Panel Matrix
	ReBuildMatrixPanel();
}

/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERASE_DRC_MARKERS */
void DIALOG_ERC::OnEraseDrcMarkersClick( wxCommandEvent& event )
/* Delete the old ERC markers, over the whole hierarchy
 */
{
    DeleteAllMarkers( MARQ_ERC );
    m_MessagesList->Clear();
    m_Parent->DrawPanel->Refresh();
}

/* wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL */
void DIALOG_ERC::OnCancelClick( wxCommandEvent& event )
{
    EndModal(0);
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESET_MATRIX */
void DIALOG_ERC::OnResetMatrixClick( wxCommandEvent& event )
{
	ResetDefaultERCDiag(event);
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERC_CMP */
void DIALOG_ERC::OnErcCmpClick( wxCommandEvent& event )
{
    m_MessagesList->Clear();
    wxSafeYield();      // m_MessagesList must be redraw
	TestErc( m_MessagesList);
}


/*********************************************/
void DIALOG_ERC::ReBuildMatrixPanel()
/*********************************************/

/* Build or rebuild the panel showing the ERC confict matrix
 */
{
    int           ii, jj, event_id, text_height;
    wxPoint       pos, BoxMatrixPosition;

#define BITMAP_SIZE 19
    int           bitmap_size = BITMAP_SIZE;
    wxStaticText* text;
    int           x, y;
    wxSize        BoxMatrixMinSize;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = TRUE;
    }

    // Get the current text size :
    text = new wxStaticText( m_PanelERCOptions, -1, wxT( "W" ), pos );    // this is a dummy text

    text_height = text->GetRect().GetHeight();
    bitmap_size = MAX( bitmap_size, text_height );
    SAFE_DELETE( text );

    // compute the Y pos interval:
    BoxMatrixMinSize.y = ( bitmap_size * (PIN_NMAX + 1) ) + 5;
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    pos = m_MatrixSizer->GetPosition();

    // Size computation is not made in constructor, in some wxWidgets version,
    // and m_BoxSizerForERC_Opt position is always 0,0. and we can't use it
    pos.x = MAX( pos.x, 5 );
    pos.y = MAX( pos.y, m_ResetOptButton->GetRect().GetHeight() + 30 );

    BoxMatrixPosition = pos;

    pos.y += text_height;

    if( m_Initialized == FALSE )
    {
        for( ii = 0; ii < PIN_NMAX; ii++ )
        {
            y    = pos.y + (ii * bitmap_size);
            text = new wxStaticText( m_PanelERCOptions, -1, CommentERC_H[ii], wxPoint( 5, y ) );

            x     = text->GetRect().GetRight();
            pos.x = MAX( pos.x, x );
        }

        pos.x += 5;
    }
    else
        pos = m_ButtonList[0][0]->GetPosition();

    for( ii = 0; ii < PIN_NMAX; ii++ )
    {
        y = pos.y + (ii * bitmap_size);
        for( jj = 0; jj <= ii; jj++ )
        {
            int diag = DiagErc[ii][jj];
            x = pos.x + (jj * bitmap_size);
            if( (ii == jj) && !m_Initialized )
            {
                wxPoint txtpos;
                txtpos.x = x + 6; txtpos.y = y - bitmap_size;
                text     = new wxStaticText( m_PanelERCOptions, -1, CommentERC_V[ii], txtpos );

                BoxMatrixMinSize.x = MAX( BoxMatrixMinSize.x, text->GetRect().GetRight() );
            }
            event_id = ID_MATRIX_0 + ii + (jj * PIN_NMAX);
            delete m_ButtonList[ii][jj];

            switch( diag )
            {
            case OK:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                          event_id,
                                                          wxBitmap( erc_green_xpm ),
                                                          wxPoint( x, y ) );

                break;

            case WAR:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                          event_id,
                                                          wxBitmap( warning_xpm ),
                                                          wxPoint( x, y ) );

                break;

            case ERR:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                          event_id,
                                                          wxBitmap( error_xpm ),
                                                          wxPoint( x, y ) );

                break;
            }
        }
    }

    if( !m_Initialized )
    {
        BoxMatrixMinSize.x += 5;
        m_MatrixSizer->SetMinSize( BoxMatrixMinSize );
        BoxMatrixMinSize.y += BoxMatrixPosition.y;
        m_PanelMatrixSizer->SetMinSize( BoxMatrixMinSize );
    }
    m_Initialized = TRUE;
}

