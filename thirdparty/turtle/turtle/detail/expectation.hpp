// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_EXPECTATION_HPP_INCLUDED
#define MOCK_EXPECTATION_HPP_INCLUDED

#include "../matcher.hpp"
#include "../sequence.hpp"
#include "action.hpp"
#include "invocation.hpp"
#include "matcher_base.hpp"
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

namespace mock { namespace detail {
    template<typename... Args>
    class default_matcher : public matcher_base<Args...>
    {
    private:
        bool operator()(ref_arg_t<Args>...) override { return true; }
        void serialize(std::ostream& s) const override
        {
            constexpr auto arity = sizeof...(Args);
            for(unsigned i = 0; i < arity; ++i)
            {
                if(i)
                    s << ", ";
                s << "any";
            }
        }
    };

    template<typename ConstraintPack, typename... Args>
    class single_matcher;

    template<typename... Constraints, typename... Args>
    class single_matcher<void(Constraints...), Args...> : public matcher_base<Args...>
    {
        static_assert(sizeof...(Args) > 0, "This class is only useful for functions with arguments");
        static_assert(sizeof...(Constraints) == sizeof...(Args), "Need exactly 1 constraint per argument");

    public:
        single_matcher(Constraints... constraints) : matchers_(matcher<Args, Constraints>(constraints)...) {}

    private:
        template<std::size_t... I>
        bool is_valid_impl(std::index_sequence<I...>, ref_arg_t<Args>... t)
        {
            using expander = bool[];
            bool result = true;
            (void)expander{ result &= std::get<I>(matchers_)(static_cast<ref_arg_t<Args>>(t))... };
            return result;
        }
        bool operator()(ref_arg_t<Args>... t) override
        {
            return is_valid_impl(std::make_index_sequence<sizeof...(Args)>{}, static_cast<ref_arg_t<Args>>(t)...);
        }
        template<std::size_t... I>
        void serialize_impl(std::index_sequence<I...>, std::ostream& s) const
        {
            using expander = int[];
            s << std::get<0>(matchers_);
            (void)expander{ 0, (s << ", " << std::get<I + 1>(matchers_), 0)... };
        }
        void serialize(std::ostream& s) const override
        {
            serialize_impl(std::make_index_sequence<sizeof...(Args) - 1>{}, s);
        }

    private:
        std::tuple<matcher<Args, Constraints>...> matchers_;
    };

    template<typename F, typename... Args>
    class multi_matcher : public matcher_base<Args...>
    {
        static_assert(sizeof...(Args) > 0, "This class is only useful for functions with arguments");

    public:
        multi_matcher(const F& f) : f_(f) {}

    private:
        bool operator()(ref_arg_t<Args>... t) override { return f_(static_cast<ref_arg_t<Args>>(t)...); }
        void serialize(std::ostream& s) const override { s << mock::format(f_); }

    private:
        F f_;
    };

    template<typename Signature>
    class expectation;

    template<typename R, typename... Args>
    class expectation<R(Args...)> : public action<R, R(Args...)>
    {
        static constexpr std::size_t arity = sizeof...(Args);

    public:
        expectation() : expectation("unknown location", 0) {}
        expectation(const char* file, int line)
            : invocation_(std::make_unique<unlimited>()), matcher_(std::make_unique<default_matcher<Args...>>()),
              file_(file), line_(line)
        {}

        expectation(expectation&&) = default;
        expectation(expectation const&) = default;
        expectation& operator=(expectation&&) = default;
        expectation& operator=(expectation const&) = default;

        ~expectation()
        {
            for(auto& sequence : sequences_)
                sequence->remove(this);
        }

        void invoke(std::unique_ptr<invocation> i) { invocation_ = std::move(i); }

        template<typename... Constraints>
        std::enable_if_t<(arity > 0u) && sizeof...(Constraints) == arity> with(Constraints... c)
        {
            matcher_ = std::make_unique<single_matcher<void(Constraints...), Args...>>(c...);
        }
        template<typename Constraint, std::size_t Arity = arity>
        std::enable_if_t<(Arity > 1u)> with(const Constraint& c)
        {
            matcher_ = std::make_unique<multi_matcher<Constraint, Args...>>(c);
        }

        void add(sequence& s)
        {
            s.impl_->add(this);
            sequences_.push_back(s.impl_);
        }

        bool verify() const { return invocation_->verify(); }

        bool is_valid(ref_arg_t<Args>... t) const
        {
            return !invocation_->exhausted() && (*matcher_)(static_cast<ref_arg_t<Args>>(t)...);
        }

        bool invoke() const
        {
            for(auto& sequence : sequences_)
            {
                if(!sequence->is_valid(this))
                    return false;
            }
            bool result = invocation_->invoke();
            for(auto& sequence : sequences_)
                sequence->invalidate(this);
            return result;
        }

        const char* file() const { return file_; }
        int line() const { return line_; }

        friend std::ostream& operator<<(std::ostream& s, const expectation& e)
        {
            s << (e.invocation_->exhausted() ? 'v' : '.') << ' ' << *e.invocation_;
            constexpr bool hasArguments = arity > 0u;
            if(hasArguments)
                s << ".with( " << *e.matcher_ << " )";
            return s;
        }

    private:
        std::unique_ptr<invocation> invocation_;
        std::unique_ptr<matcher_base<Args...>> matcher_;
        std::vector<std::shared_ptr<sequence_impl>> sequences_;
        const char* file_;
        int line_;
    };
}} // namespace mock::detail

#endif // MOCK_EXPECTATION_HPP_INCLUDED
