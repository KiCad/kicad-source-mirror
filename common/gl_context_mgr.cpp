/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

GL_CONTEXT_MANAGER& GL_CONTEXT_MANAGER::Get()
{
    static GL_CONTEXT_MANAGER instance;

    return instance;
}

wxGLContext* GL_CONTEXT_MANAGER::CreateCtx( wxGLCanvas* aCanvas, const wxGLContext* aOther )
{
    wxGLContext* context = new wxGLContext( aCanvas, aOther );
    assert( context /* && context->IsOK() */ );     // IsOK() is available in wx3.1+
    assert( m_glContexts.count( context ) == 0 );
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
        assert( false );
    }

    if( m_glCtx == aContext )
    {
        m_glCtx = NULL;
    }
}


void GL_CONTEXT_MANAGER::DeleteAll()
{
    for( auto& ctx : m_glContexts )
        delete ctx.first;

    m_glContexts.clear();
}


void GL_CONTEXT_MANAGER::LockCtx( wxGLContext* aContext, wxGLCanvas* aCanvas )
{
    assert( aCanvas || m_glContexts.count( aContext ) > 0 );

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
    assert( m_glContexts.count( aContext ) > 0 );

    if( m_glCtx == aContext )
    {
        m_glCtxMutex.unlock();
        m_glCtx = NULL;
    }
    else
    {
        wxLogDebug( "Trying to unlock GL context mutex from a wrong context" );
    }
}


GL_CONTEXT_MANAGER::GL_CONTEXT_MANAGER()
    : m_glCtx( NULL )
{
}

