/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <gl_context_mgr.h>
#include <wx/debug.h>


GL_CONTEXT_MANAGER& GL_CONTEXT_MANAGER::Get()
{
    static GL_CONTEXT_MANAGER instance;

    return instance;
}


wxGLContext* GL_CONTEXT_MANAGER::CreateCtx( wxGLCanvas* aCanvas, const wxGLContext* aOther )
{
    wxGLContext* context = new wxGLContext( aCanvas, aOther );
    wxCHECK( context, nullptr );

#if wxCHECK_VERSION( 3, 1, 0 )
    if( !context->IsOK() )
    {
        delete context;
        return nullptr;
    }
#endif /* wxCHECK_VERSION( 3, 1, 0 ) */

    m_glContexts.insert( std::make_pair( context, aCanvas ) );

    return context;
}


void GL_CONTEXT_MANAGER::DestroyCtx( wxGLContext* aContext )
{
    if( m_glContexts.count( aContext ) )
    {
        m_glContexts.erase( aContext );
        delete aContext;
    }
    else
    {
        // Do not delete unknown GL contexts
        wxFAIL;
    }

    if( m_glCtx == aContext )
        m_glCtx = nullptr;
}


void GL_CONTEXT_MANAGER::DeleteAll()
{
    m_glCtxMutex.lock();

    for( auto& ctx : m_glContexts )
        delete ctx.first;

    m_glContexts.clear();
    m_glCtx = nullptr;
    m_glCtxMutex.unlock();
}


void GL_CONTEXT_MANAGER::LockCtx( wxGLContext* aContext, wxGLCanvas* aCanvas )
{
    wxCHECK( aCanvas || m_glContexts.count( aContext ) > 0, /* void */ );

    m_glCtxMutex.lock();
    wxGLCanvas* canvas = aCanvas ? aCanvas : m_glContexts.at( aContext );

    // Prevent assertion failure in wxGLContext::SetCurrent during GAL teardown
#ifdef __WXGTK__
    if( canvas->GetXWindow() )
#endif
    {
        canvas->SetCurrent( *aContext );
    }

    m_glCtx = aContext;
}


void GL_CONTEXT_MANAGER::UnlockCtx( wxGLContext* aContext )
{
    wxCHECK( m_glContexts.count( aContext ) > 0, /* void */ );

    if( m_glCtx == aContext )
    {
        m_glCtxMutex.unlock();
        m_glCtx = nullptr;
    }
    else
    {
        wxFAIL_MSG( wxString::Format( "Trying to unlock GL context mutex from "
                    "a wrong context: aContext %p m_glCtx %p", aContext, m_glCtx ) );
    }
}


GL_CONTEXT_MANAGER::GL_CONTEXT_MANAGER()
    : m_glCtx( nullptr )
{
}

