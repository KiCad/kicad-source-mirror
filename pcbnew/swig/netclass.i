/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/*

NETCLASS is not exposed/generated via SWIG but rather class NETCLASSPTR is. This
is because we can never have a simple pointer to a NETCLASS, only can we have a
NETCLASSPTR. So we cannot build a python wrapper without using NETCLASSPTR to do
the pointing.

*/

%ignore NETCLASS;               // no code generation for it
%include class_netclass.h


/*

Propagate important member functions from NETCLASS into SWIG's NETCLASSPTR. Do
this by dereferencing the pointer to the NETCLASSPTR proxy, then dereference the
std::share_ptr to get to the actual NETCLASS::member. Complete the member
function set if you see something missing from class NETCLASS that you need.

*/

//%extend NETCLASSPTR   // would have thought the typedef in header above was golden here.
%extend std::shared_ptr<NETCLASS>
{
public:
    std::shared_ptr<NETCLASS>( std::string name )     { return new std::shared_ptr<NETCLASS>( new NETCLASS(name) ); }

    STRINGSET&  NetNames()                  { return (*self)->NetNames(); }

    const wxString& GetName()               { return (*self)->GetName(); }

    unsigned GetCount() const               { return (*self)->GetCount(); }

    const wxString& GetDescription() const          { return (*self)->GetDescription(); }
    void    SetDescription( const wxString& aDesc ) { (*self)->SetDescription( aDesc ); }

    int     GetClearance() const            { return (*self)->GetClearance(); }
    void    SetClearance( int aClearance )  { (*self)->SetClearance( aClearance ); }

    int     GetTrackWidth() const           { return (*self)->GetTrackWidth(); }
    void    SetTrackWidth( int aWidth )     { (*self)->SetTrackWidth( aWidth ); }

    int     GetViaDiameter() const          { return (*self)->GetViaDiameter(); }
    void    SetViaDiameter( int aDia )      { (*self)->SetViaDiameter( aDia ); }

    int     GetViaDrill() const             { return (*self)->GetViaDrill(); }
    void    SetViaDrill( int aSize )        { (*self)->SetViaDrill( aSize ); }

    int     GetuViaDiameter() const         { return (*self)->GetuViaDiameter(); }
    void    SetuViaDiameter( int aSize )    { (*self)->SetuViaDiameter( aSize ); }

    int     GetuViaDrill() const            { return (*self)->GetuViaDrill(); }
    void    SetuViaDrill( int aSize )       { (*self)->SetuViaDrill( aSize ); }

    int     GetDiffPairWidth() const        { return (*self)->GetDiffPairWidth(); }
    void    SetDiffPairWidth( int aSize )   { (*self)->SetDiffPairWidth( aSize ); }

    int     GetDiffPairGap() const          { return (*self)->GetDiffPairGap(); }
    void    SetDiffPairGap( int aSize )     { (*self)->SetDiffPairGap( aSize ); }
};


%{
#include <class_netclass.h>
%}
