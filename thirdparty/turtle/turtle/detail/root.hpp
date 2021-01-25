// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_ROOT_HPP_INCLUDED
#define MOCK_ROOT_HPP_INCLUDED

#include "../config.hpp"
#include "child.hpp"
#include "context.hpp"
#include "group.hpp"
#include "mutex.hpp"
#include "parent.hpp"
#include "singleton.hpp"
#include <boost/optional.hpp>
#include <map>
#include <ostream>

namespace mock { namespace detail {
    class root_t : public singleton<root_t>, public context
    {
    public:
        virtual void add(const void* p,
                         verifiable& v,
                         boost::unit_test::const_string instance,
                         boost::optional<type_name> type,
                         boost::unit_test::const_string name)
        {
            scoped_lock _(mutex_);
            auto it = children_.lower_bound(&v);
            if(it == children_.end() || children_.key_comp()(&v, it->first))
                it = children_.insert(it, std::make_pair(&v, counter_child(parents_, p)));
            it->second.update(instance, type, name);
        }
        virtual void add(verifiable& v)
        {
            scoped_lock _(mutex_);
            group_.add(v);
        }

        virtual void remove(verifiable& v)
        {
            scoped_lock _(mutex_);
            group_.remove(v);
            children_.erase(&v);
        }

        bool verify() const
        {
            scoped_lock _(mutex_);
            return group_.verify();
        }
        void reset()
        {
            scoped_lock _(mutex_);
            group_.reset();
        }

        virtual void serialize(std::ostream& s, const verifiable& v) const
        {
            scoped_lock _(mutex_);
            const auto it = children_.find(&v);
            if(it != children_.end())
                s << it->second;
            else
                s << "?";
        }

    private:
        typedef std::map<const void*, std::pair<parent, std::size_t>> parents_t;

        class counter_child
        {
        public:
            counter_child(parents_t& parents, const void* p)
                : parents_(&parents), it_(parents.insert(std::make_pair(p, parents_t::mapped_type())).first)
            {
                ++it_->second.second;
            }
            counter_child(const counter_child& rhs) : parents_(rhs.parents_), it_(rhs.it_), child_(rhs.child_)
            {
                ++it_->second.second;
            }
            ~counter_child()
            {
                if(--it_->second.second == 0)
                    parents_->erase(it_);
            }
            void update(boost::unit_test::const_string instance,
                        boost::optional<type_name> type,
                        boost::unit_test::const_string name)
            {
                child_.update(it_->second.first, instance, type, name);
            }
            friend std::ostream& operator<<(std::ostream& s, const counter_child& c) { return s << c.child_; }

        private:
            counter_child& operator=(const counter_child&);

            parents_t* parents_;
            parents_t::iterator it_;
            child child_;
        };

        parents_t parents_;
        std::map<const verifiable*, counter_child> children_;
        group group_;
        mutable mutex mutex_;

        MOCK_SINGLETON_CONS(root_t);
    };
    MOCK_SINGLETON_INST(root)
}} // namespace mock::detail

#endif // MOCK_ROOT_HPP_INCLUDED
