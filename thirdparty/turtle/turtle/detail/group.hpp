// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_GROUP_HPP_INCLUDED
#define MOCK_GROUP_HPP_INCLUDED

#include "../config.hpp"
#include "verifiable.hpp"
#include <algorithm>
#include <functional>
#include <vector>

namespace mock { namespace detail {
    class group
    {
    public:
        void add(verifiable& v) { verifiables_.push_back(&v); }
        void remove(verifiable& v)
        {
            verifiables_.erase(std::remove(verifiables_.begin(), verifiables_.end(), &v), verifiables_.end());
        }

        bool verify() const
        {
            bool valid = true;
            for(const auto* verifiable : verifiables_)
                if(!verifiable->verify())
                    valid = false;
            return valid;
        }
        void reset()
        {
            const auto verifiables = verifiables_;
            for(auto* verifiable : verifiables)
                if(std::find(verifiables_.begin(), verifiables_.end(), verifiable) != verifiables_.end())
                    verifiable->reset();
        }

    private:
        std::vector<verifiable*> verifiables_;
    };
}} // namespace mock::detail

#endif // MOCK_GROUP_HPP_INCLUDED
