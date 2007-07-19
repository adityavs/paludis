/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 Mike Kelly <pioto@pioto.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_AAVISITOR_HH
#define PALUDIS_GUARD_PALUDIS_AAVISITOR_HH 1

#include <paludis/dep_spec.hh>

/** \file
 * Declarations for the AAVisitor class.
 *
 * \ingroup grpaavisitor
 */

namespace paludis
{
    namespace erepository
    {
        /**
         * Get a list of all the URIs in a URIDepSpec, regardless of USE
         * flag settings.
         *
         * \ingroup grpaavisitor
         */
        class PALUDIS_VISIBLE AAVisitor :
            public ConstVisitor<URISpecTree>,
            private PrivateImplementationPattern<AAVisitor>
        {
            public:
                ///\name Basic operations
                ///\{

                AAVisitor();

                ~AAVisitor();

                ///\}

                /// \name Visit functions
                ///{

                void visit_sequence(const AllDepSpec &,
                        URISpecTree::ConstSequenceIterator,
                        URISpecTree::ConstSequenceIterator);

                void visit_sequence(const UseDepSpec &,
                        URISpecTree::ConstSequenceIterator,
                        URISpecTree::ConstSequenceIterator);

                void visit_leaf(const URIDepSpec &);

                ///}

                /// \name Iterator functions
                ///{
                
                typedef libwrapiter::ForwardIterator<AAVisitor, const std::string> Iterator;

                Iterator begin() const PALUDIS_ATTRIBUTE((warn_unused_result));
                Iterator end() const PALUDIS_ATTRIBUTE((warn_unused_result));

                ///}
        };
    }
}
#endif
