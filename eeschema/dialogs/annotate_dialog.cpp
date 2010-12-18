/////////////////////////////////////////////////////////////////////////////
// Name:        annotate_dialog.cpp
// Licence:   License GNU
/////////////////////////////////////////////////////////////////////////////


#include "fctsys.h"
#include "appl_wxstruct.h"
#include "bitmaps.h"
#include "common.h"
#include "wxEeschemaStruct.h"

#include "annotate_dialog.h"

#define KEY_ANNOTATE_SORT_OPTION wxT("AnnotateSortOption")

extern void AnnotateComponents( SCH_EDIT_FRAME* parent,
                                bool annotateSchematic,
                                int sortOption,
                                bool resetAnnotation,
                                bool repairsTimestamps );


DIALOG_ANNOTATE::DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent )
    : DIALOG_ANNOTATE_BASE( parent )
{
    m_Parent = parent;
    InitValues();
    Layout();
    GetSizer()->SetSizeHints(this);
    Centre();
}


/*********************************/
void DIALOG_ANNOTATE::InitValues()
/*********************************/
{
    m_Config = wxGetApp().m_EDA_Config;
	SetFocus();	// needed to close dialog by escape key
    if( m_Config )
    {
        long option;
        m_Config->Read(KEY_ANNOTATE_SORT_OPTION, &option, 0l);
        switch( option )
        {
            case 0:
                m_rbSortBy_X_Position->SetValue(1);
                break;


            case 1:
                m_rbSortBy_Y_Position->SetValue(1);
                break;


            case 2:
                rbSortByValue->SetValue(1);
                break;

            default:
                break;
        }
    }

    wxBitmap bitmap0(annotate_down_right_xpm);
    annotate_down_right_bitmap->SetBitmap(bitmap0);
    wxBitmap bitmap1(annotate_right_down_xpm);
	annotate_right_down_bitmap->SetBitmap(bitmap1);
    wxBitmap bitmap2(add_text_xpm);
	annotate_by_value_bitmap->SetBitmap(bitmap2);

    m_btnApply->SetDefault();
}


/*********************************************************/
void DIALOG_ANNOTATE::OnApplyClick( wxCommandEvent& event )
/*********************************************************/
{
    int response;
    wxString message;

    if( m_Config )
        m_Config->Write(KEY_ANNOTATE_SORT_OPTION, GetSortOrder());

    if( GetResetItems() )
        message = _( "Clear and annotate all of the components " );
    else
        message = _( "Annotate only the unannotated components " );
    if( GetLevel() )
        message += _( "on the entire schematic?" );
    else
        message += _( "on the current sheet?" );

    message += _( "\n\nThis operation will change the current annotation and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );
    if (response == wxCANCEL)
        return;
    AnnotateComponents( m_Parent, GetLevel(), GetSortOrder(), GetResetItems() , true );
    m_btnClear->Enable();
}


/************************************************************************/
void DIALOG_ANNOTATE::OnClearAnnotationCmpClick( wxCommandEvent& event )
/************************************************************************/
{
    int response;

    wxString message = _( "Clear the existing annotation for " );
    if( GetLevel() )
        message += _( "the entire schematic?" );
    else
        message += _( "the current sheet?" );

    message += _( "\n\nThis operation will clear the existing annotation and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ),
                             wxICON_EXCLAMATION | wxOK | wxCANCEL );
    if (response == wxCANCEL)
        return;
    m_Parent->DeleteAnnotation( GetLevel() ? false : true, true );
    m_btnClear->Enable(false);
}


/************************************************************/
void DIALOG_ANNOTATE::OnCancelClick( wxCommandEvent& event )
/************************************************************/
{
    if( IsModal() )
        EndModal( wxID_CANCEL );
    else
    {
        SetReturnCode( wxID_CANCEL );
        this->Show( false );
    }
}


/**************************************/
bool DIALOG_ANNOTATE::GetLevel( void )
/**************************************/
{
    return m_rbEntireSchematic->GetValue();
}

/******************************************/
bool DIALOG_ANNOTATE::GetResetItems( void )
/******************************************/
{
    return m_rbResetAnnotation->GetValue();
}

/****************************************/
int DIALOG_ANNOTATE::GetSortOrder( void )
/****************************************/
/**
 * @return 0 if annotation by X position,
 *         1 if annotation by Y position
 *         2 if annotation by value
 */
{
    if ( m_rbSortBy_X_Position->GetValue() )
        return 0;
    if ( m_rbSortBy_Y_Position->GetValue() )
        return 1;
    return 2;
}

