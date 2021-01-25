// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_ACTION_HPP_INCLUDED
#define MOCK_ACTION_HPP_INCLUDED

#include "../config.hpp"
#include <functional>
#include <memory>
#include <type_traits>

namespace mock { namespace detail {
    template<typename Result, typename Signature>
    class action_base
    {
    private:
        typedef std::function<Signature> functor_type;
        typedef std::function<Result()> action_type;

    protected:
        // Meant to be subclassed and not be directly used
        // Non-relocatable (contained functions may wrap references/pointers which could be invalidated)
        action_base() = default;
        action_base(const action_base&) = delete;
        action_base(action_base&&) = delete;
        action_base& operator=(const action_base&) = delete;
        action_base& operator=(action_base&&) = delete;

    public:
        const functor_type& functor() const { return f_; }
        bool valid() const { return f_ || a_; }
        Result trigger() const { return a_(); }

        void calls(const functor_type& f)
        {
            if(!f)
                throw std::invalid_argument("null functor");
            f_ = f;
        }

        template<typename Exception>
        void throws(Exception e)
        {
            a_ = [e]() -> Result { throw e; };
        }

    protected:
        void set(const action_type& a) { a_ = a; }
        template<typename Y>
        void set(const std::reference_wrapper<Y>& r)
        {
            a_ = [r]() -> Result { return r.get(); };
        }

    private:
        functor_type f_;
        action_type a_;
    };

    /// Type erased value storage
    struct value
    {
        value() = default;
        value(const value&) = delete;
        value& operator=(const value&) = delete;
        virtual ~value() = default;
    };
    /// Actual value storage,
    /// holds an instance of T (stripped of reference qualifiers)
    template<typename T>
    struct value_imp : value
    {
        using type = std::remove_const_t<std::remove_reference_t<T>>;

        template<typename U>
        value_imp(U&& t) : t_(std::forward<U>(t))
        {}
        type t_;
    };

    template<typename Result, typename Signature>
    class action : public action_base<Result, Signature>
    {
    public:
        template<typename Value>
        void returns(const Value& v)
        {
            this->set(std::ref(store(v)));
        }
        template<typename Y>
        void returns(const std::reference_wrapper<Y>& r)
        {
            this->set(r);
        }

        template<typename Value>
        void moves(Value&& v)
        {
            auto vRef = std::ref(store(std::move(v)));
            this->set([vRef]() { return std::move(vRef.get()); });
        }

    private:
        template<typename T>
        typename value_imp<T>::type& store(T&& t)
        {
            v_ = std::make_unique<value_imp<T>>(std::forward<T>(t));
            return static_cast<value_imp<T>&>(*v_).t_;
        }
        template<typename T>
        std::remove_reference_t<Result>& store(T* t)
        {
            v_ = std::make_unique<value_imp<Result>>(t);
            return static_cast<value_imp<Result>&>(*v_).t_;
        }

        std::unique_ptr<value> v_;
    };

    template<typename Signature>
    class action<void, Signature> : public action_base<void, Signature>
    {
    public:
        action()
        {
            this->set([]() {});
        }
    };

}} // namespace mock::detail

#endif // MOCK_ACTION_HPP_INCLUDED
