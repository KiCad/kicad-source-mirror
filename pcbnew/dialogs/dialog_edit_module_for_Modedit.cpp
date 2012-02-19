/*******************************************************************************************/
/* Dialog box for editing module	properties and carateristics in module editor (modedit)*/
/*******************************************************************************************/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <appl_wxstruct.h>
#include <gestfich.h>
#include <3d_struct.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>

#include <class_module.h>
#include <class_text_mod.h>
#include <module_editor_frame.h>
#include <dialog_edit_module_for_Modedit.h>


DIALOG_MODULE_MODULE_EDITOR::DIALOG_MODULE_MODULE_EDITOR( FOOTPRINT_EDIT_FRAME* aParent,
                                                          MODULE* aModule ) :
    DIALOG_MODULE_MODULE_EDITOR_BASE( aParent )
{
    m_Parent = aParent;
    m_CurrentModule = aModule;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    InitModeditProperties();
    m_sdbSizerStdButtonsOK->SetDefault();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_MODULE_MODULE_EDITOR::~DIALOG_MODULE_MODULE_EDITOR()
{
    for( unsigned ii = 0; ii < m_Shapes3D_list.size(); ii++ )
        delete m_Shapes3D_list[ii];

    m_Shapes3D_list.clear();

    delete m_ReferenceCopy;
    delete m_ValueCopy;
    delete m_3D_Scale;
    delete m_3D_Offset;
    delete m_3D_Rotation;
}


/********************************************************/
void DIALOG_MODULE_MODULE_EDITOR::InitModeditProperties()
/********************************************************/
{
    SetFocus();

    m_LastSelected3DShapeIndex = -1;

    // Init 3D shape list
    S3D_MASTER* draw3D = m_CurrentModule->m_3D_Drawings;

    while( draw3D )
    {
        if( !draw3D->m_Shape3DName.IsEmpty() )
        {
            S3D_MASTER* draw3DCopy = new S3D_MASTER(NULL);
            draw3DCopy->Copy( draw3D );
            m_Shapes3D_list.push_back( draw3DCopy );
            m_3D_ShapeNameListBox->Append(draw3DCopy->m_Shape3DName);
        }
        draw3D = (S3D_MASTER*) draw3D->Next();
    }

    m_DocCtrl->SetValue( m_CurrentModule->m_Doc );
    m_KeywordCtrl->SetValue( m_CurrentModule->m_KeyWord);
    m_ReferenceCopy = new TEXTE_MODULE(NULL);
    m_ValueCopy = new TEXTE_MODULE(NULL);
    m_ReferenceCopy->Copy(m_CurrentModule->m_Reference);
    m_ValueCopy->Copy(m_CurrentModule->m_Value);
    m_ReferenceCtrl->SetValue( m_ReferenceCopy->m_Text );
    m_ValueCtrl->SetValue( m_ValueCopy->m_Text );
    m_ValueCtrl->SetValue( m_ValueCopy->m_Text );
    m_FootprintNameCtrl->SetValue( m_CurrentModule->m_LibRef );

#if wxCHECK_VERSION( 2, 8, 0 )
    m_AttributsCtrl->SetItemToolTip( 0, _( "Use this attribute for most non smd components" ) );
    m_AttributsCtrl->SetItemToolTip( 1,
                                    _(
                                        "Use this attribute for smd components.\nOnly components with this option are put in the footprint position list file" ) );
    m_AttributsCtrl->SetItemToolTip( 2,
                                    _(
                                        "Use this attribute for \"virtual\" components drawn on board (like a old ISA PC bus connector)" ) );
#endif

    // Controls on right side of the dialog
    switch( m_CurrentModule->m_Attributs & 255 )
    {
    case 0:
        m_AttributsCtrl->SetSelection( 0 );
        break;

    case MOD_CMS:
        m_AttributsCtrl->SetSelection( 1 );
        break;

    case MOD_VIRTUAL:
        m_AttributsCtrl->SetSelection( 2 );
        break;

    default:
        m_AttributsCtrl->SetSelection( 0 );
        break;
    }

    m_AutoPlaceCtrl->SetSelection(
        (m_CurrentModule->m_ModuleStatus & MODULE_is_LOCKED) ? 1 : 0 );
#if wxCHECK_VERSION( 2, 8, 0 )
    m_AutoPlaceCtrl->SetItemToolTip( 0, _( "Enable hotkey move commands and Auto Placement" ) );
    m_AutoPlaceCtrl->SetItemToolTip( 1, _( "Disable hotkey move commands and Auto Placement" ) );
#endif
    m_CostRot90Ctrl->SetValue( m_CurrentModule->m_CntRot90 );

    m_CostRot180Ctrl->SetValue( m_CurrentModule->m_CntRot180 );

    // Initialize 3D parameters

    wxBoxSizer* BoxSizer = new wxBoxSizer( wxVERTICAL );
    m_3D_Scale = new WinEDA_VertexCtrl( m_Panel3D, _( "Shape Scale:" ), BoxSizer, UNSCALED_UNITS, 1 );
    m_Sizer3DValues->Add( BoxSizer, 0, wxGROW | wxALL, 5 );

    BoxSizer    = new wxBoxSizer( wxVERTICAL );
    m_3D_Offset = new WinEDA_VertexCtrl( m_Panel3D, _( "Shape Offset:" ), BoxSizer, UNSCALED_UNITS, 1 );
    m_Sizer3DValues->Add( BoxSizer, 0, wxGROW | wxALL, 5 );

    BoxSizer = new wxBoxSizer( wxVERTICAL );
    m_3D_Rotation = new WinEDA_VertexCtrl( m_Panel3D, _( "Shape Rotation:" ), BoxSizer, UNSCALED_UNITS, 1 );
    m_Sizer3DValues->Add( BoxSizer, 0, wxGROW | wxALL, 5 );

    // Initialize dialog relative to masks clearances
    m_NetClearanceUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    wxString  msg;
    int internalUnit = m_Parent->GetInternalUnits();
    PutValueInLocalUnits( *m_NetClearanceValueCtrl,
                          m_CurrentModule->m_LocalClearance, internalUnit );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl,
                          m_CurrentModule->m_LocalSolderMaskMargin, internalUnit );
    // These 2 parameters are usually < 0, so prepare entering a negative value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl,
                          m_CurrentModule->GetLocalSolderPasteMargin(), internalUnit );
    if( m_CurrentModule->GetLocalSolderPasteMargin() == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT("-") + m_SolderPasteMarginCtrl->GetValue() );
    if( m_CurrentModule->GetLocalSolderPasteMarginRatio() == 0.0 )
        msg.Printf( wxT( "-%.1f" ), m_CurrentModule->GetLocalSolderPasteMarginRatio() * 100.0 );
    else
        msg.Printf( wxT( "%.1f" ), m_CurrentModule->GetLocalSolderPasteMarginRatio() * 100.0 );
    m_SolderPasteMarginRatioCtrl->SetValue( msg );

    // if m_3D_ShapeNameListBox is not empty, preselect first 3D shape
    if( m_3D_ShapeNameListBox->GetCount() > 0 )
    {
        m_LastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );
        Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
    }
}


/* Initialize 3D info displayed in dialog box from values in aStruct3DSource
 */
void DIALOG_MODULE_MODULE_EDITOR::Transfert3DValuesToDisplay( S3D_MASTER * aStruct3DSource )
{
    if( aStruct3DSource )
    {
        m_3D_Scale->SetValue( aStruct3DSource->m_MatScale );

        m_3D_Offset->SetValue( aStruct3DSource->m_MatPosition );

        m_3D_Rotation->SetValue( aStruct3DSource->m_MatRotation );
    }
    else
    {
        S3D_Vertex dummy_vertex;
        dummy_vertex.x = dummy_vertex.y = dummy_vertex.z = 1.0;
        m_3D_Scale->SetValue( dummy_vertex );
    }
}

/** Copy 3D info displayed in dialog box to values in a item in m_Shapes3D_list
 * @param aIndexSelection = item index in m_Shapes3D_list
 */
void DIALOG_MODULE_MODULE_EDITOR::TransfertDisplayTo3DValues( int aIndexSelection  )
{
    if( aIndexSelection >= (int)m_Shapes3D_list.size() )
        return;

    S3D_MASTER * struct3DDest = m_Shapes3D_list[aIndexSelection];
    struct3DDest->m_MatScale    = m_3D_Scale->GetValue();
    struct3DDest->m_MatRotation = m_3D_Rotation->GetValue();
    struct3DDest->m_MatPosition = m_3D_Offset->GetValue();
}

/***********************************************************/
void DIALOG_MODULE_MODULE_EDITOR::On3DShapeNameSelected(wxCommandEvent& event)
/***********************************************************/
{
    if( m_LastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_LastSelected3DShapeIndex );
    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetSelection();

    if( m_LastSelected3DShapeIndex < 0 )    // happens under wxGTK when deleting an item in m_3D_ShapeNameListBox wxListBox
       return;

    if( m_LastSelected3DShapeIndex >= (int)m_Shapes3D_list.size() )
    {
        wxMessageBox(wxT("On3DShapeNameSelected() error"));
        m_LastSelected3DShapeIndex = -1;
        return;
    }
    Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
}


/***********************************************************/
void DIALOG_MODULE_MODULE_EDITOR::Remove3DShape(wxCommandEvent& event)
/***********************************************************/
{
    if( m_LastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_LastSelected3DShapeIndex );

    int ii = m_3D_ShapeNameListBox->GetSelection();
    if( ii < 0 )
        return;

    m_Shapes3D_list.erase(m_Shapes3D_list.begin() + ii );
    m_3D_ShapeNameListBox->Delete(ii);

    if( m_3D_ShapeNameListBox->GetCount() == 0)
        Transfert3DValuesToDisplay( NULL );
    else
    {
        m_LastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection(m_LastSelected3DShapeIndex);
        Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
    }
}


/*********************************************************************/
void DIALOG_MODULE_MODULE_EDITOR::BrowseAndAdd3DLib( wxCommandEvent& event )
/*********************************************************************/
{
    wxString fullfilename, shortfilename;
    wxString fullpath;
    wxString mask = wxT( "*" );

    fullpath = wxGetApp().ReturnLastVisitedLibraryPath( LIB3D_PATH );
    mask    += g_Shapes3DExtBuffer;
#ifdef __WINDOWS__
    fullpath.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
    fullfilename = EDA_FileSelector( _( "3D Shape:" ),
                                     fullpath,
                                     wxEmptyString,
                                     g_Shapes3DExtBuffer,
                                     mask,
                                     this,
                                     wxFD_OPEN,
                                     true
                                     );

    if( fullfilename == wxEmptyString )
        return;

    wxFileName fn = fullfilename;
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    /* If the file path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of these default paths
     */
    shortfilename = wxGetApp().ReturnFilenameWithRelativePathInLibPath( fullfilename );

    wxFileName aux = shortfilename;
    if( aux.IsAbsolute() )
    {   // Absolute path, ask if the user wants a relative one
        int diag = wxMessageBox(
            _( "Use a relative path?" ),
            _( "Path type" ),
            wxYES_NO | wxICON_QUESTION, this );

        if( diag == wxYES )
        {   // Make it relative
            aux.MakeRelativeTo( wxT(".") );
            shortfilename = aux.GetPathWithSep() + aux.GetFullName();
        }
    }

    S3D_MASTER* new3DShape = new S3D_MASTER(NULL);
    new3DShape->m_Shape3DName = shortfilename;
    m_Shapes3D_list.push_back( new3DShape );
    m_3D_ShapeNameListBox->Append( shortfilename );

    if( m_LastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_LastSelected3DShapeIndex );

    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetCount() - 1;
    m_3D_ShapeNameListBox->SetSelection(m_LastSelected3DShapeIndex);
    Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );

}


/**********************************************************************/
void DIALOG_MODULE_MODULE_EDITOR::OnCancelClick( wxCommandEvent& event )
/**********************************************************************/
{
    EndModal( -1 );
}


/******************************************************************************/
void DIALOG_MODULE_MODULE_EDITOR::OnOkClick( wxCommandEvent& event )
/******************************************************************************/
{
    m_Parent->SaveCopyInUndoList( m_CurrentModule, UR_MODEDIT );

    if( m_AutoPlaceCtrl->GetSelection() == 1 )
        m_CurrentModule->m_ModuleStatus |= MODULE_is_LOCKED;
    else
        m_CurrentModule->m_ModuleStatus &= ~MODULE_is_LOCKED;

    switch( m_AttributsCtrl->GetSelection() )
    {
    case 0:
        m_CurrentModule->m_Attributs = 0;
        break;

    case 1:
        m_CurrentModule->m_Attributs = MOD_CMS;
        break;

    case 2:
        m_CurrentModule->m_Attributs = MOD_VIRTUAL;
        break;
    }

    m_CurrentModule->m_CntRot90  = m_CostRot90Ctrl->GetValue();
    m_CurrentModule->m_CntRot180 = m_CostRot180Ctrl->GetValue();
    m_CurrentModule->m_Doc = m_DocCtrl->GetValue();
    m_CurrentModule->m_KeyWord = m_KeywordCtrl->GetValue();

    // Init footprint name in library
    if( ! m_FootprintNameCtrl->GetValue( ).IsEmpty() )
        m_CurrentModule->m_LibRef = m_FootprintNameCtrl->GetValue( );

    // Init Fields:
    m_CurrentModule->m_Reference->Copy(m_ReferenceCopy );
    m_CurrentModule->m_Value->Copy(m_ValueCopy );

    // Initialize masks clearances
    int internalUnit = m_Parent->GetInternalUnits();

    m_CurrentModule->SetLocalClearance(
        ReturnValueFromTextCtrl( *m_NetClearanceValueCtrl, internalUnit ) );

    m_CurrentModule->SetLocalSolderMaskMargin(
        ReturnValueFromTextCtrl( *m_SolderMaskMarginCtrl, internalUnit ) );

    m_CurrentModule->SetLocalSolderPasteMargin(
        ReturnValueFromTextCtrl( *m_SolderPasteMarginCtrl, internalUnit ) );
    double   dtmp;
    wxString msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;

    m_CurrentModule->SetLocalSolderPasteMarginRatio( dtmp / 100 );

    // Update 3D shape list
    int ii = m_3D_ShapeNameListBox->GetSelection();
    if ( ii >= 0 )
        TransfertDisplayTo3DValues( ii  );
    S3D_MASTER*   draw3D  = m_CurrentModule->m_3D_Drawings;
    for( unsigned ii = 0; ii < m_Shapes3D_list.size(); ii++ )
    {
        S3D_MASTER*   draw3DCopy = m_Shapes3D_list[ii];
        if( draw3DCopy->m_Shape3DName.IsEmpty() )
            continue;
        if( draw3D == NULL )
        {
            draw3D = new S3D_MASTER( draw3D );
            m_CurrentModule->m_3D_Drawings.Append( draw3D );
        }

        draw3D->m_Shape3DName = draw3DCopy->m_Shape3DName;
        draw3D->m_MatScale    = draw3DCopy->m_MatScale;
        draw3D->m_MatRotation = draw3DCopy->m_MatRotation;
        draw3D->m_MatPosition = draw3DCopy->m_MatPosition;

        draw3D = draw3D->Next();
    }

    // Remove old extra 3D shapes
    S3D_MASTER* nextdraw3D;
    for( ; draw3D != NULL; draw3D = nextdraw3D )
    {
        nextdraw3D = (S3D_MASTER*) draw3D->Next();
        delete m_CurrentModule->m_3D_Drawings.Remove( draw3D );
    }

    // Fill shape list with one void entry, if no entry
    if( m_CurrentModule->m_3D_Drawings == NULL )
        m_CurrentModule->m_3D_Drawings.PushBack( new S3D_MASTER( m_CurrentModule ) );


    m_CurrentModule->CalculateBoundingBox();

    m_Parent->OnModify();

    EndModal( 1 );
}


/***********************************************************************/
void DIALOG_MODULE_MODULE_EDITOR::OnEditReference(wxCommandEvent& event)
/***********************************************************************/
{
    wxPoint tmp = m_Parent->GetScreen()->GetCrossHairPosition();
    m_Parent->GetScreen()->SetCrossHairPosition( m_ReferenceCopy->m_Pos );
    m_Parent->InstallTextModOptionsFrame( m_ReferenceCopy, NULL );
    m_Parent->GetScreen()->SetCrossHairPosition( tmp );
    m_ReferenceCtrl->SetValue(m_ReferenceCopy->m_Text);
}

/***********************************************************/
void DIALOG_MODULE_MODULE_EDITOR::OnEditValue(wxCommandEvent& event)
/***********************************************************/
{
    wxPoint tmp = m_Parent->GetScreen()->GetCrossHairPosition();
    m_Parent->GetScreen()->SetCrossHairPosition( m_ValueCopy->m_Pos );
    m_Parent->InstallTextModOptionsFrame( m_ValueCopy, NULL );
    m_Parent->GetScreen()->SetCrossHairPosition( tmp);
    m_ValueCtrl->SetValue(m_ValueCopy->m_Text);
}

