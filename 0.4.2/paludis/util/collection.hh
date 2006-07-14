/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PALUDIS_GUARD_PALUDIS_SEQUENTIAL_COLLECTION_HH
#define PALUDIS_GUARD_PALUDIS_SEQUENTIAL_COLLECTION_HH 1

#include <algorithm>
#include <list>
#include <set>
#include <iterator>
#include <paludis/util/counted_ptr.hh>
#include <paludis/util/instantiation_policy.hh>

/** \file
 * Various wrappers around collections of items, for convenience and
 * avoiding passing around huge containers.
 *
 * \ingroup grpcollections
 */

namespace paludis
{
    /**
     * Wrapper around a std::list of a particular item. Multiple items
     * with the same value are disallowed.
     *
     * \ingroup grpcollections
     */
    template <typename T_>
    class SequentialCollection :
        private InstantiationPolicy<SequentialCollection<T_>, instantiation_method::NonCopyableTag>,
        public InternalCounted<SequentialCollection<T_> >,
        public std::iterator<typename std::iterator_traits<
            typename std::list<T_>::const_iterator>::iterator_category, T_>
    {
        private:
            std::list<T_> _items;

        public:
            /**
             * Issue with g++ 3.4.6: const_reference isn't defined via our std::iterator
             * inherit, but it is needed by many standard algorithms.
             */
            typedef const T_ & const_reference;

            ///\name Basic operations
            ///\{

            /**
             * Constructor.
             */
            SequentialCollection()
            {
            }

            /**
             * Destructor.
             */
            virtual ~SequentialCollection()
            {
            }

            ///\}

            ///\name Iterate over our items
            ///\{

            typedef typename std::list<T_>::const_iterator Iterator;

            Iterator begin() const
            {
                return _items.begin();
            }

            Iterator end() const
            {
                return _items.end();
            }

            Iterator last() const
            {
                return _items.begin() == _items.end() ? _items.end() : --(_items.end());
            }

            ///\}

            ///\name Finding items
            ///\{

            /**
             * Return an Iterator to an item, or end() if there's no match.
             */
            Iterator find(const T_ & v) const
            {
                return std::find(_items.begin(), _items.end(), v);
            }

            ///\}

            ///\name Adding and modifying items
            ///\{

            /**
             * Append an item, return whether we succeeded.
             */
            bool append(T_ v)
            {
                if (end() != find(v))
                    return false;

                _items.push_back(v);
                return true;
            }

            /**
             * Append an item.
             */
            void push_back(const T_ & v)
            {
                if (end() == find(v))
                    _items.push_back(v);
            }

            ///\}

            ///\name Queries
            ///\{

            /**
             * Are we empty?
             */
            bool empty() const
            {
                return _items.empty();
            }

            ///\}
    };

    /**
     * Wrapper around a std::set of a particular item. May be changed at some
     * point to support template find.
     *
     * \ingroup grpcollections
     */
    template <typename T_>
    class SortedCollection :
        private InstantiationPolicy<SortedCollection<T_>, instantiation_method::NonCopyableTag>,
        public InternalCounted<SortedCollection<T_> >,
        public std::iterator<typename std::iterator_traits<
            typename std::set<T_>::const_iterator>::iterator_category, T_>
    {
        private:
            std::set<T_> _items;

        public:
            ///\name Basic operations
            ///\{

            SortedCollection()
            {
            }

            virtual ~SortedCollection()
            {
            }

            ///\}

            ///\name Iterate over our items
            ///\{

            typedef typename std::set<T_>::const_iterator Iterator;

            Iterator begin() const
            {
                return _items.begin();
            }

            Iterator end() const
            {
                return _items.end();
            }

            typedef typename std::set<T_>::const_reverse_iterator ReverseIterator;

            ReverseIterator rbegin() const
            {
                return _items.rbegin();
            }

            ReverseIterator rend() const
            {
                return _items.rend();
            }

            Iterator last() const
            {
                Iterator result(_items.end());
                if (result != _items.begin())
                    --result;
                return result;
            }

            ///\}

            ///\name Finding items
            ///\{

            Iterator find(const T_ & v) const
            {
                return _items.find(v);
            }

            ///\}

            ///\name Adding, removing and modifying items
            ///\{

            /**
             * Insert an item, return whether we succeeded.
             */
            bool insert(const T_ & v)
            {
                return _items.insert(v).second;
            }

            /**
             * Erase an item, return whether we succeeded.
             */
            bool erase(const T_ & v)
            {
                return 0 != _items.erase(v);
            }

            /**
             * Insert all items from another container.
             */
            bool merge(typename SortedCollection<T_>::ConstPointer o)
            {
                bool result(true);
                Iterator o_begin(o->begin()), o_end(o->end());
                for ( ; o_begin != o_end ; ++o_begin)
                    result &= insert(*o_begin);
                return result;
            }

            /**
             * Our insert iterator type.
             */
            typedef typename std::insert_iterator<std::set<T_> > Inserter;

            /**
             * Fetch an inserter.
             */
            Inserter inserter()
            {
                return std::inserter(_items, _items.begin());
            }

            ///\}

            ///\name Queries
            ///\{

            /**
             * Are we empty?
             */
            bool empty() const
            {
                return _items.empty();
            }

            /**
             * How big are we?
             */
            unsigned size() const
            {
                return _items.size();
            }

            ///\}
    };
}

#endif
