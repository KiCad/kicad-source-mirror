// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_INVOCATION_HPP_INCLUDED
#define MOCK_INVOCATION_HPP_INCLUDED

#include "../config.hpp"
#include <limits>
#include <ostream>
#include <stdexcept>

namespace mock { namespace detail {
    class invocation
    {
    public:
        invocation() = default;
        invocation(const invocation&) = delete;
        invocation& operator=(const invocation&) = delete;

        virtual ~invocation() = default;

        virtual bool invoke() = 0;
        virtual bool verify() const = 0;

        virtual bool exhausted() const = 0;

        friend std::ostream& operator<<(std::ostream& s, const invocation& i) { return i.serialize(s); }

    private:
        virtual std::ostream& serialize(std::ostream& s) const = 0;
    };

    class between : public invocation
    {
    public:
        between(std::size_t min, std::size_t max) : min_(min), max_(max), count_(0)
        {
            if(min > max)
                throw std::invalid_argument("'min' > 'max'");
        }

        virtual bool invoke()
        {
            if(count_ == max_)
                return false;
            ++count_;
            return true;
        }

        virtual bool exhausted() const { return count_ >= max_; }

        virtual bool verify() const { return min_ <= count_ && count_ <= max_; }

    protected:
        const std::size_t min_, max_;
        std::size_t count_;

    private:
        virtual std::ostream& serialize(std::ostream& s) const
        {
            return s << "between( " << count_ << "/[" << min_ << ',' << max_ << "] )";
        }
    };

    class exactly : public between
    {
    public:
        explicit exactly(std::size_t count) : between(count, count) {}

    private:
        virtual std::ostream& serialize(std::ostream& s) const
        {
            return s << "exactly( " << count_ << '/' << max_ << " )";
        }
    };

    class never : public exactly
    {
    public:
        never() : exactly(0) {}

    private:
        virtual std::ostream& serialize(std::ostream& s) const { return s << "never()"; }
    };

    class once : public exactly
    {
    public:
        once() : exactly(1) {}

    private:
        virtual std::ostream& serialize(std::ostream& s) const { return s << "once()"; }
    };

    class at_least : public between
    {
    public:
        explicit at_least(std::size_t min) : between(min, (std::numeric_limits<std::size_t>::max)()) {}

    private:
        virtual std::ostream& serialize(std::ostream& s) const
        {
            return s << "at_least( " << count_ << '/' << min_ << " )";
        }
    };

    class at_most : public between
    {
    public:
        explicit at_most(std::size_t max) : between(0, max) {}

    private:
        virtual std::ostream& serialize(std::ostream& s) const
        {
            return s << "at_most( " << count_ << '/' << max_ << " )";
        }
    };

    class unlimited : public at_least
    {
    public:
        unlimited() : at_least(0) {}

    private:
        virtual std::ostream& serialize(std::ostream& s) const { return s << "unlimited()"; }
    };
}} // namespace mock::detail

#endif // MOCK_INVOCATION_HPP_INCLUDED
