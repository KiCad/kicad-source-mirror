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

#ifndef EMBED_TOOL_H
#define EMBED_TOOL_H

#include <tool/tool_interactive.h>

class wxFileName;
class EMBEDDED_FILES;

class EMBED_TOOL : public TOOL_INTERACTIVE
{
public:
    EMBED_TOOL();

    EMBED_TOOL( const std::string& aName );

    virtual ~EMBED_TOOL() = default;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int AddFile( const TOOL_EVENT& aEvent );

    int RemoveFile( const TOOL_EVENT& aEvent );

    std::vector<wxString> GetFileList();
protected:

    /// @copydoc TOOL_INTERACTIVE::setTransitions();
    void setTransitions() override;

private:
    EMBEDDED_FILES* m_files;
};

#endif /* EMBED_TOOL_H */
