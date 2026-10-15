/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/html_message_box.h>

#include "dialog_import_graphics.h"
#include <dialog_map_layers.h>
#include <import_gfx/bitmap_import_plugin.h>
#include <import_gfx/dxf_import_plugin.h>
#include <base_units.h>
#include <bitmap_tracing.h>
#include <cmath>
#include <board.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <pcb_layer_box_selector.h>
#include <wildcards_and_files_ext.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <footprint.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <kiplatform/ui.h>

#include <memory>

#include "settings/settings_manager.h"


static PCB_LAYER_ID getAutoMappedLayer( const wxString& aSourceLayer, const LSET& aPermittedLayers )
{
    for( PCB_LAYER_ID layer : aPermittedLayers.UIOrder() )
    {
        if( aSourceLayer == LayerName( layer ) || aSourceLayer == LSET::Name( layer ) )
            return layer;
    }

    return PCB_LAYER_ID::UNDEFINED_LAYER;
}


static std::vector<INPUT_LAYER_DESC> buildDxfLayerDescriptions( const std::vector<wxString>& aSourceLayers,
                                                                const LSET&                  aPermittedLayers )
{
    std::vector<INPUT_LAYER_DESC> layerDescriptions;

    for( const wxString& sourceLayer : aSourceLayers )
    {
        INPUT_LAYER_DESC desc;
        desc.Name = sourceLayer;
        desc.PermittedLayers = aPermittedLayers;
        desc.AutoMapLayer = getAutoMappedLayer( sourceLayer, aPermittedLayers );
        desc.Required = false;

        layerDescriptions.push_back( desc );
    }

    return layerDescriptions;
}


static LSET getPermittedImportLayers( PCB_BASE_FRAME* aFrame )
{
    if( aFrame && aFrame->GetBoard() )
        return aFrame->GetBoard()->GetEnabledLayers();

    return LSET::AllLayersMask();
}


DIALOG_IMPORT_GRAPHICS::DIALOG_IMPORT_GRAPHICS( PCB_BASE_FRAME* aParent ) :
        DIALOG_IMPORT_GRAPHICS_BASE( aParent ),
        m_parent( aParent ),
        m_xOrigin( aParent, nullptr, m_xCtrl, nullptr ),
        m_yOrigin( aParent, m_yLabel, m_yCtrl, m_yUnits ),
        m_defaultLineWidth( aParent, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
        m_tolerance( aParent, m_toleranceLabel, m_toleranceCtrl, m_toleranceUnits ),
        m_importWidth( aParent, m_importWidthLabel, m_importWidthCtrl, m_importWidthUnits ),
        m_importHeight( aParent, m_importHeightLabel, m_importHeightCtrl, m_importHeightUnits ),
        m_sourceSizeMM( 0, 0 ),
        m_xScale( 1.0 ),
        m_yScale( 1.0 )
{
    // The SVG import has currently a flaw: all SVG shapes are imported as curves and
    // converted to a lot of segments.  A better approach is to convert to polylines
    // (not yet existing in Pcbnew) and keep arcs and circles as primitives (not yet
    // possible with tinysvg library).

    m_importer = std::make_unique<GRAPHICS_IMPORTER_PCBNEW>( aParent->GetModel() );
    m_gfxImportMgr = std::make_unique<GRAPHICS_IMPORT_MGR>();

    // Keyed apart from the schematic dialog, which has the same title and another layout.
    m_hash_key = "DIALOG_IMPORT_GRAPHICS";

    // Configure the layers list selector
    m_SelLayerBox->SetLayersHotkeys( false );    // Do not display hotkeys
    m_SelLayerBox->SetBoardFrame( m_parent );
    m_SelLayerBox->Resync();

    m_dxfUnitsChoice->Append( GetDxfImportUnitChoices() );

    // The chosen unit decides what a DXF file's numbers mean, so its size has to be read again.
    m_dxfUnitsChoice->Bind( wxEVT_CHOICE,
                            [this]( wxCommandEvent& aEvent )
                            {
                                m_sourcePath.Clear();

                                wxCommandEvent dummy;
                                onFilenameChanged( dummy );
                                aEvent.Skip();
                            } );

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    wxCommandEvent dummy;
    onFilenameChanged( dummy );

    SetInitialFocus( m_textCtrlFileName );
    SetupStandardButtons();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_IMPORT_GRAPHICS::SetFilenameOverride( const wxString& aFilenameOverride )
{
    m_filenameOverride = aFilenameOverride;
}


bool DIALOG_IMPORT_GRAPHICS::TransferDataToWindow()
{
    DIALOG_SHIM::TransferDataToWindow();

    // We have to set the filename field value here, otherwise it gets overwritten by state loading
    if( !m_filenameOverride.IsEmpty() )
        m_textCtrlFileName->SetValue( m_filenameOverride );

    // The scale comes back from the last run without an event too, so the size fields have
    // to be told what it is before they are filled in.
    const double scale = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_importScaleCtrl->GetValue() );

    if( scale > 0.0 )
    {
        m_xScale = scale;
        m_yScale = scale;
    }

    // A filename restored from the last run arrives without an event, so sync to it here.
    wxCommandEvent dummy;
    onFilenameChanged( dummy );

    return true;
}


void DIALOG_IMPORT_GRAPHICS::onFilenameChanged( wxCommandEvent& event )
{
    loadSourceFile();

    bool     enableDXFControls = true;
    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();

    if( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( ext ) )
        enableDXFControls = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() ) != nullptr;

    m_defaultLineWidth.Enable( enableDXFControls );

    m_dxfUnitsLabel->Enable( enableDXFControls );
    m_dxfUnitsChoice->Enable( enableDXFControls );

    m_radioBtnMapLayers->Enable( enableDXFControls );

    if( !enableDXFControls && m_radioBtnMapLayers->GetValue() )
        m_radioBtnSingleLayer->SetValue( true );

    updateImageControls();
    updateSizeControls();
}


void DIALOG_IMPORT_GRAPHICS::loadSourceFile()
{
    const wxString path = m_textCtrlFileName->GetValue();

    if( path == m_sourcePath )
        return;

    m_sourcePath = path;
    m_previewSource.Destroy();
    m_sourceSizeMM = VECTOR2D( 0, 0 );

    std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( wxFileName( path ).GetExt() );

    if( !plugin )
        return;

    if( DXF_IMPORT_PLUGIN* dxfPlugin = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() ) )
        dxfPlugin->SetUnit( DxfImportUnitFromChoice( m_dxfUnitsChoice->GetSelection() ) );

    wxLogNull doNotLog;
    LOCALE_IO dummy; // Ensure floats can be read.

    if( !plugin->Load( path ) )
        return;

    const VECTOR2D size( plugin->GetImageWidth(), plugin->GetImageHeight() );

    // A file holding no geometry leaves its bounding box unset.
    if( std::isfinite( size.x ) && std::isfinite( size.y ) && size.x > 0 && size.y > 0 )
        m_sourceSizeMM = size;

    // The image data is shared, so this keeps the pixels without reading the file again.
    if( BITMAP_IMPORT_PLUGIN* bitmapPlugin = dynamic_cast<BITMAP_IMPORT_PLUGIN*>( plugin.get() ) )
        m_previewSource = bitmapPlugin->GetImage();
}


void DIALOG_IMPORT_GRAPHICS::updateSizeControls()
{
    const bool known = m_sourceSizeMM.x > 0 && m_sourceSizeMM.y > 0;

    // Without a source size there is nothing to turn a size into a scale.
    m_importWidth.Enable( known );
    m_importHeight.Enable( known );

    // Scaling the axes apart would turn an imported circle into an ellipse, which the
    // importer has no way to report, so only a traced image may be stretched.
    const bool stretchable = known && m_previewSource.IsOk();

    m_aspectRatioCheckbox->Enable( stretchable );

    if( !stretchable )
        m_aspectRatioCheckbox->SetValue( true );

    if( m_aspectRatioCheckbox->IsChecked() )
        m_yScale = m_xScale;

    sizeFromScale();
}


void DIALOG_IMPORT_GRAPHICS::sizeFromScale()
{
    if( m_sourceSizeMM.x <= 0 || m_sourceSizeMM.y <= 0 )
    {
        m_importWidth.ChangeValue( wxEmptyString );
        m_importHeight.ChangeValue( wxEmptyString );
        return;
    }

    // ChangeValue(), so that writing one field does not look like the user typing in it.
    m_importWidth.ChangeValue( pcbIUScale.mmToIU( m_sourceSizeMM.x * m_xScale ) );
    m_importHeight.ChangeValue( pcbIUScale.mmToIU( m_sourceSizeMM.y * m_yScale ) );
}


void DIALOG_IMPORT_GRAPHICS::scaleFromSize( bool aFromWidth )
{
    if( m_sourceSizeMM.x <= 0 || m_sourceSizeMM.y <= 0 )
        return;

    const double width = pcbIUScale.IUTomm( m_importWidth.GetIntValue() );
    const double height = pcbIUScale.IUTomm( m_importHeight.GetIntValue() );
    const bool   locked = m_aspectRatioCheckbox->IsChecked();

    // A half typed number is not a size to work from.
    if( aFromWidth && width > 0 )
        m_xScale = width / m_sourceSizeMM.x;
    else if( !aFromWidth && height > 0 )
        m_yScale = height / m_sourceSizeMM.y;
    else
        return;

    if( locked )
    {
        if( aFromWidth )
            m_yScale = m_xScale;
        else
            m_xScale = m_yScale;
    }

    m_importScaleCtrl->ChangeValue( wxString::Format( wxT( "%g" ), m_xScale ) );

    // The field the user is typing in is left alone; only the one that follows is written.
    if( locked && aFromWidth )
        m_importHeight.ChangeValue( pcbIUScale.mmToIU( m_sourceSizeMM.y * m_yScale ) );
    else if( locked )
        m_importWidth.ChangeValue( pcbIUScale.mmToIU( m_sourceSizeMM.x * m_xScale ) );
}


void DIALOG_IMPORT_GRAPHICS::onScaleChanged( wxCommandEvent& event )
{
    const double scale = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_importScaleCtrl->GetValue() );

    if( scale <= 0.0 )
        return;

    m_xScale = scale;
    m_yScale = scale;

    sizeFromScale();
}


void DIALOG_IMPORT_GRAPHICS::onWidthChanged( wxCommandEvent& event )
{
    scaleFromSize( true );
}


void DIALOG_IMPORT_GRAPHICS::onHeightChanged( wxCommandEvent& event )
{
    scaleFromSize( false );
}


void DIALOG_IMPORT_GRAPHICS::onLockAspectChanged( wxCommandEvent& event )
{
    // Locking the ratio again keeps the width and brings the height back into step.
    if( m_aspectRatioCheckbox->IsChecked() )
    {
        m_yScale = m_xScale;
        sizeFromScale();
    }
}


void DIALOG_IMPORT_GRAPHICS::updateImageControls()
{
    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();
    bool     isImage = false;

    if( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( ext ) )
        isImage = dynamic_cast<BITMAP_IMPORT_PLUGIN*>( plugin.get() ) != nullptr;

    // The whole panel, since a hidden wxSlider still shows its tick labels on macOS.
    m_imagePanel->Show( isImage );

    updatePreview();

    Layout();
    GetSizer()->Fit( this );
}


void DIALOG_IMPORT_GRAPHICS::updatePreview()
{
    if( !m_previewSource.IsOk() )
    {
        m_previewBitmap->SetBitmap( wxNullBitmap );
        return;
    }

    double threshold = (double) m_thresholdSlider->GetValue() / m_thresholdSlider->GetMax();

    m_previewBitmap->SetBitmap( wxBitmap( MakeBilevelPreview( m_previewSource, threshold, m_cbInvertColors->IsChecked(),
                                                              m_previewBitmap->GetMinSize() ) ) );
}


void DIALOG_IMPORT_GRAPHICS::onThresholdChanged( wxScrollEvent& event )
{
    updatePreview();
}


void DIALOG_IMPORT_GRAPHICS::onInvertChanged( wxCommandEvent& event )
{
    updatePreview();
}


void DIALOG_IMPORT_GRAPHICS::onBrowseFiles( wxCommandEvent& event )
{
    wxString path;
    wxString filename = m_textCtrlFileName->GetValue();

    if( !filename.IsEmpty() )
    {
        wxFileName fn( filename );
        path = fn.GetPath();
        filename = fn.GetFullName();
    }

    // Generate the list of handled file formats
    wxString wildcardsDesc;
    wxString allWildcards;

    for( GRAPHICS_IMPORT_MGR::GFX_FILE_T pluginType : m_gfxImportMgr->GetImportableFileTypes() )
    {
        std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPlugin( pluginType );
        const std::vector<std::string>          extensions = plugin->GetFileExtensions();

        wildcardsDesc += wxT( "|" ) + plugin->GetName() + AddFileExtListToFilter( extensions );
        allWildcards += plugin->GetWildcards() + wxT( ";" );
    }

    wildcardsDesc = _( "All supported formats" ) + wxT( "|" ) + allWildcards + wildcardsDesc;

    wxFileDialog dlg( m_parent, _( "Import Graphics" ), path, filename, wildcardsDesc,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_OK && !dlg.GetPath().IsEmpty() )
        m_textCtrlFileName->SetValue( dlg.GetPath() );
}


bool DIALOG_IMPORT_GRAPHICS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_textCtrlFileName->GetValue().IsEmpty() )
    {
        wxMessageBox( _( "Please select a file to import." ) );
        return false;
    }

    bool useDxfLayerMapping = m_radioBtnMapLayers->IsEnabled() && m_radioBtnMapLayers->GetValue();
    bool useSingleLayer = m_radioBtnSingleLayer->GetValue();

    if( useSingleLayer && m_SelLayerBox->GetLayerSelection() < 0 )
    {
        wxMessageBox( _( "Please select a valid layer." ) );
        return false;
    }

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();
    wxString ext = wxFileName( m_textCtrlFileName->GetValue() ).GetExt();
    double   scale = EDA_UNIT_UTILS::UI::DoubleValueFromString( m_importScaleCtrl->GetValue() );

    if( scale <= 0.0 )
    {
        wxMessageBox( _( "Import scale must be a positive number." ) );
        return false;
    }

    double xscale = m_xScale;
    double yscale = m_yScale;

    if( cfg->m_Display.m_DisplayInvertXAxis )
        xscale *= -1.0;

    if( cfg->m_Display.m_DisplayInvertYAxis )
        yscale *= -1.0;

    VECTOR2D origin( m_xOrigin.GetDoubleValue() / xscale, m_yOrigin.GetDoubleValue() / yscale );

    switch( m_originCtrl->GetSelection() )
    {
    case 0:                                      break;
    case 1: origin += m_parent->GetAuxOrigin();  break;     // Drill/place file origin
    case 2: origin += m_parent->GetGridOrigin(); break;
    case 3: origin += m_parent->GetUserOrigin(); break;
    }

    if( std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = m_gfxImportMgr->GetPluginByExt( ext ) )
    {
        DXF_IMPORT_PLUGIN* dxfPlugin = dynamic_cast<DXF_IMPORT_PLUGIN*>( plugin.get() );

        if( dxfPlugin )
        {
            dxfPlugin->SetUnit( DxfImportUnitFromChoice( m_dxfUnitsChoice->GetSelection() ) );
            m_importer->SetLineWidthMM( pcbIUScale.IUTomm( m_defaultLineWidth.GetIntValue() ) );
        }
        else
        {
            m_importer->SetLineWidthMM( 0.0 );
        }

        if( BITMAP_IMPORT_PLUGIN* bitmapPlugin = dynamic_cast<BITMAP_IMPORT_PLUGIN*>( plugin.get() ) )
        {
            bitmapPlugin->SetThreshold( (double) m_thresholdSlider->GetValue() / m_thresholdSlider->GetMax() );
            bitmapPlugin->SetInvert( m_cbInvertColors->IsChecked() );
        }

        m_importer->SetPlugin( std::move( plugin ) );
        m_importer->ClearLayerMap();

        if( useSingleLayer )
            m_importer->SetLayer( PCB_LAYER_ID( m_SelLayerBox->GetLayerSelection() ) );
        else
            m_importer->SetLayer( m_parent->GetActiveLayer() );

        m_importer->SetImportOffsetMM( origin * pcbIUScale.IUTomm( 1 ) );

        LOCALE_IO dummy;    // Ensure floats can be read.

        if( m_importer->Load( m_textCtrlFileName->GetValue() ) )
        {
            if( useDxfLayerMapping && dxfPlugin )
            {
                std::vector<wxString> sourceLayers = dxfPlugin->GetSourceLayers();

                if( !sourceLayers.empty() )
                {
                    LSET permittedLayers = getPermittedImportLayers( m_parent );
                    std::map<wxString, PCB_LAYER_ID> layerMap = DIALOG_MAP_LAYERS::RunModal(
                            this, buildDxfLayerDescriptions( sourceLayers, permittedLayers ) );

                    if( layerMap.empty() )
                        return false;

                    m_importer->SetLayerMap( layerMap );
                }
            }

            m_importer->Import( VECTOR2D( m_xScale, m_yScale ) );
        }

        // Get warning messages:
        wxString warnings = m_importer->GetMessages();

        // This isn't a fatal error so allow the dialog to close with wxID_OK.
        if( !warnings.empty() )
        {
            HTML_MESSAGE_BOX dlg( this, _( "Warning" ) );
            dlg.MessageSet( _( "Items in the imported file could not be handled properly." ) );
            warnings.Replace( wxT( "\n" ), wxT( "<br/>" ) );
            dlg.AddHTML_Text( warnings );
            dlg.ShowModal();
        }

        return true;
    }
    else
    {
        wxMessageBox( _( "There is no plugin to handle this file type." ) );
        return false;
    }
}


void DIALOG_IMPORT_GRAPHICS::onUpdateUI( wxUpdateUIEvent& event )
{
    m_xOrigin.Enable( m_placeAtCheckbox->GetValue() );
    m_yOrigin.Enable( m_placeAtCheckbox->GetValue() );
    m_originLabel->Enable( m_placeAtCheckbox->GetValue() );
    m_originCtrl->Enable( m_placeAtCheckbox->GetValue() );

    m_tolerance.Enable( m_rbFixDiscontinuities->GetValue() );

    m_SelLayerBox->Enable( m_radioBtnSingleLayer->GetValue() );
}
