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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_HEADLESS_SYMBOL_EDITOR_CONTEXT_H
#define KICAD_HEADLESS_SYMBOL_EDITOR_CONTEXT_H

#include <memory>

#include <api/symbol_editor_context.h>
#include <lib_id.h>

class APP_SETTINGS_BASE;
class KIWAY;
class LIB_SYMBOL;
class PROJECT;
class SCH_SCREEN;
class TOOL_MANAGER;


class HEADLESS_SYMBOL_EDITOR_CONTEXT : public SYMBOL_EDITOR_CONTEXT
{
public:
    HEADLESS_SYMBOL_EDITOR_CONTEXT( std::unique_ptr<LIB_SYMBOL> aSymbol, const LIB_ID& aLibId,
                                    PROJECT* aProject, APP_SETTINGS_BASE* aSettings,
                                    KIWAY* aKiway = nullptr );

    ~HEADLESS_SYMBOL_EDITOR_CONTEXT() override;

    LIB_SYMBOL* GetCurSymbol() const override { return m_symbol.get(); }

    SCH_SCREEN* GetScreen() const override { return m_screen.get(); }

    LIB_ID GetLoadedLibId() const override { return m_libId; }

    bool IsContentModified() const override { return m_contentModified; }

    void SetContentModified( bool aModified = true ) override { m_contentModified = aModified; }

    PROJECT& Prj() const override;

    TOOL_MANAGER* GetToolManager() const override;

    KIWAY* GetKiway() const override { return m_kiway; }

    bool CanAcceptApiCommands() const override { return true; }

    bool SaveSymbol() override;

    bool SaveSymbolCopy( const wxString& aFileName, bool aOverwrite ) override;

    bool RevertSymbol() override;

    bool LoadSymbol( const LIB_ID& aLibId ) override;

private:
    std::unique_ptr<LIB_SYMBOL>   m_symbol;
    std::unique_ptr<SCH_SCREEN>   m_screen;
    LIB_ID                        m_libId;
    PROJECT*                      m_project;
    KIWAY*                        m_kiway;
    bool                          m_contentModified = false;
    std::unique_ptr<TOOL_MANAGER> m_toolManager;
};

#endif // KICAD_HEADLESS_SYMBOL_EDITOR_CONTEXT_H
