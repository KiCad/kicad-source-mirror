// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_SEQUENCE_IMPL_HPP_INCLUDED
#define MOCK_SEQUENCE_IMPL_HPP_INCLUDED

#include "../config.hpp"
#include "mutex.hpp"
#include <algorithm>
#include <memory>
#include <vector>

namespace mock { namespace detail {
    class sequence_impl
    {
    public:
        sequence_impl() : mutex_(std::make_shared<mutex>()) {}

        void add(void* e)
        {
            lock _(mutex_);
            elements_.push_back(e);
        }
        void remove(void* e)
        {
            lock _(mutex_);
            elements_.erase(std::remove(elements_.begin(), elements_.end(), e), elements_.end());
        }

        bool is_valid(const void* e) const
        {
            lock _(mutex_);
            return std::find(elements_.begin(), elements_.end(), e) != elements_.end();
        }

        void invalidate(const void* e)
        {
            lock _(mutex_);
            const auto it = std::find(elements_.begin(), elements_.end(), e);
            if(it != elements_.end())
                elements_.erase(elements_.begin(), it);
        }

    private:
        std::vector<void*> elements_;
        const std::shared_ptr<mutex> mutex_;
    };
}} // namespace mock::detail

#endif // MOCK_SEQUENCE_IMPL_HPP_INCLUDED
