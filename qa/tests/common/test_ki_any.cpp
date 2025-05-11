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

// Tests taken from GCC:
// Copyright (C) 2014-2024 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <type_traits>

#include <ki_any.h>

using ki::any;
using ki::any_cast;

//////////////////////////////////////////
// Exception hierarchy
//////////////////////////////////////////
///
static_assert(std::is_base_of<std::bad_cast, ki::bad_any_cast>::value,
          "ki::bad_any_cast must derive from std::bad_cast");

//////////////////////////////////////////
// Type requirements
//////////////////////////////////////////

static_assert(std::is_assignable<any&, int>::value);
static_assert(!std::is_assignable<any&, std::unique_ptr<int>>::value);
static_assert(std::is_constructible<any, int>::value);
static_assert(!std::is_constructible<any, std::unique_ptr<int>>::value);
static_assert(!std::is_assignable<any&, const std::unique_ptr<int>&>::value);
static_assert(!std::is_constructible<any&, const std::unique_ptr<int>&>::value);
static_assert(!std::is_assignable<any&, std::unique_ptr<int>&>::value);
static_assert(!std::is_constructible<any&, std::unique_ptr<int>&>::value);

struct NoDefaultCtor
{
    NoDefaultCtor() = delete;
};

static_assert(!std::is_constructible_v<any,
          std::in_place_type_t<NoDefaultCtor>>);

static_assert(!std::is_constructible_v<any,
          std::in_place_type_t<NoDefaultCtor>&>);

static_assert(!std::is_constructible_v<any,
          std::in_place_type_t<NoDefaultCtor>&&>);

static_assert(!std::is_constructible_v<any,
          const std::in_place_type_t<NoDefaultCtor>&>);

static_assert(!std::is_constructible_v<any,
          const std::in_place_type_t<NoDefaultCtor>&&>);

static_assert( std::is_copy_constructible_v<std::tuple<any>> );

struct A {
    A(const A&) = default;
    explicit A(any value);
};
static_assert(std::is_copy_constructible_v<A>);

BOOST_AUTO_TEST_SUITE( ki_any )

struct combined {
    std::vector<int> v;
    std::tuple<int, int> t;
    template<class... Args>
    combined(std::initializer_list<int> il, Args&&... args)
      : v(il), t(std::forward<Args>(args)...)
    {
    }
};


BOOST_AUTO_TEST_CASE( AnyCast_1 )
{
    using std::string;
    using std::strcmp;

    any x(5);                                   // x holds int
    BOOST_CHECK(any_cast<int>(x) == 5);              // cast to value
    any_cast<int&>(x) = 10;                     // cast to reference
    BOOST_CHECK(any_cast<int>(x) == 10);

    x = "Meow";                                 // x holds const char*
    BOOST_CHECK(strcmp(any_cast<const char*>(x), "Meow") == 0);
    any_cast<const char*&>(x) = "Harry";
    BOOST_CHECK(strcmp(any_cast<const char*>(x), "Harry") == 0);

    x = string("Meow");                         // x holds string
    string s, s2("Jane");
    s = std::move(any_cast<string&>(x));             // move from any
    BOOST_CHECK(s == "Meow");
    any_cast<string&>(x) = std::move(s2);            // move to any
    BOOST_CHECK(any_cast<const string&>(x) == "Jane");

    string cat("Meow");
    const any y(cat);                           // const y holds string
    BOOST_CHECK(any_cast<const string&>(y) == cat);
}


BOOST_AUTO_TEST_CASE( AnyCast_2 )
{
    using ki::bad_any_cast;
    any x(1);
    auto p = any_cast<double>(&x);
    BOOST_CHECK(p == nullptr);

    x = 1.0;
    p = any_cast<double>(&x);
    BOOST_CHECK(p != nullptr);

    x = any();
    p = any_cast<double>(&x);
    BOOST_CHECK(p == nullptr);

    try {
        any_cast<double>(x);
        BOOST_CHECK(false);
    } catch (const bad_any_cast&) {
    }
}

static int move_count = 0;

BOOST_AUTO_TEST_CASE( AnyCast_3 )
{
    struct MoveEnabled
    {
        MoveEnabled(MoveEnabled&&)
        {
            ++move_count;
        }
        MoveEnabled() = default;
        MoveEnabled(const MoveEnabled&) = default;
    };
    MoveEnabled m;
    MoveEnabled m2 = any_cast<MoveEnabled>(any(m));
    BOOST_CHECK(move_count == 1);
    MoveEnabled&& m3 = any_cast<MoveEnabled&&>(any(m));
    BOOST_CHECK(move_count == 1);
}


BOOST_AUTO_TEST_CASE( AnyCast_4 )
{
    struct ExplicitCopy
    {
        ExplicitCopy() = default;
        explicit ExplicitCopy(const ExplicitCopy&) = default;
    };
    any x = ExplicitCopy();
    ExplicitCopy ec{any_cast<ExplicitCopy>(x)};
    ExplicitCopy ec2{any_cast<ExplicitCopy>(std::move(x))};
}


BOOST_AUTO_TEST_CASE( AnyCast_5 )
{
    struct noncopyable {
        noncopyable(noncopyable const&) = delete;
    };

    any a;
    auto p = any_cast<noncopyable>(&a);
    BOOST_CHECK( p == nullptr );
}


BOOST_AUTO_TEST_CASE( AnyCast_6 )
{
    // The contained value of a std::any is always an object type,
    // but any_cast does not forbid checking for function types.

    any a(1);
    void (*p1)() = any_cast<void()>(&a);
    BOOST_CHECK( p1 == nullptr );
    int (*p2)(int) = any_cast<int(int)>(&a);
    BOOST_CHECK( p2 == nullptr );
    int (*p3)() = any_cast<int()>(&std::as_const(a));
    BOOST_CHECK( p3 == nullptr );

    try {
        any_cast<int(&)()>(a);
        BOOST_CHECK( false );
    } catch (const ki::bad_any_cast&) {
    }

    try {
        any_cast<int(&)()>(std::move(a));
        BOOST_CHECK( false );
    } catch (const ki::bad_any_cast&) {
    }

    try {
        any_cast<int(&)()>(std::as_const(a));
        BOOST_CHECK( false );
    } catch (const ki::bad_any_cast&) {
    }
}


BOOST_AUTO_TEST_CASE( AnyCast_7 )
{
    int arr[3];
    any a(arr);

    BOOST_CHECK( a.type() == typeid(int*) );	// contained value is decayed
    int (*p1)[3] = any_cast<int[3]>(&a);
    BOOST_CHECK( a.type() != typeid(int[3]) ); // so any_cast should return nullptr
    BOOST_CHECK( p1 == nullptr );
    int (*p2)[] = any_cast<int[]>(&a);
    BOOST_CHECK( a.type() != typeid(int[]) );	// so any_cast should return nullptr
    BOOST_CHECK( p2 == nullptr );
    const int (*p3)[] = any_cast<int[]>(&std::as_const(a));
    BOOST_CHECK( p3 == nullptr );
}


struct LocationAware
{
    LocationAware() { }
    ~LocationAware() { BOOST_CHECK(self == this); }
    LocationAware(const LocationAware&) { }
    LocationAware& operator=(const LocationAware&) { return *this; }
    LocationAware(LocationAware&&) noexcept { }
    LocationAware& operator=(LocationAware&&) noexcept { return *this; }

    void* const self = this;
};
static_assert(std::is_nothrow_move_constructible<LocationAware>::value, "");
static_assert(!std::is_trivially_copyable<LocationAware>::value, "");


BOOST_AUTO_TEST_CASE( NonTrivialType_1 )
{
    LocationAware l;
    any a = l;
}


BOOST_AUTO_TEST_CASE( NonTrivialType_2 )
{
    LocationAware l;
    any a = l;
    any b = a;
    {
        any tmp = std::move(a);
        a = std::move(b);
        b = std::move(tmp);
    }
}


BOOST_AUTO_TEST_CASE( NonTrivialType_3 )
{
    LocationAware l;
    any a = l;
    any b = a;
    swap(a, b);
}


BOOST_AUTO_TEST_CASE( MakeAny )
{
    const int i = 42;
    auto o = ki::make_any<int>(i);
    int& i2 = any_cast<int&>(o);
    BOOST_CHECK( i2 == 42 );
    BOOST_CHECK( &i2 != &i );
    auto o2 = ki::make_any<std::tuple<int, int>>(1, 2);
    std::tuple<int, int>& t = any_cast<std::tuple<int, int>&>(o2);
    BOOST_CHECK( std::get<0>(t) == 1 && std::get<1>(t) == 2);
    auto o3 = ki::make_any<std::vector<int>>({42, 666});
    std::vector<int>& v = any_cast<std::vector<int>&>(o3);
    BOOST_CHECK(v[0] == 42 && v[1] == 666);
    auto o4 = ki::make_any<combined>({42, 666});
    combined& c = any_cast<combined&>(o4);
    BOOST_CHECK(c.v[0] == 42 && c.v[1] == 666
       && std::get<0>(c.t) == 0 && std::get<1>(c.t) == 0 );
    auto o5 = ki::make_any<combined>({1, 2}, 3, 4);
    combined& c2 = any_cast<combined&>(o5);
    BOOST_CHECK(c2.v[0] == 1 && c2.v[1] == 2
       && std::get<0>(c2.t) == 3 && std::get<1>(c2.t) == 4 );
}


BOOST_AUTO_TEST_CASE( TypeObserver )
{
    any x;
    BOOST_CHECK( x.type() == typeid(void) );
    x = 1;
    BOOST_CHECK( x.type() == typeid(int) );
    x = any();
    BOOST_CHECK( x.type() == typeid(void) );
}


BOOST_AUTO_TEST_CASE( Swap )
{
    any x(1);
    any y;
    swap(x, y);
    BOOST_CHECK( !x.has_value() );
    BOOST_CHECK( y.has_value() );
}


BOOST_AUTO_TEST_CASE( Modifiers )
{
    any x(1);
    any y;
    x.swap(y);
    BOOST_CHECK( !x.has_value() );
    BOOST_CHECK( y.has_value() );
    x.swap(y);
    BOOST_CHECK( x.has_value() );
    BOOST_CHECK( !y.has_value() );

    x.reset();
    BOOST_CHECK( !x.has_value() );
}


BOOST_AUTO_TEST_CASE( Construction_InPlace_1 )
{
    const int i = 42;
    any o(std::in_place_type<int>, i);
    int& i2 = any_cast<int&>(o);
    BOOST_CHECK( i2 == 42 );
    BOOST_CHECK( &i2 != &i );
    any o2(std::in_place_type<std::tuple<int, int>>, 1, 2);
    std::tuple<int, int>& t = any_cast<std::tuple<int, int>&>(o2);
    BOOST_CHECK( std::get<0>(t) == 1 && std::get<1>(t) == 2);
    any o3(std::in_place_type<std::vector<int>>, {42, 666});
    std::vector<int>& v = any_cast<std::vector<int>&>(o3);
    BOOST_CHECK(v[0] == 42 && v[1] == 666);
    any o4(std::in_place_type<combined>, {42, 666});
    combined& c = any_cast<combined&>(o4);
    BOOST_CHECK(c.v[0] == 42 && c.v[1] == 666
       && std::get<0>(c.t) == 0 && std::get<1>(c.t) == 0 );
    any o5(std::in_place_type<combined>, {1, 2}, 3, 4);
    combined& c2 = any_cast<combined&>(o5);
    BOOST_CHECK(c2.v[0] == 1 && c2.v[1] == 2
       && std::get<0>(c2.t) == 3 && std::get<1>(c2.t) == 4 );
    any o6(std::in_place_type<int&>, i);
    BOOST_CHECK(o6.type() == o.type());
    any o7(std::in_place_type<void()>, nullptr);
    any o8(std::in_place_type<void(*)()>, nullptr);
    BOOST_CHECK(o7.type() == o8.type());
    any o9(std::in_place_type<char(&)[42]>, nullptr);
    any o10(std::in_place_type<char*>, nullptr);
    BOOST_CHECK(o9.type() == o10.type());
}

bool moved = false;
bool copied = false;

struct X
{
    X() = default;
    X(const X&) { copied = true; }
    X(X&&) { moved = true; }
};

struct X2
{
    X2() = default;
    X2(const X2&) { copied = true; }
    X2(X2&&) noexcept { moved = true; }
};


BOOST_AUTO_TEST_CASE( Construction_Basic_1 )
{
    moved = false;
    X x;
    any a1(x);
    BOOST_CHECK(moved == false);
    any a2(std::move(x));
    BOOST_CHECK(moved == true);
}


BOOST_AUTO_TEST_CASE( Construction_Basic_2 )
{
    moved = false;
    X x;
    any a1(x);
    BOOST_CHECK(moved == false);
    copied = false;
    any a2(std::move(a1));
    BOOST_CHECK(copied == false);
}


BOOST_AUTO_TEST_CASE( Construction_Basic_3 )
{
    moved = false;
    X2 x;
    any a1(x);
    BOOST_CHECK(moved == false);
    copied = false;
    any a2(std::move(a1));
    BOOST_CHECK(copied == false);
    BOOST_CHECK(moved == true);
}


BOOST_AUTO_TEST_CASE( Construction_Basic_4)
{
    any x;
    BOOST_CHECK( !x.has_value() );

    any y(x);
    BOOST_CHECK( !x.has_value() );
    BOOST_CHECK( !y.has_value() );

    any z(std::move(y));
    BOOST_CHECK( !y.has_value() );
    BOOST_CHECK( !z.has_value() );
}


BOOST_AUTO_TEST_CASE( Construction_Basic_5)
{
    any x(1);
    BOOST_CHECK( x.has_value() );

    any y(x);
    BOOST_CHECK( x.has_value() );
    BOOST_CHECK( y.has_value() );

    any z(std::move(y));
    BOOST_CHECK( !y.has_value() );
    BOOST_CHECK( z.has_value() );
}


BOOST_AUTO_TEST_CASE( Construction_InPlace_2 )
{
    auto a = any(std::in_place_type<any>, 5);
    BOOST_CHECK( any_cast<int>(any_cast<any>(a)) == 5 );
}


BOOST_AUTO_TEST_CASE( Construction_Pair )
{
    any p = std::pair<any, any>(1, 1);
    auto pt = any_cast<std::pair<any, any>>(p);
    BOOST_CHECK( any_cast<int>(pt.first) == 1 );
    BOOST_CHECK( any_cast<int>(pt.second) == 1 );

    any t = std::tuple<any>(1);
    auto tt = any_cast<std::tuple<any>>(t);
    BOOST_CHECK( any_cast<int>(std::get<0>(tt)) == 1 );
}


// Alignment requiremnts of this type prevent it being stored in 'any'
struct alignas(2 * alignof(void*)) X3 { };

bool
stored_internally(void* obj, const ki::any& a)
{
    std::uintptr_t a_addr = reinterpret_cast<std::uintptr_t>(&a);
    std::uintptr_t a_end = a_addr + sizeof(a);
    std::uintptr_t obj_addr = reinterpret_cast<std::uintptr_t>(obj);
    return (a_addr <= obj_addr) && (obj_addr < a_end);
}


BOOST_AUTO_TEST_CASE( Construction_Alignment )
{
    any a = X3{};
    X3& x = any_cast<X3&>(a);
    BOOST_CHECK( !stored_internally(&x, a) );

    a = 'X';
    char& c = any_cast<char&>(a);
    BOOST_CHECK( stored_internally(&c, a) );
}

struct wrapper
{
    wrapper() = default;

    wrapper(const any& t);

    wrapper(const wrapper& w);

    auto& operator=(const any& t);

    auto& operator=(const wrapper& w)
    {
        value = w.value;
        return *this;
    }

    any value;
};

BOOST_AUTO_TEST_CASE( Construction_Wrapper )
{
    wrapper a, b;
    a = b;
}

// Following tests commented out until Apple Clang build version >= 16
/*
struct aggressive_aggregate
{
    int a;
    int b;
};

BOOST_AUTO_TEST_CASE( Construction_Aggregate_1 )
{
    any x{std::in_place_type<aggressive_aggregate>, 1, 2};
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).a == 1);
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).b == 2);
    any y{std::in_place_type<aggressive_aggregate>, 1};
    BOOST_CHECK(any_cast<aggressive_aggregate>(y).a == 1);
    BOOST_CHECK(any_cast<aggressive_aggregate>(y).b == 0);
    any z{std::in_place_type<aggressive_aggregate>};
    BOOST_CHECK(any_cast<aggressive_aggregate>(z).a == 0);
    BOOST_CHECK(any_cast<aggressive_aggregate>(z).b == 0);
}

BOOST_AUTO_TEST_CASE( Construction_Aggregate_2 )
{
    any x;
    x.emplace<aggressive_aggregate>(1, 2);
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).a == 1);
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).b == 2);
    x.emplace<aggressive_aggregate>(1);
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).a == 1);
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).b == 0);
    x.emplace<aggressive_aggregate>();
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).a == 0);
    BOOST_CHECK(any_cast<aggressive_aggregate>(x).b == 0);
}

*/

std::set<const void*> live_objects;

struct A {
    A() { live_objects.insert(this); }
    ~A() { live_objects.erase(this); }
    A(const A& a) { BOOST_CHECK(live_objects.count(&a)); live_objects.insert(this); }
};

BOOST_AUTO_TEST_CASE( Assign_1 )
{
    any a;
    a = a;
    BOOST_CHECK( !a.has_value() );

    a = A{};
    a = a;
    BOOST_CHECK( a.has_value() );

    a.reset();
    BOOST_CHECK( live_objects.empty() );
}


BOOST_AUTO_TEST_CASE( Assign_2 )
{
    struct X {
        any a;
    };

    X x;
    std::swap(x, x); // results in "self-move-assignment" of X::a
    BOOST_CHECK( !x.a.has_value() );

    x.a = A{};
    std::swap(x, x); // results in "self-move-assignment" of X::a
    BOOST_CHECK( x.a.has_value() );

    x.a.reset();
    BOOST_CHECK( live_objects.empty() );
}


BOOST_AUTO_TEST_CASE( Assign_3 )
{
    any a;
    a.swap(a);
    BOOST_CHECK( !a.has_value() );

    a = A{};
    a.swap(a);
    BOOST_CHECK( a.has_value() );

    a.reset();
    BOOST_CHECK( live_objects.empty() );
}


BOOST_AUTO_TEST_CASE( Assign_4 )
{
    moved = false;
    X x;
    any a1;
    a1 = x;
    BOOST_CHECK(moved == false);
    any a2;
    copied = false;
    a2 = std::move(x);
    BOOST_CHECK(moved == true);
    BOOST_CHECK(copied == false);
}


BOOST_AUTO_TEST_CASE( Assign_5 )
{
    moved = false;
    X x;
    any a1;
    a1 = x;
    BOOST_CHECK(moved == false);
    any a2;
    copied = false;
    a2 = std::move(a1);
    BOOST_CHECK(moved == false);
    BOOST_CHECK(copied == false);
}


BOOST_AUTO_TEST_CASE( Assign_6 )
{
    moved = false;
    X2 x;
    any a1;
    a1 = x;
    BOOST_CHECK(copied && moved);
    any a2;
    moved = false;
    copied = false;
    a2 = std::move(a1);
    BOOST_CHECK(moved == true);
    BOOST_CHECK(copied == false);
}

BOOST_AUTO_TEST_CASE( Assign_7 )
{
    any x;
    any y;
    y = x;
    BOOST_CHECK( !x.has_value() );
    BOOST_CHECK( !y.has_value() );

    y = std::move(x);
    BOOST_CHECK( !x.has_value() );
    BOOST_CHECK( !y.has_value() );
}

BOOST_AUTO_TEST_CASE( Assign_8 )
{
    any x(1);
    any y;
    y = x;
    BOOST_CHECK( x.has_value() );
    BOOST_CHECK( y.has_value() );

    x = std::move(y);
    BOOST_CHECK( x.has_value() );
    BOOST_CHECK( !y.has_value() );

    x = y;
    BOOST_CHECK( !x.has_value() );
    BOOST_CHECK( !y.has_value() );
}


bool should_throw = false;
struct Bad
{
    Bad() = default;
    Bad(const Bad&) {if (should_throw) throw 666;}
};

struct Bad2
{
    Bad2() = default;
    Bad2(const Bad2&) {if (should_throw) throw 666;}
    Bad2(Bad2&&) noexcept {}
};

int del_count = 0;
struct Good
{
    Good() = default;
    Good(const Good&) = default;
    Good(Good&&) = default;
    ~Good() {++del_count;}
};

BOOST_AUTO_TEST_CASE( Exceptions )
{
    any a1 = Good();
    del_count = 0;
    try {
        Bad b;
        any a2 = b;
        should_throw = true;
        a1 = a2;
    } catch (...) {
        auto x = any_cast<Good>(a1);
        BOOST_CHECK( del_count == 0 );
        BOOST_CHECK( a1.has_value() );
        any_cast<Good>(a1);
    }
    any a3 = Good();
    del_count = 0;
    try {
        Bad2 b;
        any a4 = b;
        should_throw = true;
        a3 = a4;
    } catch (...) {
        auto x = any_cast<Good>(a1);
        BOOST_CHECK( del_count == 0 );
        BOOST_CHECK( a1.has_value() );
        any_cast<Good>(a1);
    }
}


BOOST_AUTO_TEST_CASE( Emplace_1 )
{
    const int i = 42;
    any o;
    o.emplace<int>(i);
    int& i2 = any_cast<int&>(o);
    BOOST_CHECK( i2 == 42 );
    BOOST_CHECK( &i2 != &i );
    any o2;
    o2.emplace<std::tuple<int, int>>(1, 2);
    std::tuple<int, int>& t = any_cast<std::tuple<int, int>&>(o2);
    BOOST_CHECK( std::get<0>(t) == 1 && std::get<1>(t) == 2);
    any o3;
    o3.emplace<std::vector<int>>({42, 666});
    std::vector<int>& v = any_cast<std::vector<int>&>(o3);
    BOOST_CHECK(v[0] == 42 && v[1] == 666);
    any o4;
    o4.emplace<combined>({42, 666});
    combined& c = any_cast<combined&>(o4);
    BOOST_CHECK(c.v[0] == 42 && c.v[1] == 666
       && std::get<0>(c.t) == 0 && std::get<1>(c.t) == 0 );
    any o5;
    o5.emplace<combined>({1, 2}, 3, 4);
    combined& c2 = any_cast<combined&>(o5);
    BOOST_CHECK(c2.v[0] == 1 && c2.v[1] == 2
       && std::get<0>(c2.t) == 3 && std::get<1>(c2.t) == 4 );
    any o6;
    o6.emplace<const int&>(i);
    BOOST_CHECK(o6.type() == o.type());
    any o7;
    o7.emplace<void()>(nullptr);
    any o8;
    o8.emplace<void(*)()>(nullptr);
    BOOST_CHECK(o7.type() == o8.type());
    any o9;
    o9.emplace<char(&)[42]>(nullptr);
    any o10;
    o10.emplace<char*>(nullptr);
    BOOST_CHECK(o9.type() == o10.type());
    any o11;
    BOOST_CHECK(&o11.emplace<int>(42) == &any_cast<int&>(o11));
    BOOST_CHECK(&o11.emplace<std::vector<int>>({1,2,3}) ==
       &any_cast<std::vector<int>&>(o11));
}



struct X_e
{
    X_e() = default;
    X_e(const X_e&) { copied = true; }
    X_e(X_e&&) { moved = true; }
};

struct X2_e
{
    X2_e() = default;
    X2_e(const X2_e&) { copied = true; }
    X2_e(X2_e&&) noexcept { moved = true; }
};


BOOST_AUTO_TEST_SUITE_END()
