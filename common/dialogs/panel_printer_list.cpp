/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) KiCad Developers, see AUTHORS.TXT for contributors.
 *
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/panel_printer_list.h>

// Define CUPS_LIST_PRINTERS to allow printer selection in PANEL_PRINTER_LIST
// on Unices. It needs cups and cups dev files
// Currently not tested and not finished for cups, so disable it
// However the code (from wxWidgets forum) is kept here, just in case
//#define CUPS_LIST_PRINTERS

#ifdef __WXMSW__
    #include <windows.h>
    #include <winspool.h>
#elif defined( CUPS_LIST_PRINTERS )
    #include <cups/cups.h>
#endif

// GetPrinterList code comes from samples on:
// https://forums.wxwidgets.org/viewtopic.php?t=13251
// https://forums.wxwidgets.org/viewtopic.php?t=43930
static bool GetPrinterList(wxArrayString &aPrinterList, wxString &aDefaultPrinterName)
{
    aPrinterList.Empty();
    aDefaultPrinterName.Empty();

#ifdef __WXMSW__
    DWORD dwSize, dwPrinters;
    BYTE *pBuffer;

    DWORD szz = 255;
    WCHAR c[256];
    GetDefaultPrinter(&c[0], &szz);
    aDefaultPrinterName = c;

    ::EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &dwSize, &dwPrinters);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || dwSize == 0)
        return false;

    pBuffer = new BYTE[dwSize];
    ::EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, pBuffer, dwSize, &dwSize, &dwPrinters);

    if (dwPrinters != 0)
    {
        PRINTER_INFO_5 *pPrnInfo = (PRINTER_INFO_5 *)pBuffer;

        for (UINT i = 0; i < dwPrinters; i++)
        {
            aPrinterList.Add(pPrnInfo->pPrinterName);
            pPrnInfo++;
        }
    }
    if (pBuffer)
    {
        delete[] pBuffer;
        pBuffer = NULL;
    }

    return true;
#elif defined( CUPS_LIST_PRINTERS )
    cups_dest_t* dests;
    int num_dests = cupsGetDests(&dests);

    for (int i = 0; i < num_dests; i++)
    {
        wxString sz;

        if (dests[i].instance)
            sz = wxString::Format("%s%s", dests[i].name, dests[i].instance);
        else
            sz = wxString::Format("%s", dests[i].name);

        if (dests[i].is_default)
            aDefaultPrinterName = sz;

       aPrinterList.Add(sz);
    }

   //free memory from cups data
   cupsFreeDests(num_dests, dests);

    if(aPrinterList.GetCount())
        return true;

    return false;
#else
    return false;
#endif
}


wxString PANEL_PRINTER_LIST::m_selectedPrinterName;

PANEL_PRINTER_LIST::PANEL_PRINTER_LIST( wxWindow* aParent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size, long style,
                        const wxString& name ):
            PANEL_PRINTER_LIST_BASE( aParent, id, pos, size, style, name )
{

    GetPrinterList( m_printer_list, m_defaultPrinterName);

    if( m_printer_list.size() )
    {
        m_choicePrinter->Append( m_printer_list );
        m_stPrinterState->SetLabel( wxEmptyString );

        bool selected = false;

        if( !m_selectedPrinterName.IsEmpty() )
        {
            for( size_t ii = 0; ii < m_printer_list.GetCount(); ii++ )
            {
                if( m_selectedPrinterName == m_printer_list[ii] )
                {
                    m_choicePrinter->SetSelection( ii );
                    selected = true;
                    break;
                }
            }
        }

        if( !selected )
        {
            for( size_t ii = 0; ii < m_printer_list.GetCount(); ii++ )
            {
                if( m_defaultPrinterName == m_printer_list[ii] )
                {
                    m_choicePrinter->SetSelection( ii );
                    m_selectedPrinterName = m_defaultPrinterName;
                    break;
                }
            }
        }

        if( m_selectedPrinterName == m_defaultPrinterName )
            m_stPrinterState->SetLabel( _( "Default printer" ) );
    }
}


PANEL_PRINTER_LIST::~PANEL_PRINTER_LIST()
{
}


bool PANEL_PRINTER_LIST::AsPrintersAvailable()
{
    return  m_choicePrinter->GetCount() > 0;
}


void PANEL_PRINTER_LIST::onPrinterChoice( wxCommandEvent& event )
{
    int select = m_choicePrinter->GetSelection();

    if( m_choicePrinter->GetString( select ) == m_defaultPrinterName )
        m_stPrinterState->SetLabel( _( "Default printer" ) );
    else
        m_stPrinterState->SetLabel( wxEmptyString );

    m_selectedPrinterName = m_choicePrinter->GetString( select );
}


wxString PANEL_PRINTER_LIST::GetSelectedPrinterName()
{
    if( AsPrintersAvailable() )
    {
        int select = m_choicePrinter->GetSelection();
        m_selectedPrinterName = m_choicePrinter->GetString( select );
    }
    else
        m_selectedPrinterName.Empty();

    return m_selectedPrinterName;
}
