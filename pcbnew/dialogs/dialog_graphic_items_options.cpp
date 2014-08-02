/************************************/
/* dialog_graphic_items_options.cpp */
/************************************/


#include <fctsys.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <base_units.h>

#include <pcbnew_id.h>
#include <module_editor_frame.h>
#include <class_board.h>

#include <dialog_graphic_items_options.h>


void PCB_EDIT_FRAME::OnConfigurePcbOptions( wxCommandEvent& aEvent )
{
    DIALOG_GRAPHIC_ITEMS_OPTIONS dlg( this );

    dlg.ShowModal();
}


void FOOTPRINT_EDIT_FRAME::InstallOptionsFrame( const wxPoint& pos )
{
    DIALOG_GRAPHIC_ITEMS_OPTIONS dlg( this );
    dlg.ShowModal();
}


/*
 * DIALOG_GRAPHIC_ITEMS_OPTIONS constructor
 */

DIALOG_GRAPHIC_ITEMS_OPTIONS::DIALOG_GRAPHIC_ITEMS_OPTIONS( PCB_BASE_FRAME* parent )
    : DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE( parent )
{
    m_parent = parent;
    m_brdSettings = m_parent->GetDesignSettings();
    initValues(  );

    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );

    Centre();
}

DIALOG_GRAPHIC_ITEMS_OPTIONS::~DIALOG_GRAPHIC_ITEMS_OPTIONS()
{
}


void DIALOG_GRAPHIC_ITEMS_OPTIONS::initValues()
{
    SetFocus();

    /* Drawings width */
    AddUnitSymbol( *m_GraphicSegmWidthTitle );
    PutValueInLocalUnits( *m_OptPcbSegmWidth, m_brdSettings.m_DrawSegmentWidth );

    /* Edges width */
    AddUnitSymbol( *m_BoardEdgesWidthTitle );
    PutValueInLocalUnits( *m_OptPcbEdgesWidth, m_brdSettings.m_EdgeSegmentWidth );

    /* Pcb Textes (Size & Width) */
    AddUnitSymbol( *m_CopperTextWidthTitle );
    PutValueInLocalUnits( *m_OptPcbTextWidth, m_brdSettings.m_PcbTextWidth );

    AddUnitSymbol( *m_TextSizeVTitle );
    PutValueInLocalUnits( *m_OptPcbTextVSize, m_brdSettings.m_PcbTextSize.y );

    AddUnitSymbol( *m_TextSizeHTitle );
    PutValueInLocalUnits( *m_OptPcbTextHSize, m_brdSettings.m_PcbTextSize.x );


    /* Modules: Edges width */
    AddUnitSymbol( *m_EdgeModWidthTitle );
    PutValueInLocalUnits( *m_OptModuleEdgesWidth, m_brdSettings.m_ModuleSegmentWidth );

    /* Modules: Texts: Size & width */
    AddUnitSymbol( *m_TextModWidthTitle );
    PutValueInLocalUnits( *m_OptModuleTextWidth, m_brdSettings.m_ModuleTextWidth );

    AddUnitSymbol( *m_TextModSizeVTitle );
    PutValueInLocalUnits( *m_OptModuleTextVSize, m_brdSettings.m_ModuleTextSize.y );

    AddUnitSymbol( *m_TextModSizeHTitle );
    PutValueInLocalUnits( *m_OptModuleTextHSize, m_brdSettings.m_ModuleTextSize.x );

    AddUnitSymbol( *m_DefaultPenSizeTitle );
    PutValueInLocalUnits( *m_DefaultPenSizeCtrl, g_DrawDefaultLineThickness );
}


void DIALOG_GRAPHIC_ITEMS_OPTIONS::OnOkClick( wxCommandEvent& event )
{
    m_brdSettings.m_DrawSegmentWidth = ValueFromTextCtrl( *m_OptPcbSegmWidth );
    m_brdSettings.m_EdgeSegmentWidth = ValueFromTextCtrl( *m_OptPcbEdgesWidth );
    m_brdSettings.m_PcbTextWidth = ValueFromTextCtrl( *m_OptPcbTextWidth );
    m_brdSettings.m_PcbTextSize.y = ValueFromTextCtrl( *m_OptPcbTextVSize );
    m_brdSettings.m_PcbTextSize.x = ValueFromTextCtrl( *m_OptPcbTextHSize );

    m_parent->GetBoard()->SetDesignSettings( m_brdSettings );

    m_brdSettings.m_ModuleSegmentWidth = ValueFromTextCtrl( *m_OptModuleEdgesWidth );
    m_brdSettings.m_ModuleTextWidth = ValueFromTextCtrl( *m_OptModuleTextWidth );
    m_brdSettings.m_ModuleTextSize.y = ValueFromTextCtrl( *m_OptModuleTextVSize );
    m_brdSettings.m_ModuleTextSize.x = ValueFromTextCtrl( *m_OptModuleTextHSize );

    g_DrawDefaultLineThickness = ValueFromTextCtrl( *m_DefaultPenSizeCtrl );

    if( g_DrawDefaultLineThickness < 0 )
        g_DrawDefaultLineThickness = 0;

    m_parent->SetDesignSettings( m_brdSettings );

    EndModal( wxID_OK );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_GRAPHIC_ITEMS_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}
