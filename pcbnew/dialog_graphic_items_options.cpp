	/*************************************************/
	/* dialog_graphic_items_options.cpp - Gestion des Options et Reglages */
	/*************************************************/
/*
 Affichage et modifications des parametres de travail de PcbNew

*/


#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "autorout.h"

#include "id.h"

#include "protos.h"

#include "dialog_graphic_items_options.h"

#include <wx/spinctrl.h>


/**************************************************************/
void WinEDA_GraphicItemsOptionsDialog::SetDisplayValue( void )
/**************************************************************/
{
	/* Drawings width */
	AddUnitSymbol(*m_GraphicSegmWidthTitle);
	PutValueInLocalUnits(*m_OptPcbSegmWidth,
					g_DesignSettings.m_DrawSegmentWidth, PCB_INTERNAL_UNIT );
	/* Edges width */
	AddUnitSymbol(*m_BoardEdgesWidthTitle);
	PutValueInLocalUnits(*m_OptPcbEdgesWidth,
					g_DesignSettings.m_EdgeSegmentWidth, PCB_INTERNAL_UNIT );

	/* Pcb Textes (Size & Width) */
	AddUnitSymbol(*m_CopperTextWidthTitle);
	PutValueInLocalUnits(*m_OptPcbTextWidth,
					g_DesignSettings.m_PcbTextWidth, PCB_INTERNAL_UNIT );

	AddUnitSymbol(*m_TextSizeVTitle);
	PutValueInLocalUnits(*m_OptPcbTextVSize,
					g_DesignSettings.m_PcbTextSize.y, PCB_INTERNAL_UNIT );

	AddUnitSymbol(*m_TextSizeHTitle);
	PutValueInLocalUnits(*m_OptPcbTextHSize,
					g_DesignSettings.m_PcbTextSize.x, PCB_INTERNAL_UNIT );


	/* Modules: Edges width */
	AddUnitSymbol(*m_EdgeModWidthTitle);
	PutValueInLocalUnits(*m_OptModuleEdgesWidth,
					ModuleSegmentWidth, PCB_INTERNAL_UNIT );

	/* Modules: Texts: Size & width */
	AddUnitSymbol(*m_TextModWidthTitle);
	PutValueInLocalUnits(*m_OptModuleTextWidth,
					ModuleTextWidth, PCB_INTERNAL_UNIT );

	AddUnitSymbol(*m_TextModSizeVTitle);
	PutValueInLocalUnits(*m_OptModuleTextVSize,
					ModuleTextSize.y, PCB_INTERNAL_UNIT );

	AddUnitSymbol(*m_TextModSizeHTitle);
	PutValueInLocalUnits(*m_OptModuleTextHSize,
					ModuleTextSize.x, PCB_INTERNAL_UNIT );

}


/*********************************************************************/
void WinEDA_GraphicItemsOptionsDialog::AcceptOptions(wxCommandEvent& event)
/*********************************************************************/
{
	g_DesignSettings.m_DrawSegmentWidth =
		ReturnValueFromTextCtrl( *m_OptPcbSegmWidth, PCB_INTERNAL_UNIT );
	g_DesignSettings.m_EdgeSegmentWidth =
		ReturnValueFromTextCtrl( *m_OptPcbEdgesWidth, PCB_INTERNAL_UNIT );
	g_DesignSettings.m_PcbTextWidth =
		ReturnValueFromTextCtrl( *m_OptPcbTextWidth, PCB_INTERNAL_UNIT );
	g_DesignSettings.m_PcbTextSize.y =
		ReturnValueFromTextCtrl( *m_OptPcbTextVSize, PCB_INTERNAL_UNIT );
	g_DesignSettings.m_PcbTextSize.x =
		ReturnValueFromTextCtrl( *m_OptPcbTextHSize, PCB_INTERNAL_UNIT );

	ModuleSegmentWidth =
		ReturnValueFromTextCtrl( *m_OptModuleEdgesWidth, PCB_INTERNAL_UNIT );
	ModuleTextWidth =
		ReturnValueFromTextCtrl( *m_OptModuleTextWidth, PCB_INTERNAL_UNIT );
	ModuleTextSize.y =
		ReturnValueFromTextCtrl( *m_OptModuleTextVSize, PCB_INTERNAL_UNIT );
	ModuleTextSize.x =
		ReturnValueFromTextCtrl( *m_OptModuleTextHSize, PCB_INTERNAL_UNIT );

	EndModal(1);
}


/*!
 * WinEDA_GraphicItemsOptionsDialog type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_GraphicItemsOptionsDialog, wxDialog )

/*!
 * WinEDA_GraphicItemsOptionsDialog event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_GraphicItemsOptionsDialog, wxDialog )

////@begin WinEDA_GraphicItemsOptionsDialog event table entries
    EVT_BUTTON( wxID_OK, WinEDA_GraphicItemsOptionsDialog::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, WinEDA_GraphicItemsOptionsDialog::OnCancelClick )

////@end WinEDA_GraphicItemsOptionsDialog event table entries

END_EVENT_TABLE()

/*!
 * WinEDA_GraphicItemsOptionsDialog constructors
 */

WinEDA_GraphicItemsOptionsDialog::WinEDA_GraphicItemsOptionsDialog( )
{
}

WinEDA_GraphicItemsOptionsDialog::WinEDA_GraphicItemsOptionsDialog( WinEDA_BasePcbFrame* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WinEDA_GraphicItemsOptionsDialog creator
 */

bool WinEDA_GraphicItemsOptionsDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WinEDA_GraphicItemsOptionsDialog member initialisation
    m_GraphicSegmWidthTitle = NULL;
    m_OptPcbSegmWidth = NULL;
    m_BoardEdgesWidthTitle = NULL;
    m_OptPcbEdgesWidth = NULL;
    m_CopperTextWidthTitle = NULL;
    m_OptPcbTextWidth = NULL;
    m_TextSizeVTitle = NULL;
    m_OptPcbTextVSize = NULL;
    m_TextSizeHTitle = NULL;
    m_OptPcbTextHSize = NULL;
    m_EdgeModWidthTitle = NULL;
    m_OptModuleEdgesWidth = NULL;
    m_TextModWidthTitle = NULL;
    m_OptModuleTextWidth = NULL;
    m_TextModSizeVTitle = NULL;
    m_OptModuleTextVSize = NULL;
    m_TextModSizeHTitle = NULL;
    m_OptModuleTextHSize = NULL;
////@end WinEDA_GraphicItemsOptionsDialog member initialisation

////@begin WinEDA_GraphicItemsOptionsDialog creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WinEDA_GraphicItemsOptionsDialog creation
    return true;
}

/*!
 * Control creation for WinEDA_GraphicItemsOptionsDialog
 */

void WinEDA_GraphicItemsOptionsDialog::CreateControls()
{    
	SetFont(*g_DialogFont);
    
////@begin WinEDA_GraphicItemsOptionsDialog content construction
    // Generated by DialogBlocks, 25/02/2006 10:43:53 (unregistered)

    WinEDA_GraphicItemsOptionsDialog* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Graphics:"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxALL, 5);

    m_GraphicSegmWidthTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Graphic segm Width"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_GraphicSegmWidthTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptPcbSegmWidth = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_SEGW, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_OptPcbSegmWidth, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_BoardEdgesWidthTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Board Edges Width"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_BoardEdgesWidthTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptPcbEdgesWidth = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_EDGES, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_OptPcbEdgesWidth, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_CopperTextWidthTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Copper Text Width"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_CopperTextWidthTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptPcbTextWidth = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_TEXTW, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_OptPcbTextWidth, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_TextSizeVTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Text Size V"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_TextSizeVTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptPcbTextVSize = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_TEXTV, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_OptPcbTextVSize, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_TextSizeHTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Text Size H"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_TextSizeHTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptPcbTextHSize = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_TEXTH, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer3->Add(m_OptPcbTextHSize, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer2->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer15Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Modules:"));
    wxStaticBoxSizer* itemStaticBoxSizer15 = new wxStaticBoxSizer(itemStaticBoxSizer15Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer15, 0, wxGROW|wxALL, 5);

    m_EdgeModWidthTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Edges Module Width"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_EdgeModWidthTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptModuleEdgesWidth = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_EDGEMOD_W, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_OptModuleEdgesWidth, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_TextModWidthTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Text Module Width"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_TextModWidthTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptModuleTextWidth = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_TXTMOD_W, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_OptModuleTextWidth, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_TextModSizeVTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Text Module Size V"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_TextModSizeVTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptModuleTextVSize = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_TXTMOD_V, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_OptModuleTextVSize, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_TextModSizeHTitle = new wxStaticText( itemDialog1, wxID_STATIC, _("Text Module Size H"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_TextModSizeHTitle, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_OptModuleTextHSize = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_TXTMOD_H, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(m_OptModuleTextHSize, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer2->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton26 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton26->SetForegroundColour(wxColour(206, 0, 0));
    itemBoxSizer25->Add(itemButton26, 0, wxGROW|wxALL, 5);

    wxButton* itemButton27 = new wxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton27->SetForegroundColour(wxColour(0, 0, 255));
    itemBoxSizer25->Add(itemButton27, 0, wxGROW|wxALL, 5);

////@end WinEDA_GraphicItemsOptionsDialog content construction
	SetDisplayValue();
}

/*!
 * Should we show tooltips?
 */

bool WinEDA_GraphicItemsOptionsDialog::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_GraphicItemsOptionsDialog::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WinEDA_GraphicItemsOptionsDialog bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WinEDA_GraphicItemsOptionsDialog bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WinEDA_GraphicItemsOptionsDialog::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_GraphicItemsOptionsDialog icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WinEDA_GraphicItemsOptionsDialog icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void WinEDA_GraphicItemsOptionsDialog::OnOkClick( wxCommandEvent& event )
{
	AcceptOptions(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void WinEDA_GraphicItemsOptionsDialog::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in WinEDA_GraphicItemsOptionsDialog.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in WinEDA_GraphicItemsOptionsDialog. 
}


