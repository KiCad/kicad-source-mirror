/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2014-2021 3Dconnexion.
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

/**
 * @file navlib_stub.c
 * @brief interface routines to the navlib library routines.
 */

/*-----------------------------------------------------------------------------
 * The module contains interface routines to the navlib library routines contained
 * in the associated Dynamic Link Library.  The DLL is loaded explicitly when
 * NlLoadLibrary is invoked.  When the DLL is loaded, the initialization routine
 * finds the addresses of the routines that it exposes and allows them to be used
 * in this code.
 */

#if _WIN32
/* windows */
#include <windows.h>
#include <errno.h>
#elif __APPLE__
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#endif

#ifndef EISCONN
#define EISCONN 113
#define ENOBUFS 119
#define ENODATA 120
#define EOPNOTSUPP 130
#endif

/* navlib */
#include <navlib/navlib.h>

/* library name */
#ifdef _WIN32
static const wchar_t* TheLibrary = L"TDxNavLib";
#elif __APPLE__
static const char* TheLibrary = "/Library/Frameworks/3DconnexionNavlib.framework/3DconnexionNavlib";
#endif

/* Names of functions contained in the framework used to find their addresses at load time */
static const char* cNlCreate = "NlCreate";
static const char* cNlClose = "NlClose";
static const char* cNlReadValue = "NlReadValue";
static const char* cNlWriteValue = "NlWriteValue";
static const char* cNlGetType = "NlGetType";

typedef long( __cdecl* PFN_NLCREATE )( nlHandle_t* pnh, const char* appname,
                                       const accessor_t accessors[], size_t accessor_count,
                                       nlCreateOptions_t const* options );
typedef long( __cdecl* PFN_NLCLOSE )( nlHandle_t nh );
typedef long( __cdecl* PFN_NLREADVALUE )( nlHandle_t nh, property_t name, value_t* value );
typedef long( __cdecl* PFN_NLWRITEVALUE )( nlHandle_t nh, property_t name, const value_t* value );
typedef propertyType_t( __cdecl* PFN_NLGETTYPE )( property_t name );

/* Function pointers to functions in DLL */
static PFN_NLCREATE     pfnNlCreate = NULL;
static PFN_NLCLOSE      pfnNlClose = NULL;
static PFN_NLREADVALUE  pfnNlReadValue = NULL;
static PFN_NLWRITEVALUE pfnNlWriteValue = NULL;
static PFN_NLGETTYPE    pfnNlGetType = NULL;

extern long NlErrorCode;

#if _WIN32

long NlLoadLibrary()
{
    long    error = 0;
    HMODULE h = LoadLibrary( TheLibrary );
    if( !h )
    {
        error = HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        /* load up the function pointer table */
        if( ( ( pfnNlCreate = (PFN_NLCREATE) GetProcAddress( h, cNlCreate ) ) == NULL )
            || ( ( pfnNlClose = (PFN_NLCLOSE) GetProcAddress( h, cNlClose ) ) == NULL )
            || ( ( pfnNlReadValue = (PFN_NLREADVALUE) GetProcAddress( h, cNlReadValue ) ) == NULL )
            || ( ( pfnNlWriteValue = (PFN_NLWRITEVALUE) GetProcAddress( h, cNlWriteValue ) )
                 == NULL )
            || ( ( pfnNlGetType = (PFN_NLGETTYPE) GetProcAddress( h, cNlGetType ) ) == NULL ) )
        {
            error = HRESULT_FROM_WIN32( GetLastError() );
            FreeLibrary( h );
            h = NULL;
        }
    }
    return error;
}

#elif __APPLE__

long NlLoadLibrary()
{
    long  error = 0;
    void* libHandle = dlopen( TheLibrary, RTLD_LAZY | RTLD_LOCAL );
    if( NULL == libHandle )
    {
        error = -1; // whatever error it's an error dlopen() does not set errno
#if 0
        fprintf( stderr, "Error: Failed to open library \"%s\"! Error: %s!\n", TheLibrary,
                 dlerror() );
#endif
    }
    else
    {
        /* load up the function pointer table */
        if( ( ( pfnNlCreate = (PFN_NLCREATE) dlsym( libHandle, cNlCreate ) ) == NULL )
            || ( ( pfnNlClose = (PFN_NLCLOSE) dlsym( libHandle, cNlClose ) ) == NULL )
            || ( ( pfnNlReadValue = (PFN_NLREADVALUE) dlsym( libHandle, cNlReadValue ) ) == NULL )
            || ( ( pfnNlWriteValue = (PFN_NLWRITEVALUE) dlsym( libHandle, cNlWriteValue ) )
                 == NULL )
            || ( ( pfnNlGetType = (PFN_NLGETTYPE) dlsym( libHandle, cNlGetType ) ) == NULL ) )
        {
            error = -2; // whatever error it is - it's an error dlsym() does not set errno
#if 0
            fprintf( stderr, "Error: Failed to fetch symbols from \"%s\"! Error: %s!\n", TheLibrary,
                     dlerror() );
#endif
            dlclose( libHandle );
            libHandle = NULL;
        }
    }
    return error;
}

#else

long NlLoadLibrary()
{
    return EOPNOTSUPP;
}

#endif

long __cdecl NlCreate( nlHandle_t* pnh, const char* appname, const accessor_t accessors[],
                       size_t accessor_count, const nlCreateOptions_t* options )
{
    if( pfnNlCreate )
    {
        return pfnNlCreate( pnh, appname, accessors, accessor_count, options );
    }

    return NlErrorCode;
}

long __cdecl NlClose( nlHandle_t nh )
{
    if( pfnNlClose )
    {
        return pfnNlClose( nh );
    }

    return NlErrorCode;
}

long __cdecl NlReadValue( nlHandle_t nh, property_t name, value_t* value )
{
    if( pfnNlReadValue )
    {
        return pfnNlReadValue( nh, name, value );
    }

    return NlErrorCode;
}

long __cdecl NlWriteValue( nlHandle_t nh, property_t name, const value_t* value )
{
    if( pfnNlWriteValue )
    {
        return pfnNlWriteValue( nh, name, value );
    }

    return NlErrorCode;
}

propertyType_t __cdecl NlGetType( property_t name )
{
    if( pfnNlGetType )
    {
        return pfnNlGetType( name );
    }

    return unknown_type;
}
