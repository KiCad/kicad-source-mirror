
#include "dialog_3D_view_option_base.h"
#include <3d_viewer.h>
#include <info3d_visu.h>

class DIALOG_3D_VIEW_OPTIONS : public DIALOG_3D_VIEW_OPTIONS_BASE
{
public:
    DIALOG_3D_VIEW_OPTIONS( EDA_3D_FRAME* parent );

private:
    EDA_3D_FRAME* m_parent;
    INFO3D_VISU & m_3Dprms;

    void initDialog();

    // Event functions:
    void OnShowAllClick( wxCommandEvent& event );
    void OnShowNoneClick( wxCommandEvent& event );
    void OnOKClick( wxCommandEvent& event );
};

void EDA_3D_FRAME::Install_3D_ViewOptionDialog( wxCommandEvent& event )
{
    DIALOG_3D_VIEW_OPTIONS dlg( this );
    if( dlg.ShowModal() == wxID_OK )
    {
        SetMenuBarOptionsState();
        NewDisplay();
    }
}


DIALOG_3D_VIEW_OPTIONS::DIALOG_3D_VIEW_OPTIONS( EDA_3D_FRAME* parent )
     :DIALOG_3D_VIEW_OPTIONS_BASE( parent ), m_3Dprms( g_Parm_3D_Visu )
{
    m_parent = parent;

    initDialog();

	Layout();
    GetSizer()->SetSizeHints(this);
	Centre();
}

void DIALOG_3D_VIEW_OPTIONS::initDialog()
{
    m_bitmapCuThickness->SetBitmap( KiBitmap( use_3D_copper_thickness_xpm ) );
    m_bitmap3Dshapes->SetBitmap( KiBitmap( shape_3d_xpm ) );
    m_bitmapAreas->SetBitmap( KiBitmap( add_zone_xpm ) );
    m_bitmapSilkscreen->SetBitmap( KiBitmap( add_text_xpm ) );
    m_bitmapSolderMask->SetBitmap( KiBitmap( pads_mask_layers_xpm ) );
    m_bitmapSolderPaste->SetBitmap( KiBitmap( pads_mask_layers_xpm ) );
    m_bitmapAdhesive->SetBitmap( KiBitmap( tools_xpm ) );
    m_bitmapComments->SetBitmap( KiBitmap( edit_sheet_xpm ) );
    m_bitmapECO->SetBitmap( KiBitmap( edit_sheet_xpm ) );

    // Check/uncheck checkboxes
    m_checkBoxCuThickness->SetValue( m_3Dprms.GetFlag( FL_USE_COPPER_THICKNESS ) );
    m_checkBox3Dshapes->SetValue( m_3Dprms.GetFlag( FL_MODULE ) );
    m_checkBoxAreas->SetValue( m_3Dprms.GetFlag( FL_ZONE ) );
    m_checkBoxSilkscreen->SetValue( m_3Dprms.GetFlag( FL_SILKSCREEN ) );
    m_checkBoxSolderMask->SetValue( m_3Dprms.GetFlag( FL_SOLDERMASK ) );
    m_checkBoxSolderpaste->SetValue( m_3Dprms.GetFlag( FL_SOLDERPASTE ) );
    m_checkBoxAdhesive->SetValue( m_3Dprms.GetFlag( FL_ADHESIVE ) );
    m_checkBoxComments->SetValue( m_3Dprms.GetFlag( FL_COMMENTS ) );
    m_checkBoxECO->SetValue( m_3Dprms.GetFlag( FL_ECO ) );
}

void DIALOG_3D_VIEW_OPTIONS::OnShowAllClick( wxCommandEvent& event )
{
    bool state = true;
    m_checkBoxCuThickness->SetValue( state );
    m_checkBox3Dshapes->SetValue( state );
    m_checkBoxAreas->SetValue( state );
    m_checkBoxSilkscreen->SetValue( state );
    m_checkBoxSolderMask->SetValue( state );
    m_checkBoxSolderpaste->SetValue( state );
    m_checkBoxAdhesive->SetValue( state );
    m_checkBoxComments->SetValue( state );
    m_checkBoxECO->SetValue( state );
}

void DIALOG_3D_VIEW_OPTIONS::OnShowNoneClick( wxCommandEvent& event )
{
    bool state = false;
    m_checkBoxCuThickness->SetValue( state );
    m_checkBox3Dshapes->SetValue( state );
    m_checkBoxAreas->SetValue( state );
    m_checkBoxSilkscreen->SetValue( state );
    m_checkBoxSolderMask->SetValue( state );
    m_checkBoxSolderpaste->SetValue( state );
    m_checkBoxAdhesive->SetValue( state );
    m_checkBoxComments->SetValue( state );
    m_checkBoxECO->SetValue( state );
}

void DIALOG_3D_VIEW_OPTIONS::OnOKClick( wxCommandEvent& event )
{
    m_3Dprms.SetFlag( FL_USE_COPPER_THICKNESS,
                            m_checkBoxCuThickness->GetValue() );
    m_3Dprms.SetFlag( FL_MODULE, m_checkBox3Dshapes->GetValue() );
    m_3Dprms.SetFlag( FL_ZONE, m_checkBoxAreas->GetValue() );
    m_3Dprms.SetFlag( FL_SILKSCREEN, m_checkBoxSilkscreen->GetValue() );
    m_3Dprms.SetFlag( FL_SOLDERMASK, m_checkBoxSolderMask->GetValue() );
    m_3Dprms.SetFlag( FL_SOLDERPASTE, m_checkBoxSolderpaste->GetValue() );
    m_3Dprms.SetFlag( FL_ADHESIVE, m_checkBoxAdhesive->GetValue() );
    m_3Dprms.SetFlag( FL_COMMENTS, m_checkBoxComments->GetValue() );
    m_3Dprms.SetFlag( FL_ECO, m_checkBoxECO->GetValue( ) );

    EndModal( wxID_OK );
}
