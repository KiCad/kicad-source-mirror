/******************************************************************/
/* msgpanel.cpp - fonctions des classes du type WinEDA_MsgPanel */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "colors.h"


/* table des evenements captes par un WinEDA_MsgPanel */
BEGIN_EVENT_TABLE( WinEDA_MsgPanel, wxPanel )
    EVT_PAINT( WinEDA_MsgPanel::OnPaint )
END_EVENT_TABLE()


/***********************************************************/
/* Fonctions de base de WinEDA_MsgPanel: l'ecran de messages */
/***********************************************************/

WinEDA_MsgPanel::WinEDA_MsgPanel( WinEDA_DrawFrame* parent, int id,
                                  const wxPoint& pos, const wxSize& size ) :
    wxPanel( parent, id, pos, size )
{
    m_Parent = parent;
    SetFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_last_x = 0;

    m_fontSize = computeFontSize();
}


WinEDA_MsgPanel::~WinEDA_MsgPanel()
{
}


wxSize WinEDA_MsgPanel::computeFontSize()
{
    // Get size of the wxSYS_DEFAULT_GUI_FONT
    wxSize      fontSizeInPixels;

    wxScreenDC dc;

    dc.SetFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );
    dc.GetTextExtent( wxT( "W" ), &fontSizeInPixels.x, &fontSizeInPixels.y );

    return fontSizeInPixels;
}


int WinEDA_MsgPanel::GetRequiredHeight()
{
    // make space for two rows of text plus a number of pixels between them.
    return 2 * computeFontSize().y + 0;
}


wxSize WinEDA_MsgPanel::computeTextSize( const wxString& text )
{
    // Get size of the wxSYS_DEFAULT_GUI_FONT
    wxSize      textSizeInPixels;

    wxScreenDC dc;

    dc.SetFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );
    dc.GetTextExtent( text, &textSizeInPixels.x, &textSizeInPixels.y );

    return textSizeInPixels;
}


/*************************************************/
void WinEDA_MsgPanel::OnPaint( wxPaintEvent& event )
/*************************************************/
{
    wxPaintDC dc( this );

    erase( &dc );

    dc.SetBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    dc.SetBackgroundMode( wxSOLID );
    dc.SetTextBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    dc.SetFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );

    for( unsigned i=0;  i<m_Items.size();  ++i )
        showItem( dc, m_Items[i] );

    event.Skip();
}

void WinEDA_MsgPanel::AppendMessage( const wxString& textUpper,
                                     const wxString& textLower,
                                     int color, int pad )
{
    wxString    text;
    wxSize      drawSize = GetClientSize();

    text = ( textUpper.Len() > textLower.Len() ) ? textUpper : textLower;
    text.Append( ' ', pad );

    MsgItem item;

    /* Don't put the first message a window client position 0.  Offset by
     * one 'W' character width. */
    if( m_last_x == 0 )
        m_last_x = m_fontSize.x;

    item.m_X = m_last_x;

    item.m_UpperY = ( drawSize.y / 2 ) - m_fontSize.y;
    item.m_LowerY = drawSize.y - m_fontSize.y;

    item.m_UpperText = textUpper;
    item.m_LowerText = textLower;
    item.m_Color = color;
    m_Items.push_back( item );
    m_last_x += computeTextSize( text ).x;

    // Add an extra space between texts for a better look:
    m_last_x += m_fontSize.x;

    Refresh();
}

/*****************************************************************************/
void WinEDA_MsgPanel::Affiche_1_Parametre( int pos_X, const wxString& texte_H,
                                           const wxString& texte_L, int color )
/*****************************************************************************/

/*
 *  Routine d'affichage d'un parametre.
 *  pos_X = cadrage horizontal
 *      si pos_X < 0 : la position horizontale est la derniere
 *          valeur demandee >= 0
 *  texte_H = texte a afficher en ligne superieure.
 *      si "", par d'affichage sur cette ligne
 *  texte_L = texte a afficher en ligne inferieure.
 *      si "", par d'affichage sur cette ligne
 *  color = couleur d'affichage
 */
{
    wxPoint     pos;
    wxSize      drawSize = GetClientSize();

    if( pos_X >= 0 )
    {
        m_last_x = pos.x = pos_X * (m_fontSize.x + 2);
    }
    else
        pos.x = m_last_x;

    MsgItem item;

    item.m_X = pos.x;

    item.m_UpperY = (drawSize.y / 2) - m_fontSize.y;
    item.m_LowerY = drawSize.y - m_fontSize.y;

    item.m_UpperText = texte_H;
    item.m_LowerText = texte_L;
    item.m_Color = color;

    int ndx;

    // update the vector, which is sorted by m_X
    int limit = m_Items.size();
    for( ndx=0;  ndx<limit;  ++ndx )
    {
        // replace any item with same X
        if( m_Items[ndx].m_X == item.m_X )
        {
            m_Items[ndx] = item;
            break;
        }

        if( m_Items[ndx].m_X > item.m_X )
        {
            m_Items.insert( m_Items.begin()+ndx, item );
            break;
        }
    }

    if( ndx==limit )        // mutually exclusive with two above if tests
    {
        m_Items.push_back( item );
    }

    Refresh();
}


void WinEDA_MsgPanel::showItem( wxDC& dc, const MsgItem& aItem )
{
    int color = aItem.m_Color;

    if( color >= 0 )
    {
        color &= MASKCOLOR;
        dc.SetTextForeground( wxColour( ColorRefs[color].m_Red,
                                        ColorRefs[color].m_Green,
                                        ColorRefs[color].m_Blue ) );
    }

    if( !aItem.m_UpperText.IsEmpty() )
    {
        dc.DrawText( aItem.m_UpperText, aItem.m_X, aItem.m_UpperY );
    }

    if( !aItem.m_LowerText.IsEmpty() )
    {
        dc.DrawText( aItem.m_LowerText, aItem.m_X, aItem.m_LowerY );
    }
}


/****************************************/
void WinEDA_MsgPanel::EraseMsgBox()
/****************************************/
{
   m_Items.clear();
   m_last_x = 0;
   Refresh();
}

/*******************************************/
void WinEDA_MsgPanel::erase( wxDC* DC )
/*******************************************/
{
    wxPen   pen;
    wxBrush brush;

    wxSize  size  = GetClientSize();
    wxColor color = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

    pen.SetColour( color );

    brush.SetColour( color );
    brush.SetStyle( wxSOLID );

    DC->SetPen( pen );
    DC->SetBrush( brush );

    DC->DrawRectangle( 0, 0, size.x, size.y );
}
