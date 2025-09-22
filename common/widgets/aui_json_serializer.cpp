/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <widgets/aui_json_serializer.h>

#include <settings/aui_settings.h>

#include <wx/aui/framemanager.h>
#if wxCHECK_VERSION( 3, 3, 0 )
#include <wx/aui/serializer.h>
#include <wx/aui/auibook.h>
#endif
#include <wx/log.h>
#include <wx/window.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
#if wxCHECK_VERSION( 3, 3, 0 )

struct PANE_METADATA
{
    wxAuiPaneInfo* paneInfo;
    wxString       name;
    wxString       windowName;
    wxString       className;
    wxString       caption;
    bool           isToolbar;
    bool           isCenter;
    bool           isNotebook;
    size_t         index;
};

static wxString getWindowName( wxWindow* aWindow )
{
    if( !aWindow )
        return wxString();

    wxString name = aWindow->GetName();

    if( name.IsEmpty() )
        // Fallback: use label if available (GetWindowVariant returns enum, not a string)
        name = aWindow->GetLabel();

    return name;
}

static PANE_METADATA buildMetadata( wxAuiPaneInfo& aInfo, size_t aIndex )
{
    PANE_METADATA meta;

    meta.paneInfo   = &aInfo;
    meta.name       = aInfo.name;
    meta.caption    = aInfo.caption;
    meta.isToolbar  = aInfo.IsToolbar();
    meta.isCenter   = ( aInfo.dock_direction == wxAUI_DOCK_CENTER );
    meta.isNotebook = aInfo.window && wxDynamicCast( aInfo.window, wxAuiNotebook );
    meta.index      = aIndex;

    if( aInfo.window )
    {
        meta.windowName = getWindowName( aInfo.window );

        if( aInfo.window->GetClassInfo() )
            meta.className = aInfo.window->GetClassInfo()->GetClassName();
    }

    return meta;
}

static void addDockLayout( nlohmann::json& aNode, const wxAuiDockLayoutInfo& aLayout )
{
    nlohmann::json dock = nlohmann::json::object();

    dock["direction"]  = aLayout.dock_direction;
    dock["layer"]      = aLayout.dock_layer;
    dock["row"]        = aLayout.dock_row;
    dock["position"]   = aLayout.dock_pos;
    dock["proportion"] = aLayout.dock_proportion;
    dock["size"]       = aLayout.dock_size;

    aNode["dock"] = std::move( dock );
}

static void readDockLayout( const nlohmann::json& aNode, wxAuiDockLayoutInfo& aLayout )
{
    if( !aNode.is_object() )
        return;

    aLayout.dock_direction   = aNode.value( "direction", aLayout.dock_direction );
    aLayout.dock_layer       = aNode.value( "layer", aLayout.dock_layer );
    aLayout.dock_row         = aNode.value( "row", aLayout.dock_row );
    aLayout.dock_pos         = aNode.value( "position", aLayout.dock_pos );
    aLayout.dock_proportion  = aNode.value( "proportion", aLayout.dock_proportion );
    aLayout.dock_size        = aNode.value( "size", aLayout.dock_size );
}

class JSON_SERIALIZER : public wxAuiSerializer
{
public:
    explicit JSON_SERIALIZER( wxAuiManager& aManager ) : m_manager( aManager ), m_paneIndex( 0 )
    {
        m_root = nlohmann::json::object();
    }

    nlohmann::json GetState() const
    {
        return m_root;
    }

    void BeforeSave() override
    {
        m_root["format"]  = "kicad.wxaui";
        m_root["version"] = 1;
    }

    void BeforeSavePanes() override
    {
        m_panes = nlohmann::json::array();
        m_paneIndex = 0;
    }

    void SavePane( const wxAuiPaneLayoutInfo& aPane ) override
    {
        nlohmann::json pane = nlohmann::json::object();

        pane["name"] = aPane.name.ToStdString();

        addDockLayout( pane, aPane );

        if( aPane.floating_pos != wxDefaultPosition || aPane.floating_size != wxDefaultSize )
        {
            nlohmann::json floating = nlohmann::json::object();
            floating["rect"] = wxRect( aPane.floating_pos, aPane.floating_size );

            pane["floating"] = std::move( floating );
        }

        if( aPane.is_maximized )
            pane["maximized"] = true;

        if( aPane.is_hidden )
            pane["hidden"] = true;

        wxAuiPaneInfo& info = m_manager.GetPane( aPane.name );

        nlohmann::json meta = nlohmann::json::object();
        meta["toolbar"]  = info.IsToolbar();
        meta["center"]   = ( info.dock_direction == wxAUI_DOCK_CENTER );
        meta["notebook"] = info.window && wxDynamicCast( info.window, wxAuiNotebook );
        meta["index"]    = m_paneIndex++;

        if( info.window )
        {
            wxString windowName = getWindowName( info.window );

            if( !windowName.IsEmpty() )
                meta["window_name"] = windowName.ToStdString();

            if( info.window->GetClassInfo() )
                meta["class_name"] = wxString( info.window->GetClassInfo()->GetClassName() ).ToStdString();
        }

        if( !info.caption.IsEmpty() )
            meta["caption"] = info.caption.ToStdString();

        pane["meta"] = std::move( meta );

        m_panes.push_back( std::move( pane ) );
    }

    void AfterSavePanes() override
    {
        m_root["panes"] = std::move( m_panes );
    }

    void BeforeSaveNotebooks() override
    {
        m_notebooks = nlohmann::json::array();
    }

    void BeforeSaveNotebook( const wxString& aName ) override
    {
        m_currentNotebook = nlohmann::json::object();
        m_currentNotebook["name"] = aName.ToStdString();

        wxAuiPaneInfo& info = m_manager.GetPane( aName );
        nlohmann::json meta = nlohmann::json::object();

        if( info.window )
        {
            wxString windowName = getWindowName( info.window );

            if( !windowName.IsEmpty() )
                meta["window_name"] = windowName.ToStdString();

            if( info.window->GetClassInfo() )
                meta["class_name"] = wxString( info.window->GetClassInfo()->GetClassName() ).ToStdString();
        }

        if( !info.caption.IsEmpty() )
            meta["caption"] = info.caption.ToStdString();

        meta["toolbar"]  = info.IsToolbar();
        meta["center"]   = ( info.dock_direction == wxAUI_DOCK_CENTER );
        meta["notebook"] = true;

        m_currentNotebook["meta"] = std::move( meta );
        m_currentNotebook["tabs"] = nlohmann::json::array();
    }

    void SaveNotebookTabControl( const wxAuiTabLayoutInfo& aTab ) override
    {
        nlohmann::json tab = nlohmann::json::object();

        addDockLayout( tab, aTab );

        if( !aTab.pages.empty() )
            tab["pages"] = aTab.pages;

        if( !aTab.pinned.empty() )
            tab["pinned"] = aTab.pinned;

        if( aTab.active >= 0 )
            tab["active"] = aTab.active;

        m_currentNotebook["tabs"].push_back( std::move( tab ) );
    }

    void AfterSaveNotebook() override
    {
        m_notebooks.push_back( std::move( m_currentNotebook ) );
        m_currentNotebook = nlohmann::json();
    }

    void AfterSaveNotebooks() override
    {
        if( !m_notebooks.empty() )
            m_root["notebooks"] = std::move( m_notebooks );
    }

    void AfterSave() override {}

private:
    wxAuiManager&   m_manager;
    nlohmann::json  m_root;
    nlohmann::json  m_panes;
    nlohmann::json  m_notebooks;
    nlohmann::json  m_currentNotebook;
    size_t          m_paneIndex;
};

class JSON_DESERIALIZER : public wxAuiDeserializer
{
public:
    JSON_DESERIALIZER( wxAuiManager& aManager, const nlohmann::json& aState )
            : wxAuiDeserializer( aManager ), m_manager( aManager ), m_state( aState )
    {
        if( !m_state.is_object() )
            throw std::runtime_error( "Invalid AUI layout state" );

        const std::string format = m_state.value( "format", std::string() );

        if( format != "kicad.wxaui" )
            throw std::runtime_error( "Unsupported AUI layout format" );

        int version = m_state.value( "version", 0 );

        if( version != 1 )
            throw std::runtime_error( "Unsupported AUI layout version" );

        if( m_state.contains( "panes" ) && m_state["panes"].is_array() )
            m_serializedPanes = m_state["panes"].get<std::vector<nlohmann::json>>();

        if( m_state.contains( "notebooks" ) && m_state["notebooks"].is_array() )
            m_serializedNotebooks = m_state["notebooks"].get<std::vector<nlohmann::json>>();
    }

    std::vector<wxAuiPaneLayoutInfo> LoadPanes() override
    {
        std::vector<wxAuiPaneLayoutInfo> panes;

        wxAuiPaneInfoArray paneArray = m_manager.GetAllPanes();

        std::vector<PANE_METADATA> metadata;
        metadata.reserve( paneArray.GetCount() );

        for( size_t i = 0; i < paneArray.GetCount(); ++i )
            metadata.push_back( buildMetadata( paneArray[i], i ) );

        std::set<wxAuiPaneInfo*> used;

        for( const nlohmann::json& jsonPane : m_serializedPanes )
        {
            if( !jsonPane.is_object() )
                continue;

            wxAuiPaneLayoutInfo pane( wxString::FromUTF8( jsonPane.value( "name", std::string() ) ) );

            readDockLayout( jsonPane.value( "dock", nlohmann::json::object() ), pane );

            if( jsonPane.contains( "floating" ) )
            {
                const nlohmann::json& floating = jsonPane["floating"];

                if( floating.contains( "rect" ) )
                {
                    wxRect rect = floating["rect"].get<wxRect>();
                    pane.floating_pos  = rect.GetPosition();
                    pane.floating_size = rect.GetSize();
                }
            }

            pane.is_maximized = jsonPane.value( "maximized", false );
            pane.is_hidden    = jsonPane.value( "hidden", false );

            wxAuiPaneInfo* actualPane = matchPane( jsonPane, metadata, used );

            if( actualPane )
            {
                pane.name = actualPane->name;
                used.insert( actualPane );
                panes.push_back( pane );
            }
        }

        return panes;
    }

    std::vector<wxAuiTabLayoutInfo> LoadNotebookTabs( const wxString& aName ) override
    {
        const wxAuiPaneInfo& paneInfo = m_manager.GetPane( aName );

        auto loadTabs = []( const nlohmann::json& aNotebook )
        {
            std::vector<wxAuiTabLayoutInfo> tabs;

            if( !aNotebook.contains( "tabs" ) || !aNotebook["tabs"].is_array() )
                return tabs;

            for( const nlohmann::json& tabJson : aNotebook["tabs"].get<std::vector<nlohmann::json>>() )
            {
                if( !tabJson.is_object() )
                    continue;

                wxAuiTabLayoutInfo info;

                readDockLayout( tabJson.value( "dock", nlohmann::json::object() ), info );

                if( tabJson.contains( "pages" ) )
                {
                    info.pages.clear();
                    for( int page : tabJson["pages"].get<std::vector<int>>() )
                        info.pages.push_back( page );
                }

                if( tabJson.contains( "pinned" ) )
                {
                    info.pinned.clear();
                    for( int page : tabJson["pinned"].get<std::vector<int>>() )
                        info.pinned.push_back( page );
                }

                info.active = tabJson.value( "active", info.active );

                tabs.push_back( info );
            }

            return tabs;
        };

        for( const nlohmann::json& notebook : m_serializedNotebooks )
        {
            if( !notebook.is_object() )
                continue;

            wxString storedName = wxString::FromUTF8( notebook.value( "name", std::string() ) );

            if( storedName == aName )
                return loadTabs( notebook );
        }

        if( !paneInfo.IsOk() )
            return {};

        wxString windowName = paneInfo.window ? getWindowName( paneInfo.window ) : wxString();
        wxString className;

        if( paneInfo.window && paneInfo.window->GetClassInfo() )
            className = paneInfo.window->GetClassInfo()->GetClassName();

        for( const nlohmann::json& notebook : m_serializedNotebooks )
        {
            if( !notebook.is_object() )
                continue;

            const nlohmann::json& meta = notebook.value( "meta", nlohmann::json::object() );

            const wxString storedWindowName = wxString::FromUTF8( meta.value( "window_name", std::string() ) );
            const wxString storedClassName  = wxString::FromUTF8( meta.value( "class_name", std::string() ) );

            const bool toolbar  = meta.value( "toolbar", false );
            const bool center   = meta.value( "center", false );
            const bool notebookFlag = meta.value( "notebook", false );

            if( toolbar != paneInfo.IsToolbar() || center != ( paneInfo.dock_direction == wxAUI_DOCK_CENTER ) )
                continue;

            if( !notebookFlag )
                continue;

            if( !windowName.IsEmpty() && !storedWindowName.IsEmpty() && windowName == storedWindowName )
                return loadTabs( notebook );

            if( !className.IsEmpty() && !storedClassName.IsEmpty() && className == storedClassName )
                return loadTabs( notebook );
        }

        return {};
    }

    wxWindow* CreatePaneWindow( wxAuiPaneInfo& ) override
    {
        return nullptr;
    }

private:
    wxAuiPaneInfo* matchPane( const nlohmann::json& aJson,
                              const std::vector<PANE_METADATA>& aMetadata,
                              const std::set<wxAuiPaneInfo*>& aUsed ) const
    {
        auto selectCandidate = [&aUsed]( wxAuiPaneInfo* pane ) -> bool
        {
            return pane && aUsed.find( pane ) == aUsed.end();
        };

        wxString storedName = wxString::FromUTF8( aJson.value( "name", std::string() ) );

        if( !storedName.IsEmpty() )
        {
            wxAuiPaneInfo& pane = m_manager.GetPane( storedName );

            if( pane.IsOk() && selectCandidate( &pane ) )
                return &pane;
        }

        wxAuiPaneInfo* match = nullptr;
        const nlohmann::json& meta = aJson.value( "meta", nlohmann::json::object() );

        const wxString windowName = wxString::FromUTF8( meta.value( "window_name", std::string() ) );
        const wxString className  = wxString::FromUTF8( meta.value( "class_name", std::string() ) );
        const wxString caption    = wxString::FromUTF8( meta.value( "caption", std::string() ) );
        const bool     isToolbar  = meta.value( "toolbar", false );
        const bool     isCenter   = meta.value( "center", false );
        const bool     isNotebook = meta.value( "notebook", false );
        const size_t   index      = meta.value( "index", std::numeric_limits<size_t>::max() );

        auto findBy = [&]( auto predicate ) -> wxAuiPaneInfo*
        {
            wxAuiPaneInfo* candidate = nullptr;

            for( const PANE_METADATA& data : aMetadata )
            {
                if( !predicate( data ) )
                    continue;

                if( !selectCandidate( data.paneInfo ) )
                    continue;

                if( candidate )
                    return nullptr;

                candidate = data.paneInfo;
            }

            return candidate;
        };

        if( !windowName.IsEmpty() )
        {
            match = findBy( [&]( const PANE_METADATA& data )
                            {
                                return data.windowName == windowName;
                            } );

            if( match )
                return match;
        }

        if( !className.IsEmpty() )
        {
            match = findBy( [&]( const PANE_METADATA& data )
                            {
                                if( data.className != className )
                                    return false;

                                if( data.isToolbar != isToolbar )
                                    return false;

                                if( data.isCenter != isCenter )
                                    return false;

                                if( data.isNotebook != isNotebook )
                                    return false;

                                return true;
                            } );

            if( match )
                return match;
        }

        if( index != std::numeric_limits<size_t>::max() )
        {
            match = findBy( [&]( const PANE_METADATA& data )
                            {
                                return data.index == index;
                            } );

            if( match )
                return match;
        }

        if( !caption.IsEmpty() )
        {
            match = findBy( [&]( const PANE_METADATA& data )
                            {
                                return data.caption == caption;
                            } );

            if( match )
                return match;
        }

        match = findBy( [&]( const PANE_METADATA& data )
                        {
                            if( data.isToolbar != isToolbar )
                                return false;

                            if( data.isNotebook != isNotebook )
                                return false;

                            return true;
                        } );

        return match;
    }

    wxAuiManager& m_manager;
    nlohmann::json m_state;
    std::vector<nlohmann::json> m_serializedPanes;
    std::vector<nlohmann::json> m_serializedNotebooks;
};

#endif
} // namespace

WX_AUI_JSON_SERIALIZER::WX_AUI_JSON_SERIALIZER( wxAuiManager& aManager ) : m_manager( aManager )
{
}

nlohmann::json WX_AUI_JSON_SERIALIZER::Serialize() const
{
#if wxCHECK_VERSION( 3, 3, 0 )
    try
    {
        JSON_SERIALIZER serializer( m_manager );
        m_manager.SaveLayout( serializer );
        return serializer.GetState();
    }
    catch( const std::exception& err )
    {
        wxLogWarning( "Failed to serialize AUI layout: %s", err.what() );
    }
#endif

    return nlohmann::json();
}

bool WX_AUI_JSON_SERIALIZER::Deserialize( const nlohmann::json& aState ) const
{
#if wxCHECK_VERSION( 3, 3, 0 )
    if( aState.is_null() || aState.empty() )
        return false;

    try
    {
        JSON_DESERIALIZER deserializer( m_manager, aState );
        m_manager.LoadLayout( deserializer );
        return true;
    }
    catch( const std::exception& err )
    {
        wxLogWarning( "Failed to deserialize AUI layout: %s", err.what() );
    }
#endif

    return false;
}
