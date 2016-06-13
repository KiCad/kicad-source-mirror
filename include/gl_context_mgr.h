/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#ifndef GL_CONTEXT_MANAGER_H
#define GL_CONTEXT_MANAGER_H

#include <wx/glcanvas.h>
#include <ki_mutex.h>
#include <map>

class GL_CONTEXT_MANAGER
{
public:
    /**
     * Function Get
     * returns the GL_CONTEXT_MANAGER instance (singleton).
     */
    static GL_CONTEXT_MANAGER& Get();

    /**
     * Function CreateCtx
     * creates a managed OpenGL context. It is assured that the created context is freed upon
     * exit.
     * See wxGLContext documentation for the parameters description.
     * @return Created OpenGL context.
     */
    wxGLContext* CreateCtx( wxGLCanvas* aCanvas, const wxGLContext* aOther = NULL );

    /**
     * Function DestroyCtx
     * destroys a managed OpenGL context. The context to be removed has to be created using
     * GL_CONTEXT_MANAGER::CreateCtx() first.
     * @param aContext is the OpenGL context to be destroyed. It will not be managed anymore.
     */
    void DestroyCtx( wxGLContext* aContext );

    /**
     * Function DeleteAll
     * destroys all managed OpenGL contexts. This method should be called in the
     * final deinitialization routine.
     */
    void DeleteAll();

    /**
     * Function LockCtx
     * sets a context as current and prevents other canvases from switching it.
     * Requires calling UnlockCtx() when there are no more GL calls for the context.
     * If another canvas has already locked a GL context, then the calling process is blocked.
     * @param aContext is the GL context to be bound.
     * @param aCanvas (optional) allows caller to bind the context to a non-parent canvas
     * (e.g. when a few canvases share a single GL context).
     */
    void LockCtx( wxGLContext* aContext, wxGLCanvas* aCanvas );

    /**
     * Function UnlockCtx
     * allows other canvases to bind an OpenGL context.
     * @param aContext is the currently bound context. It is only a check to assure the right
     * canvas wants to unlock GL context.
     */
    void UnlockCtx( wxGLContext* aContext );

private:
    ///> Map of GL contexts & their parent canvases.
    std::map<wxGLContext*, wxGLCanvas*> m_glContexts;

    ///> Currently bound GL context.
    wxGLContext* m_glCtx;

    ///> Lock to prevent unexpected GL context switching.
    MUTEX m_glCtxMutex;

    // Singleton
    GL_CONTEXT_MANAGER();
    GL_CONTEXT_MANAGER( const GL_CONTEXT_MANAGER& );
    void operator=( const GL_CONTEXT_MANAGER& );
};

#endif /* GL_CONTEXT_MANAGER_H */

