/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_IMPORTED_LAYERS_H
#define DIALOG_IMPORTED_LAYERS_H

#include <dialog_imported_layers_base.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>

class WX_INFOBAR;
class BOARD;


class DIALOG_MAP_LAYERS : public DIALOG_IMPORTED_LAYERS_BASE
{
public:
    DIALOG_MAP_LAYERS( wxWindow* aParent, const std::vector<INPUT_LAYER_DESC>& aLayerDesc,
                       const BOARD* aValidationBoard, bool aImportNames );

    /**
     * Return a list of layers names that are required, but they are not mapped.
     */
    std::vector<wxString> GetUnmappedRequiredLayers() const;

    /**
     * Create and show a dialog (modal) and returns the data from it after completion. If the
     * dialog is closed or cancel is pressed, board import throws IO_CANCELLED and graphics
     * mapping returns an empty map.
     *
     * @param aParent Parent window for the invoked dialog.
     * @param aLayerDesc
     * @return Mapped layers
     */
    static std::map<wxString, PCB_LAYER_ID> RunModal( wxWindow*                            aParent,
                                                      const std::vector<INPUT_LAYER_DESC>& aLayerDesc,
                                                      std::map<PCB_LAYER_ID, wxString>*    aRenamedLayers = nullptr,
                                                      const BOARD*                         aValidationBoard = nullptr );

private:
    std::map<wxString, wxString> GetInvalidLayerNames() const;
    void                         UpdateNameWarning();
    void                         OnNameEdited( wxListEvent& event );
    void                         OnRenameClicked( wxCommandEvent& event );
    wxString                     SourceNameForRow( long aRow ) const;
    wxString                     ImportedLayerName( const wxString& aSourceName ) const;

    //Helper functions
    PCB_LAYER_ID GetSelectedLayerID();
    PCB_LAYER_ID GetAutoMatchLayerID( const wxString& aInputLayerName );

    void AddMappings();
    void RemoveMappings( int aStatus );
    void DeleteListItems( const wxArrayInt& aRowsToDelete, wxListCtrl* aListCtrl );

    void AutoMatchLayers();

    const INPUT_LAYER_DESC* GetLayerDescription( const wxString& aLayerName ) const;

    /// KiCad layer name to display, using the destination board's names when available.
    wxString KiCadLayerName( PCB_LAYER_ID aLayer ) const;

    static wxString WrapRequired( const wxString& aLayerName );
    static wxString UnwrapRequired( const wxString& aLayerName );

    //Event Handlers
    void OnAutoMatchLayersClicked( wxCommandEvent& event ) override;

    void OnUnMatchedDoubleClick( wxListEvent& event ) override  { AddMappings(); }
    void OnAddClicked( wxCommandEvent& event ) override         { AddMappings(); }
    void OnMatchedDoubleClick( wxListEvent& event ) override    { RemoveMappings( selected ); }
    void OnRemoveClicked( wxCommandEvent& event ) override      { RemoveMappings( selected ); }
    void OnRemoveAllClicked( wxCommandEvent& event ) override   { RemoveMappings( allitems ); }

    const int selected = wxLIST_STATE_SELECTED;
    const int allitems = wxLIST_STATE_DONTCARE;

    BOARD*                           m_board = nullptr;
    const BOARD*                     m_validationBoard = nullptr;
    std::vector<INPUT_LAYER_DESC>    m_input_layers;
    std::vector<wxString>            m_unmatched_layer_names;
    std::map<wxString, PCB_LAYER_ID> m_matched_layers_map;
    std::map<wxString, wxString>     m_renamedLayerNames;
    std::vector<wxString>            m_sourceNames;
    WX_INFOBAR*                      m_nameWarning = nullptr;
    wxButton*                        m_renameButton = nullptr;
};

#endif // DIALOG_IMPORTED_LAYERS_H
