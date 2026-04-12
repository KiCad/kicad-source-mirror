/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_lcsc_import.h"
#include <python_scripting.h>
#include <sch_base_frame.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <libraries/symbol_library_adapter.h>
#include <project_sch.h>
#include <widgets/symbol_preview_widget.h>
#include <widgets/footprint_preview_widget.h>
#include <widgets/footprint_3d_preview_widget.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <gal/gal_display_options.h>
#include <wx/filename.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>


PANEL_LCSC_IMPORT::PANEL_LCSC_IMPORT( SCH_BASE_FRAME* aFrame, wxWindow* aParent,
                                       std::function<void()> aAcceptHandler ) :
        wxPanel( aParent, wxID_ANY ),
        m_frame( aFrame ),
        m_acceptHandler( std::move( aAcceptHandler ) ),
        m_unitLabel( nullptr ),
        m_unitSelector( nullptr ),
        m_symbolPreview( nullptr ),
        m_fpPreview( nullptr ),
        m_fp3dPreview( nullptr ),
        m_watchdog( nullptr ),
        m_watchdogUnit( 0 )
{
    // ---------------------------------------------------------------
    // Create per-session staging directory in /tmp
    // ---------------------------------------------------------------
    wxString pid = wxString::Format( wxT( "%ld" ), static_cast<long>( ::wxGetProcessId() ) );
    m_tempDir = wxFileName::GetTempDir() + wxFILE_SEP_PATH + wxT( "kicad_lcsc_" ) + pid;
    wxFileName::Mkdir( m_tempDir, 0700, wxPATH_MKDIR_FULL );

    // ---------------------------------------------------------------
    // Watchdog timer (one-shot, retries preview after 400 ms)
    // ---------------------------------------------------------------
    m_watchdog = new wxTimer( this );
    Bind( wxEVT_TIMER, &PANEL_LCSC_IMPORT::onWatchdogTimer, this, m_watchdog->GetId() );

    // ---------------------------------------------------------------
    // UI layout
    // ---------------------------------------------------------------
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // -- Search bar --
    wxBoxSizer* searchSizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* label = new wxStaticText( this, wxID_ANY, _( "LCSC Part Number:" ) );
    searchSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    m_partNumberCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                        wxSize( 200, -1 ), wxTE_PROCESS_ENTER );
    m_partNumberCtrl->SetHint( _( "e.g. C25804" ) );
    searchSizer->Add( m_partNumberCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    m_importBtn = new wxButton( this, wxID_ANY, _( "Import" ) );
    searchSizer->Add( m_importBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    m_addToLibBtn = new wxButton( this, wxID_ANY, _( "Add to Library" ) );
    m_addToLibBtn->Enable( false );   // enabled after first successful import
    searchSizer->Add( m_addToLibBtn, 0, wxALIGN_CENTER_VERTICAL, 0 );

    mainSizer->Add( searchSizer, 0, wxEXPAND | wxALL, 8 );

    // -- Progress bar --
    m_progressBar = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1, 6 ) );
    m_progressBar->Hide();
    mainSizer->Add( m_progressBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 8 );

    // -- Splitter: results list (left) + preview panel (right) --
    m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxSP_LIVE_UPDATE | wxSP_3DSASH );
    m_splitter->SetMinimumPaneSize( 200 );

    // Left: results list
    m_resultsList = new wxListCtrl( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT | wxLC_SINGLE_SEL );
    m_resultsList->AppendColumn( _( "LCSC #" ),  wxLIST_FORMAT_LEFT, 90 );
    m_resultsList->AppendColumn( _( "Name" ),    wxLIST_FORMAT_LEFT, 180 );
    m_resultsList->AppendColumn( _( "Symbol" ),  wxLIST_FORMAT_LEFT, 50 );
    m_resultsList->AppendColumn( _( "FP" ),      wxLIST_FORMAT_LEFT, 50 );
    m_resultsList->AppendColumn( _( "3D" ),      wxLIST_FORMAT_LEFT, 50 );

    // Right: preview panel
    wxPanel*    previewPanel = new wxPanel( m_splitter );
    wxBoxSizer* previewSizer = new wxBoxSizer( wxVERTICAL );

    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE::GAL_TYPE_OPENGL;
    if( m_frame->GetCanvas() )
        backend = m_frame->GetCanvas()->GetBackend();
    else if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        backend = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( cfg->m_Graphics.canvas_type );

    // Symbol unit selector row (hidden until a multi-unit symbol is shown)
    wxBoxSizer* unitSizer = new wxBoxSizer( wxHORIZONTAL );
    m_unitLabel    = new wxStaticText( previewPanel, wxID_ANY, _( "Unit:" ) );
    m_unitSelector = new wxChoice( previewPanel, wxID_ANY );
    unitSizer->Add( m_unitLabel,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
    unitSizer->Add( m_unitSelector, 1, wxALIGN_CENTER_VERTICAL, 0 );
    previewSizer->Add( unitSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3 );
    m_unitLabel->Hide();
    m_unitSelector->Hide();

    m_symbolPreview = new SYMBOL_PREVIEW_WIDGET( previewPanel, &m_frame->Kiway(), true, backend );
    m_symbolPreview->SetLayoutDirection( wxLayout_LeftToRight );
    previewSizer->Add( m_symbolPreview, 5, wxEXPAND | wxALL, 3 );

    m_fpPreviewBook = new wxNotebook( previewPanel, wxID_ANY );

    m_fpPreview = new FOOTPRINT_PREVIEW_WIDGET( m_fpPreviewBook, m_frame->Kiway() );
    m_fpPreview->SetUserUnits( m_frame->GetUserUnits() );
    m_fpPreviewBook->AddPage( m_fpPreview, _( "2D" ), true );

    m_fp3dPreview = new FOOTPRINT_3D_PREVIEW_WIDGET( m_fpPreviewBook, m_frame->Kiway() );
    m_fpPreviewBook->AddPage( m_fp3dPreview, _( "3D" ), false );

    previewSizer->Add( m_fpPreviewBook, 5, wxEXPAND | wxALL, 3 );
    previewPanel->SetSizer( previewSizer );

    m_splitter->SplitVertically( m_resultsList, previewPanel, 420 );
    mainSizer->Add( m_splitter, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8 );

    // -- Status text --
    m_statusText = new wxStaticText( this, wxID_ANY,
            _( "Enter an LCSC part number and click Import." ) );
    mainSizer->Add( m_statusText, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8 );

    SetSizer( mainSizer );

    // -- Event bindings --
    m_importBtn->Bind(    wxEVT_BUTTON,             &PANEL_LCSC_IMPORT::onImportClicked,   this );
    m_addToLibBtn->Bind(  wxEVT_BUTTON,             &PANEL_LCSC_IMPORT::onAddToLibClicked, this );
    m_partNumberCtrl->Bind( wxEVT_TEXT_ENTER,       &PANEL_LCSC_IMPORT::onPartNumberEnter, this );
    m_resultsList->Bind(  wxEVT_LIST_ITEM_SELECTED, &PANEL_LCSC_IMPORT::onResultSelected,  this );
    m_unitSelector->Bind( wxEVT_CHOICE,             &PANEL_LCSC_IMPORT::onUnitSelected,    this );
}


PANEL_LCSC_IMPORT::~PANEL_LCSC_IMPORT()
{
    if( m_watchdog )
        m_watchdog->Stop();
}


void PANEL_LCSC_IMPORT::ShutdownCanvases()
{
    if( m_symbolPreview )
        m_symbolPreview->SetStatusText( wxEmptyString );

    if( m_fp3dPreview )
        m_fp3dPreview->SetStatusText( wxEmptyString );
}


// ---------------------------------------------------------------------------
// Button / list handlers
// ---------------------------------------------------------------------------

void PANEL_LCSC_IMPORT::onImportClicked( wxCommandEvent& /*aEvent*/ )
{
    wxString partNumber = m_partNumberCtrl->GetValue().Trim().Trim( false );

    if( partNumber.IsEmpty() )
    {
        m_statusText->SetLabel( _( "Please enter a part number." ) );
        return;
    }

    if( !partNumber.StartsWith( "C" ) && !partNumber.StartsWith( "c" ) )
        partNumber = "C" + partNumber;

    partNumber.MakeUpper();

    m_statusText->SetLabel( wxString::Format( _( "Importing %s..." ), partNumber ) );
    m_progressBar->Show();
    m_progressBar->Pulse();
    Update();
    wxYield();

    bool ok = doImport( partNumber );

    m_progressBar->Hide();

    if( ok )
    {
        m_statusText->SetLabel( wxString::Format(
                _( "Staged %s.  Click 'Add to Library' to save permanently." ), partNumber ) );

        // Enable the Add-to-Library button now that staging has something.
        m_addToLibBtn->Enable( true );

        // Select the last row and show its preview.
        int lastIdx = m_resultsList->GetItemCount() - 1;
        if( lastIdx >= 0 && lastIdx < static_cast<int>( m_importedParts.size() ) )
        {
            m_resultsList->SetItemState( lastIdx, wxLIST_STATE_SELECTED,
                                          wxLIST_STATE_SELECTED );
            m_resultsList->EnsureVisible( lastIdx );

            const ImportedPart& part = m_importedParts[lastIdx];
            LIB_ID symId( "LCSC_Parts", part.symbolName );
            m_importedLibId = symId;
            showPreview( symId, part.footprintName, part.unitCount );
        }
    }
    else
    {
        m_statusText->SetLabel( wxString::Format( _( "Failed to import %s." ), partNumber ) );
    }
}


void PANEL_LCSC_IMPORT::onPartNumberEnter( wxCommandEvent& /*aEvent*/ )
{
    wxCommandEvent dummy;
    onImportClicked( dummy );
}


void PANEL_LCSC_IMPORT::onResultSelected( wxListEvent& aEvent )
{
    int idx = aEvent.GetIndex();
    if( idx >= 0 && idx < static_cast<int>( m_importedParts.size() ) )
    {
        const ImportedPart& part = m_importedParts[idx];
        LIB_ID symId( "LCSC_Parts", part.symbolName );
        m_importedLibId = symId;
        showPreview( symId, part.footprintName, part.unitCount );
    }
}


void PANEL_LCSC_IMPORT::onUnitSelected( wxCommandEvent& /*aEvent*/ )
{
    if( !m_importedLibId.IsValid() || !m_symbolPreview )
        return;

    int unit = m_unitSelector->GetSelection() + 1;   // KiCad units are 1-based
    m_symbolPreview->DisplaySymbol( m_importedLibId, unit );
}


void PANEL_LCSC_IMPORT::onAddToLibClicked( wxCommandEvent& /*aEvent*/ )
{
    m_statusText->SetLabel( _( "Adding to library..." ) );
    Update();
    wxYield();

    if( addToLibrary() )
    {
        m_statusText->SetLabel(
                _( "Added to LCSC_Parts library.  Parts are now visible in the Library tab." ) );
    }
    // On failure, addToLibrary() sets the status text itself.
}


// ---------------------------------------------------------------------------
// Watchdog
// ---------------------------------------------------------------------------

void PANEL_LCSC_IMPORT::onWatchdogTimer( wxTimerEvent& /*aEvent*/ )
{
    if( !m_importedLibId.IsValid() )
        return;

    // Re-ensure library is in the adapter cache (the first LoadOne may have
    // happened before the file was fully flushed to disk).
    PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() )
            ->LoadOne( wxT( "LCSC_Parts" ) );

    // Retry symbol preview
    if( m_symbolPreview )
        m_symbolPreview->DisplaySymbol( m_importedLibId, m_watchdogUnit );

    // Retry footprint preview: bypass the m_libid == aFPID short-circuit in
    // FOOTPRINT_PREVIEW_WIDGET::DisplayFootprint() by calling the underlying
    // panel directly, then clearing the status ourselves.
    if( m_fpPreview && m_fpPreview->IsInitialized() && m_lastFpLibId.IsValid() )
    {
        if( FOOTPRINT_PREVIEW_PANEL_BASE* panel = m_fpPreview->GetPreviewPanel() )
        {
            if( panel->DisplayFootprint( m_lastFpLibId ) )
                m_fpPreview->ClearStatus();
        }
    }
}


// ---------------------------------------------------------------------------
// showPreview
// ---------------------------------------------------------------------------

void PANEL_LCSC_IMPORT::showPreview( const LIB_ID& aSymbolId, const wxString& aFootprint,
                                     int aUnitCount )
{
    // -- Unit selector --
    if( m_unitSelector && m_unitLabel )
    {
        m_unitSelector->Clear();
        if( aUnitCount > 1 )
        {
            for( int i = 1; i <= aUnitCount; ++i )
                m_unitSelector->Append( wxString::Format( _( "Unit %c" ), wxUniChar( 'A' + i - 1 ) ) );
            m_unitSelector->SetSelection( 0 );
            m_unitLabel->Show();
            m_unitSelector->Show();
        }
        else
        {
            m_unitLabel->Hide();
            m_unitSelector->Hide();
        }
        if( m_unitSelector->GetParent() )
            m_unitSelector->GetParent()->Layout();
    }

    // Display symbol (unit 1 when multi-unit; 0 = "all" for single-unit)
    int unit = ( aUnitCount > 1 ) ? 1 : 0;
    m_watchdogUnit = unit;

    if( m_symbolPreview )
        m_symbolPreview->DisplaySymbol( aSymbolId, unit );

    // Display footprint
    if( !aFootprint.IsEmpty() )
    {
        LIB_ID fpId( "LCSC_Parts", aFootprint );
        m_lastFpLibId = fpId;

        if( m_fpPreview && m_fpPreview->IsInitialized() )
            m_fpPreview->DisplayFootprint( fpId );

        if( m_fp3dPreview )
            m_fp3dPreview->DisplayFootprint( fpId );
    }

    // Arm watchdog: fires once after 400 ms to retry if the first render missed
    if( m_watchdog )
    {
        m_watchdog->Stop();
        m_watchdog->StartOnce( 400 );
    }
}


// ---------------------------------------------------------------------------
// doImport — runs easyeda2kicad via embedded Python into m_tempDir
// ---------------------------------------------------------------------------

bool PANEL_LCSC_IMPORT::doImport( const wxString& aLcscId )
{
    PyLOCK lock;

    // The Python script receives 4 positional %s arguments:
    //   1 → m_tempDir   (staging output directory)
    //   2 → aLcscId     (--lcsc_id argument)
    //   3 → aLcscId     (lcsc_id variable for keyword tagging)
    //   4 → aLcscId     (sym_name fallback)
    wxString pyScript = wxString::Format( R"PYSCRIPT(
import sys
import os
import json
import site
import shutil

result = {"success": False, "name": "", "footprint": "", "symbol": False,
          "has_fp": False, "model3d": False, "unit_count": 1, "error": ""}

try:
    # Ensure system site-packages are on the path
    for p in site.getsitepackages() + [site.getusersitepackages()]:
        if p not in sys.path:
            sys.path.append(p)

    e2k_main = None
    try:
        from easyeda2kicad.__main__ import main as e2k_main
    except ImportError:
        plugin_dirs = [
            os.path.expanduser("~/.local/share/kicad/10.0/scripting/plugins"),
        ]
        for d in plugin_dirs:
            if os.path.isdir(d) and d not in sys.path:
                sys.path.insert(0, d)
        try:
            from easyeda2kicad.__main__ import main as e2k_main
        except ImportError:
            pass

    if e2k_main is None:
        raise ImportError("easyeda2kicad not found. Install: pip install easyeda2kicad")

    # --- staging directory (per-session temp) ---
    output_dir = r"%s"
    os.makedirs(output_dir, exist_ok=True)

    e2k_main(["--full", "--lcsc_id=%s", "--output=" + output_dir])

    # easyeda2kicad writes easyeda2kicad.* files; we copy/rename to LCSC_Parts.*
    old_sym     = os.path.join(output_dir, "easyeda2kicad.kicad_sym")
    new_sym     = os.path.join(output_dir, "LCSC_Parts.kicad_sym")
    old_fp_dir  = os.path.join(output_dir, "easyeda2kicad.pretty")
    new_fp_dir  = os.path.join(output_dir, "LCSC_Parts.pretty")
    old_3d_dir  = os.path.join(output_dir, "easyeda2kicad.3dshapes")
    new_3d_dir  = os.path.join(output_dir, "LCSC_Parts.3dshapes")

    # Process symbol library
    if os.path.isfile(old_sym):
        with open(old_sym, "r") as f:
            raw_lines = f.readlines()
        new_lines = []
        lcsc_id = "%s"
        pending_kw = False
        pending_lcsc_val = False
        cur_lcsc_val = ""
        for line in raw_lines:
            text = line.replace('"easyeda2kicad:', '"LCSC_Parts:')
            text = text.replace('"LCSC Part"', '"LCSC"')
            stripped = text.strip()
            if stripped == '"LCSC"':
                pending_lcsc_val = True
                new_lines.append(text)
                continue
            if pending_lcsc_val and stripped.startswith('"'):
                cur_lcsc_val = stripped.strip('"')
                pending_lcsc_val = False
                new_lines.append(text)
                continue
            if stripped == '"ki_keywords"':
                pending_kw = True
                new_lines.append(text)
                continue
            if pending_kw and stripped.startswith('"'):
                kw_val = stripped.strip('"')
                lcsc_to_add = cur_lcsc_val if cur_lcsc_val else lcsc_id
                if lcsc_to_add and lcsc_to_add not in kw_val:
                    kw_val = kw_val + " " + lcsc_to_add
                indent = text[:len(text) - len(text.lstrip())]
                new_lines.append(indent + '"' + kw_val + '"\n')
                pending_kw = False
                continue
            new_lines.append(text)
        with open(new_sym, "w") as f:
            f.writelines(new_lines)

    # Process footprints
    os.makedirs(new_fp_dir, exist_ok=True)
    if os.path.isdir(old_fp_dir):
        for fp_file in os.listdir(old_fp_dir):
            if fp_file.endswith('.kicad_mod'):
                src = os.path.join(old_fp_dir, fp_file)
                dst = os.path.join(new_fp_dir, fp_file)
                with open(src, "r") as f:
                    content = f.read()
                content = content.replace("easyeda2kicad:", "LCSC_Parts:")
                content = content.replace("easyeda2kicad.3dshapes", "LCSC_Parts.3dshapes")
                with open(dst, "w") as f:
                    f.write(content)

    # Copy 3D models
    os.makedirs(new_3d_dir, exist_ok=True)
    if os.path.isdir(old_3d_dir):
        for model_file in os.listdir(old_3d_dir):
            shutil.copy2(os.path.join(old_3d_dir, model_file),
                         os.path.join(new_3d_dir, model_file))

    result["symbol"]  = os.path.isfile(new_sym)
    result["has_fp"]  = os.path.isdir(new_fp_dir) and len(os.listdir(new_fp_dir)) > 0
    result["model3d"] = os.path.isdir(new_3d_dir) and len(os.listdir(new_3d_dir)) > 0
    result["success"] = result["symbol"]

    # Parse symbol name, footprint name, and unit count from the sym file
    sym_name = "%s"
    fp_name  = ""
    last_unit_count = 1
    if result["symbol"]:
        try:
            with open(new_sym, "r") as f:
                lines = f.readlines()
            last_sym  = ""
            last_fp   = ""
            last_unit_count = 1
            pending_prop = None
            for line in lines:
                stripped = line.strip()
                if stripped.startswith('(symbol "') and not stripped.startswith('(symbol_lib'):
                    parts = stripped.split('"')
                    if len(parts) >= 2:
                        cand = parts[1]
                        sp = cand.rsplit('_', 2)
                        if not (len(sp) == 3 and sp[1].isdigit() and sp[2].isdigit()):
                            last_sym = cand
                            last_unit_count = 1
                        elif len(sp) == 3 and sp[1].isdigit() and sp[2].isdigit():
                            unit_idx = int(sp[1])
                            if unit_idx >= 1 and last_sym and cand.startswith(last_sym + '_'):
                                last_unit_count = max(last_unit_count, unit_idx)
                if stripped == '"Footprint"':
                    pending_prop = "Footprint"
                elif pending_prop == "Footprint" and stripped.startswith('"'):
                    fp_val = stripped.strip('"')
                    if ":" in fp_val:
                        last_fp = fp_val.split(":")[-1]
                    else:
                        last_fp = fp_val
                    pending_prop = None
                elif stripped.startswith('(property'):
                    pending_prop = None
                    if '"Footprint"' in stripped:
                        parts = stripped.split('"')
                        for i, p in enumerate(parts):
                            if p == "Footprint" and i + 2 < len(parts) and parts[i + 2]:
                                fp_val = parts[i + 2]
                                if ":" in fp_val:
                                    last_fp = fp_val.split(":")[-1]
                                else:
                                    last_fp = fp_val
                                break
            if last_sym:
                sym_name = last_sym
                result["unit_count"] = last_unit_count
            if last_fp:
                fp_name = last_fp
        except:
            import traceback
            traceback.print_exc()
    result["name"]      = sym_name
    result["footprint"] = fp_name

except Exception as e:
    result["error"] = str(e)
    import traceback
    traceback.print_exc()

_lcsc_import_result_dict = result
)PYSCRIPT", m_tempDir, aLcscId, aLcscId, aLcscId );

    PyErr_Clear();
    int rc = PyRun_SimpleString( pyScript.utf8_str() );

    if( rc != 0 )
    {
        if( PyErr_Occurred() )
        {
            wxString traceback = PyErrStringWithTraceback();
            wxMessageBox( traceback, _( "Python Error" ), wxICON_ERROR | wxOK );
        }
        return false;
    }

    // Retrieve result dict from Python
    PyObject* mainModule = PyImport_AddModule( "__main__" );
    PyObject* mainDict   = PyModule_GetDict( mainModule );
    PyObject* resultDict = PyDict_GetItemString( mainDict, "_lcsc_import_result_dict" );

    if( !resultDict || !PyDict_Check( resultDict ) )
        return false;

    auto getBool = [&]( const char* key ) -> bool
    {
        PyObject* val = PyDict_GetItemString( resultDict, key );
        return val && PyObject_IsTrue( val );
    };
    auto getStr = [&]( const char* key ) -> wxString
    {
        PyObject* val = PyDict_GetItemString( resultDict, key );
        return val ? PyStringToWx( val ) : wxString();
    };

    bool     success   = getBool( "success" );
    bool     hasSymbol = getBool( "symbol" );
    bool     hasFP     = getBool( "has_fp" );
    bool     has3D     = getBool( "model3d" );
    wxString partName  = getStr( "name" );
    wxString fpName    = getStr( "footprint" );
    int      unitCount = 1;
    {
        PyObject* val = PyDict_GetItemString( resultDict, "unit_count" );
        if( val && PyLong_Check( val ) )
            unitCount = static_cast<int>( PyLong_AsLong( val ) );
        if( unitCount < 1 )
            unitCount = 1;
    }

    if( partName.IsEmpty() )
        partName = aLcscId;

    // Add to results list
    long idx = m_resultsList->InsertItem( m_resultsList->GetItemCount(), aLcscId );
    m_resultsList->SetItem( idx, 1, partName );
    m_resultsList->SetItem( idx, 2, hasSymbol ? wxT( "\u2713" ) : wxT( "\u2717" ) );
    m_resultsList->SetItem( idx, 3, hasFP     ? wxT( "\u2713" ) : wxT( "\u2717" ) );
    m_resultsList->SetItem( idx, 4, has3D     ? wxT( "\u2713" ) : wxT( "\u2717" ) );

    ImportedPart ip;
    ip.symbolName    = partName;
    ip.footprintName = fpName;
    ip.unitCount     = unitCount;
    m_importedParts.push_back( ip );

    if( success && hasSymbol )
    {
        m_importedLibId = LIB_ID( "LCSC_Parts", partName );

        wxString lcscLibPath = m_tempDir + wxFILE_SEP_PATH + wxT( "LCSC_Parts.kicad_sym" );
        wxString lcscFpPath  = m_tempDir + wxFILE_SEP_PATH + wxT( "LCSC_Parts.pretty" );

        LIBRARY_MANAGER& libMgr = Pgm().GetLibraryManager();

        // Helper: register or reload a library table entry
        auto ensureLibEntry = [&]( LIBRARY_TABLE_TYPE aType, const wxString& aPath,
                                   const wxString& aTypeStr, const wxString& aDesc )
        {
            std::optional<LIBRARY_TABLE*> tableOpt =
                    libMgr.Table( aType, LIBRARY_TABLE_SCOPE::PROJECT );
            if( !tableOpt )
                return;

            LIBRARY_TABLE* table = *tableOpt;

            if( !table->HasRow( wxT( "LCSC_Parts" ) ) )
            {
                LIBRARY_TABLE_ROW& row = table->InsertRow();
                row.SetNickname( wxT( "LCSC_Parts" ) );
                row.SetURI( aPath );
                row.SetType( aTypeStr );
                row.SetOptions( wxEmptyString );
                row.SetDescription( aDesc );
                row.SetOk( true );
            }
            else
            {
                // Update URI to the current staging path (may have changed between
                // sessions) and flush the IO plugin cache.
                std::optional<LIBRARY_TABLE_ROW*> rowOpt =
                        table->Row( wxT( "LCSC_Parts" ) );
                if( rowOpt )
                    ( *rowOpt )->SetURI( aPath );

                libMgr.ReloadLibraryEntry( aType, wxT( "LCSC_Parts" ),
                                           LIBRARY_TABLE_SCOPE::PROJECT );
            }

            try { table->Save(); } catch( ... ) {}
        };

        if( wxFileName::FileExists( lcscLibPath ) )
        {
            ensureLibEntry( LIBRARY_TABLE_TYPE::SYMBOL, lcscLibPath,
                            wxT( "KiCad" ), wxT( "LCSC imported symbols (staging)" ) );

            // fetchIfLoaded() in LoadSymbol() requires the library to be in the
            // adapter's cache.  Call LoadOne() to populate it.
            PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() )
                    ->LoadOne( wxT( "LCSC_Parts" ) );
        }

        if( wxFileName::DirExists( lcscFpPath ) )
        {
            ensureLibEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, lcscFpPath,
                            wxT( "KiCad" ), wxT( "LCSC imported footprints (staging)" ) );
        }
    }

    return success;
}


// ---------------------------------------------------------------------------
// addToLibrary — merge staging dir into permanent ~/Documents/Kicad/LCSC_Parts/
// ---------------------------------------------------------------------------

bool PANEL_LCSC_IMPORT::addToLibrary()
{
    wxString permanentDir  = wxGetHomeDir() + wxT( "/Documents/Kicad/LCSC_Parts" );
    wxString permanentSym  = permanentDir   + wxFILE_SEP_PATH + wxT( "LCSC_Parts.kicad_sym" );
    wxString permanentFpDir = permanentDir  + wxFILE_SEP_PATH + wxT( "LCSC_Parts.pretty" );
    wxString permanent3dDir = permanentDir  + wxFILE_SEP_PATH + wxT( "LCSC_Parts.3dshapes" );

    wxString tempSym    = m_tempDir + wxFILE_SEP_PATH + wxT( "LCSC_Parts.kicad_sym" );
    wxString tempFpDir  = m_tempDir + wxFILE_SEP_PATH + wxT( "LCSC_Parts.pretty" );
    wxString temp3dDir  = m_tempDir + wxFILE_SEP_PATH + wxT( "LCSC_Parts.3dshapes" );

    if( !wxFileName::FileExists( tempSym ) )
    {
        m_statusText->SetLabel( _( "Nothing to add — import a part first." ) );
        return false;
    }

    // Use Python for the merge: robust S-expression-aware symbol merge
    // plus file copy with 3D-path fixup.
    PyLOCK lock;

    // Build the merge script with concrete path strings embedded.
    // NOTE: all paths are on Linux (forward slashes, no special chars),
    // so direct embedding is safe.  The script avoids any % operators to
    // prevent interference with wxString::Format.
    wxString mergeScript = wxString::Format( R"MERGE(
import os, shutil

_addlib_result = {"success": False, "added_count": 0, "error": ""}

def _extract_sym_blocks(content):
    """
    Yield (name, block_text) for every top-level (symbol ...) entry in a
    kicad_sym file, using a simple parenthesis-depth tracker.
    Skips sub-symbol entries whose last two underscore-separated tokens
    are both digit strings.
    """
    n = len(content)
    depth = 0
    i = 0
    while i < n:
        c = content[i]
        if c == '(':
            depth += 1
            if depth == 2 and content[i:i+9] == '(symbol "':
                name_start = i + 9
                name_end   = content.index('"', name_start)
                name       = content[name_start:name_end]
                sp = name.rsplit('_', 2)
                is_sub = (len(sp) == 3 and sp[1].isdigit() and sp[2].isdigit())
                if not is_sub:
                    # Extract the complete block
                    block_start = i
                    d = 0
                    j = i
                    while j < n:
                        if content[j] == '(':
                            d += 1
                        elif content[j] == ')':
                            d -= 1
                            if d == 0:
                                yield (name, content[block_start : j + 1])
                                depth -= 1
                                i = j + 1
                                break
                        j += 1
                    continue
        elif c == ')':
            depth -= 1
        i += 1

try:
    temp_sym     = r"%s"
    perm_dir     = r"%s"
    temp_fp_dir  = r"%s"
    temp_3d_dir  = r"%s"
    perm_sym     = os.path.join(perm_dir, "LCSC_Parts.kicad_sym")
    perm_fp_dir  = os.path.join(perm_dir, "LCSC_Parts.pretty")
    perm_3d_dir  = os.path.join(perm_dir, "LCSC_Parts.3dshapes")

    os.makedirs(perm_fp_dir, exist_ok=True)
    os.makedirs(perm_3d_dir, exist_ok=True)

    # ---- Symbol file merge ----
    with open(temp_sym, "r") as fh:
        temp_content = fh.read()

    if not os.path.isfile(perm_sym):
        # First time: just copy
        shutil.copy2(temp_sym, perm_sym)
        _addlib_result["added_count"] = sum(1 for _ in _extract_sym_blocks(temp_content))
    else:
        with open(perm_sym, "r") as fh:
            perm_content = fh.read()

        # Collect names already in permanent lib
        perm_names = set(name for name, _ in _extract_sym_blocks(perm_content))

        # Collect new blocks from temp
        new_blocks = [(name, blk) for name, blk in _extract_sym_blocks(temp_content)
                      if name not in perm_names]

        if new_blocks:
            # Strip trailing ')' from permanent content, append new blocks, close
            stripped = perm_content.rstrip()
            if stripped.endswith(')'):
                stripped = stripped[:-1].rstrip()
            with open(perm_sym, "w") as fh:
                fh.write(stripped)
                fh.write("\n")
                for _, blk in new_blocks:
                    fh.write("  " + blk + "\n")
                fh.write(")\n")
            _addlib_result["added_count"] = len(new_blocks)

    # ---- Footprint files (with 3D-path fix) ----
    if os.path.isdir(temp_fp_dir):
        for fp_file in os.listdir(temp_fp_dir):
            if not fp_file.endswith('.kicad_mod'):
                continue
            src = os.path.join(temp_fp_dir, fp_file)
            dst = os.path.join(perm_fp_dir, fp_file)
            with open(src, "r") as fh:
                content = fh.read()
            # Replace absolute temp 3D-model path with permanent 3D-model path
            content = content.replace(temp_3d_dir, perm_3d_dir)
            with open(dst, "w") as fh:
                fh.write(content)

    # ---- 3D models ----
    if os.path.isdir(temp_3d_dir):
        for model_file in os.listdir(temp_3d_dir):
            shutil.copy2(os.path.join(temp_3d_dir, model_file),
                         os.path.join(perm_3d_dir, model_file))

    _addlib_result["success"] = True

except Exception as exc:
    import traceback
    _addlib_result["error"] = str(exc)
    traceback.print_exc()
)MERGE",
            tempSym, permanentDir, tempFpDir, temp3dDir );

    PyErr_Clear();
    PyRun_SimpleString( mergeScript.utf8_str() );

    // Check Python result
    PyObject* mainMod  = PyImport_AddModule( "__main__" );
    PyObject* mainDict = PyModule_GetDict( mainMod );
    PyObject* res      = PyDict_GetItemString( mainDict, "_addlib_result" );

    bool pyOk = false;
    int  added = 0;
    if( res && PyDict_Check( res ) )
    {
        PyObject* v = PyDict_GetItemString( res, "success" );
        pyOk = v && PyObject_IsTrue( v );
        PyObject* c = PyDict_GetItemString( res, "added_count" );
        if( c && PyLong_Check( c ) )
            added = static_cast<int>( PyLong_AsLong( c ) );
    }

    if( !pyOk )
    {
        m_statusText->SetLabel( _( "Failed to add to library — see KiCad console for details." ) );
        return false;
    }

    // ---- Update project library table to point at permanent location ----
    LIBRARY_MANAGER& libMgr = Pgm().GetLibraryManager();

    auto updateLibEntry = [&]( LIBRARY_TABLE_TYPE aType, const wxString& aPath,
                                const wxString& aTypeStr, const wxString& aDesc )
    {
        std::optional<LIBRARY_TABLE*> tableOpt =
                libMgr.Table( aType, LIBRARY_TABLE_SCOPE::PROJECT );
        if( !tableOpt )
            return;
        LIBRARY_TABLE* table = *tableOpt;

        if( !table->HasRow( wxT( "LCSC_Parts" ) ) )
        {
            LIBRARY_TABLE_ROW& row = table->InsertRow();
            row.SetNickname( wxT( "LCSC_Parts" ) );
            row.SetURI( aPath );
            row.SetType( aTypeStr );
            row.SetOptions( wxEmptyString );
            row.SetDescription( aDesc );
            row.SetOk( true );
        }
        else
        {
            std::optional<LIBRARY_TABLE_ROW*> rowOpt = table->Row( wxT( "LCSC_Parts" ) );
            if( rowOpt )
                ( *rowOpt )->SetURI( aPath );

            libMgr.ReloadLibraryEntry( aType, wxT( "LCSC_Parts" ),
                                       LIBRARY_TABLE_SCOPE::PROJECT );
        }
        try { table->Save(); } catch( ... ) {}
    };

    if( wxFileName::FileExists( permanentSym ) )
    {
        updateLibEntry( LIBRARY_TABLE_TYPE::SYMBOL, permanentSym,
                        wxT( "KiCad" ), wxT( "LCSC imported symbols" ) );

        PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() )
                ->LoadOne( wxT( "LCSC_Parts" ) );
    }

    if( wxFileName::DirExists( permanentFpDir ) )
    {
        updateLibEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, permanentFpDir,
                        wxT( "KiCad" ), wxT( "LCSC imported footprints" ) );
    }

    // The current preview LIB_IDs (m_importedLibId, m_lastFpLibId) use the
    // "LCSC_Parts" nickname which now resolves to the permanent location.
    // Re-arm the watchdog so the preview re-renders from the permanent lib.
    if( m_watchdog && m_importedLibId.IsValid() )
    {
        m_watchdog->Stop();
        m_watchdog->StartOnce( 300 );
    }

    wxString msg = wxString::Format(
            wxPLURAL( "Added %d new symbol to LCSC_Parts library.",
                      "Added %d new symbols to LCSC_Parts library.", added ),
            added );
    if( added == 0 )
        msg = _( "All parts already in library — nothing new to add." );
    m_statusText->SetLabel( msg );

    return true;
}
