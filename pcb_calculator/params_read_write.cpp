/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/app.h>
#include <wx/colour.h>
#include <wx/msgdlg.h>

#include <calculator_panels/panel_transline.h>
#include <pcb_calculator_frame.h>
#include <transline/transline.h>

/*
 * Return the value from a string,
 * Unlike standard string to double conversion,
 * both point and comma F.P. separator are accepted
 * and values having units (like 4.7 K) are accepted
 * but units are ignored.
 * notation like 1e+3 is legal
 */
double DoubleFromString( const wxString& TextValue )
{
    double value = 0;

    /* Acquire the 'right' decimal point separator */
    const struct lconv* lc            = localeconv();
    wxChar              decimal_point = lc->decimal_point[0];
    wxString            buf( TextValue.Strip( wxString::both ) );

    /* Convert the period in decimal point */
    buf.Replace( wxT( "." ), wxString( decimal_point, 1 ) );
    // An ugly fix needed by WxWidgets 2.9.1 that sometimes
    // back to a point as separator, although the separator is the comma
    buf.Replace( wxT( "," ), wxString( decimal_point, 1 ) );

    /* Find the end of the numeric part
     *(when units are append to the number, remove them)
    */
    unsigned brk_point = 0;
    while( brk_point < buf.Len() )
    {
        wxChar ch = buf[brk_point];
        if( !( ( ch >= '0' && ch <= '9' ) || ( ch == decimal_point ) || ( ch == '-' )
                    || ( ch == '+' ) || ( ch == 'e' ) || ( ch == 'E' ) ) )
        {
            break;
        }
        ++brk_point;
    }

    // Check for strings that cannot qualify as a number
    if( brk_point == 0 )
        return std::nan( "" );

    /* Extract the numeric part */
    if( !buf.Left( brk_point ).ToDouble( &value ) )
        return std::nan( "" );

    return value;
}


// A helper function to get a reference to the PANEL_TRANSLINE
PANEL_TRANSLINE* getTranslinePanel()
{
    PCB_CALCULATOR_FRAME* frame     = (PCB_CALCULATOR_FRAME*) wxTheApp->GetTopWindow();
    PANEL_TRANSLINE*      transline = frame->GetCalculator<PANEL_TRANSLINE>();

    wxASSERT( transline );
    return transline;
}


// Functions to Read/Write parameters in pcb_calculator main frame:
// They are only wrapper to actual functions, so all transline functions do not
// depend on Graphic User Interface
void SetPropertyInDialog( enum PRMS_ID aPrmId, double value )
{
    getTranslinePanel()->SetPrmValue( aPrmId, value );
}

void SetPropertyBgColorInDialog( enum PRMS_ID aPrmId, const KIGFX::COLOR4D* aCol )
{
    getTranslinePanel()->SetPrmBgColor( aPrmId, aCol );
}

/* Puts the text into the given result line.
*/
void SetResultInDialog( int line, const char* aText )
{
    wxString msg   = wxString::FromUTF8( aText );
    getTranslinePanel()->SetResult( line, msg );
}

/* print aValue into the given result line.
*/
void SetResultInDialog( int aLineNumber, double aValue, const char* aText )
{
    wxString              msg   = wxString::FromUTF8( aText );
    wxString              fullmsg;
    fullmsg.Printf( wxT( "%g " ), aValue );
    fullmsg += msg;
    getTranslinePanel()->SetResult( aLineNumber, fullmsg );
}

/* Returns a named property value. */
double GetPropertyInDialog( enum PRMS_ID aPrmId )
{
    return getTranslinePanel()->GetPrmValue( aPrmId );
}

// Returns true if the param aPrmId is selected
// Has meaning only for params that have a radio button
bool IsSelectedInDialog( enum PRMS_ID aPrmId )
{
    return getTranslinePanel()->IsPrmSelected( aPrmId );
}


/**
 * Function GetPrmValue
 * Returns a param value.
 * @param aPrmId = param id to write
 * @return the value always in normalized unit (meter, Hz, Ohm, radian)
 */
double PANEL_TRANSLINE::GetPrmValue( enum PRMS_ID aPrmId ) const
{
    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];

    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );

        if( aPrmId == prm->m_Id )
            return prm->m_NormalizedValue;
    }

    return 1.0;
}

/**
 * Function SetPrmValue
 * Read/write params values and results
 * @param aPrmId = param id to write
 * @param aValue = value to write
 */
void PANEL_TRANSLINE::SetPrmValue( enum PRMS_ID aPrmId, double aValue )
{
    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];

    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );

        if( aPrmId == prm->m_Id )
        {
            prm->m_NormalizedValue                = aValue;
            prm->m_Value                          = prm->m_NormalizedValue * prm->ToUserUnit();
            wxString msg;
            msg.Printf( wxT( "%g" ), prm->m_Value );
            ( (wxTextCtrl*) prm->m_ValueCtrl )->SetValue( msg );
            return;
        }
    }
    wxLogMessage( wxT( "GetPrmValue: prm %d not found" ), (int) aPrmId );
}

/**
 * Function SetPrmBgColor
 * Set the background color for a given parameter
 * @param aPrmId = @ref PRMS_ID of the parameter
 * @param aCol = color ( @ref KIGFX::COLOR4D * )
 */
void PANEL_TRANSLINE::SetPrmBgColor( enum PRMS_ID aPrmId, const KIGFX::COLOR4D* aCol )
{
    wxColour wxcol = wxColour( static_cast<unsigned char>( aCol->r * 255 ),
            static_cast<unsigned char>( aCol->g * 255 ),
            static_cast<unsigned char>( aCol->b * 255 ) );

    if( !wxcol.IsOk() )
        return;

    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];

    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );
        wxTextCtrl* ctl = static_cast<wxTextCtrl*>( prm->m_ValueCtrl );

        if( aPrmId == prm->m_Id )
        {
            ctl->SetBackgroundColour( wxcol );
            ctl->SetStyle( 0, -1, ctl->GetDefaultStyle() );
            return;
        }

    }
}

/**
 * Function SetResult
 * Puts the text into the given result line.
 * @param aLineNumber = the line (0 to MSG_CNT_MAX-1) where to display the text
 * @param aText = the text to display
 */
void PANEL_TRANSLINE::SetResult( int aLineNumber, const wxString& aText )
{
#define MSG_CNT_MAX 10
    wxStaticText* messages[MSG_CNT_MAX] = { m_Message1, m_Message2, m_Message3, m_Message4, m_Message5,
                                            m_Message6, m_Message7, m_Message8, m_Message9, m_Message10 };

    wxASSERT( ( aLineNumber >= 0 ) && ( aLineNumber < MSG_CNT_MAX ) );

    if( aLineNumber < 0 )
        aLineNumber = 0;

    if( aLineNumber >= MSG_CNT_MAX )
        aLineNumber = MSG_CNT_MAX - 1;

    messages[aLineNumber]->SetLabel( aText );
}

/**
 * Function IsPrmSelected
 * @return true if the param aPrmId is selected
 * Has meaning only for params that have a radio button
 */
bool PANEL_TRANSLINE::IsPrmSelected( enum PRMS_ID aPrmId ) const
{
    switch( aPrmId )
    {
    default:
        wxMessageBox( wxT( "IsPrmSelected() error" ) );
        break;

    case PHYS_WIDTH_PRM:
    case PHYS_DIAM_IN_PRM:
        return m_radioBtnPrm1->GetValue();
        break;

    case PHYS_S_PRM:
    case PHYS_DIAM_OUT_PRM:
        return m_radioBtnPrm2->GetValue();
        break;
    }
    return false;
}
