#include "fctsys.h"
#include "macros.h"
#include "gr_basic.h"
#include "libeditframe.h"
#include "class_libentry.h"
#include "lib_pin.h"

#include "dialog_lib_edit_pin.h"

// dialog should remember its previous screen position and size
// Not also if the defaut size is > s_LastSize, default size is used
wxPoint DIALOG_LIB_EDIT_PIN::s_LastPos( -1, -1 );
wxSize  DIALOG_LIB_EDIT_PIN::s_LastSize;

DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( wxWindow* parent, LIB_PIN* aPin ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent )
{
    // Creates a dummy pin to show on a panel, inside this dialog:
    m_dummyPin = new LIB_PIN( *aPin );

    // m_dummyPin changes do not propagate to other pins of the current lib component,
    // so set parent to null and clear flags
    m_dummyPin->SetParent( NULL );
    m_dummyPin->ClearFlags();

    m_panelShowPin->SetBackgroundColour( MakeColour( g_DrawBgColor ) );

    /* Required to make escape key work correctly in wxGTK. */
    SetFocus();
    // Set tab order
    m_textPadName-> MoveAfterInTabOrder(m_textPinName);
    m_sdbSizerButtonsOK->SetDefault();
}

DIALOG_LIB_EDIT_PIN::~DIALOG_LIB_EDIT_PIN()
{
    delete m_dummyPin;
}

void DIALOG_LIB_EDIT_PIN::SetLastSizeAndPosition()
{
    if( s_LastPos.x != -1 )
    {
        wxSize defaultSize = GetSize();
        if(  s_LastSize.x < defaultSize.x )
            s_LastSize.x = defaultSize.x;
        SetSize( s_LastSize );
        SetPosition( s_LastPos );
    }
    else
        Center();
}

/*
 * Draw (on m_panelShowPin) the pin currently edited
 * accroding to current settings in dialog
 */
void DIALOG_LIB_EDIT_PIN::OnPaintShowPanel( wxPaintEvent& event )
{
    wxPaintDC    dc( m_panelShowPin );
    wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Give a parent to m_dummyPin only from draw purpose.
    // In fact m_dummyPin should not have a parent, but draw functions need a parent
    // to know some options, about pin texts
    LIB_EDIT_FRAME* libframe = (LIB_EDIT_FRAME*) GetParent();
    m_dummyPin->SetParent( libframe->GetComponent() );

    // Calculate a suitable scale to fit the available draw area
    EDA_RECT bBox = m_dummyPin->GetBoundingBox();
    double xscale    = (double) dc_size.x / bBox.GetWidth();
    double yscale = (double) dc_size.y / bBox.GetHeight();
    double scale = MIN( xscale, yscale );

    // Give a 10% margin
    scale *= 0.9;
    dc.SetUserScale( scale, scale );

    wxPoint offset =  bBox.Centre();
    NEGATE( offset.x );
    NEGATE( offset.y );

    GRResetPenAndBrush( &dc );
    m_dummyPin->Draw( NULL, &dc, offset, -1, wxCOPY,
                      NULL, DefaultTransform );

    m_dummyPin->SetParent(NULL);

    event.Skip();
}

void DIALOG_LIB_EDIT_PIN::OnCloseDialog( wxCloseEvent& event )
{
    // Save the dialog's position
    s_LastPos  = GetPosition();
    s_LastSize = GetSize();
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_EDIT_PIN::OnCancelButtonClick( wxCommandEvent& event )
{
    // Save the dialog's position
    s_LastPos  = GetPosition();
    s_LastSize = GetSize();
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_EDIT_PIN::OnOKButtonClick( wxCommandEvent& event )
{
    // Save the dialog's position
    s_LastPos  = GetPosition();
    s_LastSize = GetSize();
    EndModal( wxID_OK );
}

// Called when a pin properties changes
void DIALOG_LIB_EDIT_PIN::OnPropertiesChange( wxCommandEvent& event )
{
    if( ! IsShown() )   // do nothing at init time
        return;
    int units = ((LIB_EDIT_FRAME*)GetParent())->m_InternalUnits;
    int pinNameSize = ReturnValueFromString( g_UserUnit, GetNameTextSize(), units );
    int pinNumSize = ReturnValueFromString( g_UserUnit, GetPadNameTextSize(), units);
    int pinOrient = LIB_PIN::GetOrientationCode( GetOrientation() );
    int pinLength = ReturnValueFromString( g_UserUnit, GetLength(), units );
    int pinShape = LIB_PIN::GetStyleCode( GetStyle() );

    m_dummyPin->SetName( GetName() );
    m_dummyPin->SetNameTextSize( pinNameSize );
    m_dummyPin->SetNumber( GetPadName() );
    m_dummyPin->SetNumberTextSize( pinNumSize );
    m_dummyPin->SetOrientation( pinOrient );
    m_dummyPin->SetLength( pinLength );
    m_dummyPin->SetShape( pinShape );
    m_dummyPin->SetVisible( GetVisible() );

    m_panelShowPin->Refresh();
}


void DIALOG_LIB_EDIT_PIN::SetOrientationList( const wxArrayString& list,
                                              const char*** aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceOrientation->Append( list[ii] );
        else
            m_choiceOrientation->Insert( list[ii], KiBitmap( aBitmaps[ii] ), ii );
    }
}


void DIALOG_LIB_EDIT_PIN::SetElectricalTypeList( const wxArrayString& list,
                                                 const char*** aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceElectricalType->Append( list[ii] );
        else
            m_choiceElectricalType->Insert( list[ii], KiBitmap( aBitmaps[ii] ), ii );
    }
}


void DIALOG_LIB_EDIT_PIN::SetStyleList( const wxArrayString& list, const char*** aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceStyle->Append( list[ii] );
        else
            m_choiceStyle->Insert( list[ii], KiBitmap( aBitmaps[ii] ), ii );
    }
}
