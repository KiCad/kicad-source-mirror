/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#include <wx/app.h>
#include <wx/config.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

#include <pcb_calculator_frame_base.h>
#include <pcb_calculator.h>
#include <transline.h>

/*
 * Return the value from a string,
 * Unlike standard string to double convertion,
 * both point and comma F.P. separator are accepted
 * and values having units (like 4.7 K) are accepted
 * but units are ignored.
 * notation like 1e+3 is legal
 */
double DoubleFromString( const wxString& TextValue )
{
    double value = 0;

    /* Acquire the 'right' decimal point separator */
    const struct lconv* lc = localeconv();
    wxChar decimal_point = lc->decimal_point[0];
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
        if( !( (ch >= '0' && ch <='9') || (ch == decimal_point)
             || (ch == '-') || (ch == '+') || (ch == 'e')  || (ch == 'E') ) )
        {
            break;
        }
        ++brk_point;
    }

    /* Extract the numeric part */
    buf.Left( brk_point ).ToDouble( &value );

    return value;
}


// Functions to Read/Write parameters in pcb_calculator main frame:
// They are only wrapper to actual functions, so all transline functions do not
// depend on Graphic User Interface
void SetPropertyInDialog( enum PRMS_ID aPrmId, double value )
{
    PCB_CALCULATOR_FRAME * frame = (PCB_CALCULATOR_FRAME *) wxTheApp->GetTopWindow();
    frame->SetPrmValue( aPrmId, value );
}

/* Puts the text into the given result line.
*/
void SetResultInDialog( int line, const char* aText )
{
    PCB_CALCULATOR_FRAME * frame = (PCB_CALCULATOR_FRAME *) wxTheApp->GetTopWindow();
    wxString msg = wxString::FromUTF8( aText );
    frame->SetResult( line, msg );
}

/* print aValue into the given result line.
*/
void SetResultInDialog( int aLineNumber, double aValue, const char* aText )
{
    PCB_CALCULATOR_FRAME * frame = (PCB_CALCULATOR_FRAME *) wxTheApp->GetTopWindow();
    wxString msg = wxString::FromUTF8( aText );
    wxString fullmsg;
    fullmsg.Printf( wxT("%g "), aValue );
    fullmsg += msg;
    frame->SetResult( aLineNumber, fullmsg );
}

/* Returns a named property value. */
double GetPropertyInDialog( enum PRMS_ID aPrmId )
{
    PCB_CALCULATOR_FRAME * frame = (PCB_CALCULATOR_FRAME *) wxTheApp->GetTopWindow();
    return frame->GetPrmValue( aPrmId );
}

// Returns true if the param aPrmId is selected
// Has meaning only for params that have a radio button
bool   IsSelectedInDialog( enum PRMS_ID aPrmId )
{
    PCB_CALCULATOR_FRAME * frame = (PCB_CALCULATOR_FRAME *) wxTheApp->GetTopWindow();
    return frame->IsPrmSelected( aPrmId );
}


/**
 * Function GetPrmValue
 * Returns a param value.
 * @param aPrmId = param id to write
 * @return the value always in normalized unit (meter, Hz, Ohm, radian)
 */
double PCB_CALCULATOR_FRAME::GetPrmValue( enum PRMS_ID aPrmId )
{
    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];
    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );
        if( aPrmId == prm->m_Id )
            return prm->m_NormalizedValue;
    }

    wxLogMessage( wxT("GetPrmValue: prm %d not found"), (int) aPrmId );
    return 1.0;
}

/**
 * Function SetPrmValue
 * Read/write params values and results
 * @param aPrmId = param id to write
 * @param aValue = value to write
 */
void PCB_CALCULATOR_FRAME::SetPrmValue( enum PRMS_ID aPrmId, double aValue )
{
    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];
    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );
        if( aPrmId == prm->m_Id )
        {
            prm->m_Value = prm->m_NormalizedValue = aValue;
            prm->m_NormalizedValue = aValue;
            prm->m_Value = prm->m_NormalizedValue * prm->ToUserUnit();
            wxString msg;
            msg.Printf( wxT("%g"), prm->m_Value);
            ((wxTextCtrl*)prm->m_ValueCtrl )->SetValue( msg );
            return;
        }
    }

    wxLogMessage( wxT("GetPrmValue: prm %d not found"), (int) aPrmId );
}

/**
 * Function SetResult
 * Puts the text into the given result line.
 * @param aLineNumber = the line (0 to MSG_CNT_MAX-1) wher to display the text
 * @param aText = the text to display
 */
void PCB_CALCULATOR_FRAME::SetResult( int aLineNumber, const wxString & aText )
{
    #define MSG_CNT_MAX 7
    wxStaticText * messages[MSG_CNT_MAX] =
        { m_Message1, m_Message2, m_Message3,
          m_Message4, m_Message5, m_Message6,
            m_Message7
        };

    wxASSERT( ( aLineNumber >= 0 ) && ( aLineNumber < MSG_CNT_MAX ) );

    if( aLineNumber < 0 )
        aLineNumber = 0;
    if( aLineNumber >= MSG_CNT_MAX )
        aLineNumber = MSG_CNT_MAX-1;

    messages[aLineNumber]->SetLabel( aText );
}

/**
 * Function IsPrmSelected
 * @return true if the param aPrmId is selected
 * Has meaning only for params that have a radio button
 */
bool   PCB_CALCULATOR_FRAME::IsPrmSelected( enum PRMS_ID aPrmId )
{
    switch( aPrmId )
    {
        default:
            wxMessageBox( wxT("IsPrmSelected() error") );
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

