/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ciaran McCreesh
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

#ifndef PALUDIS_GUARD_PALUDIS_RESOLVER_DESTINATIONS_HH
#define PALUDIS_GUARD_PALUDIS_RESOLVER_DESTINATIONS_HH 1

#include <paludis/resolver/destinations-fwd.hh>
#include <paludis/resolver/serialise-fwd.hh>
#include <paludis/resolver/destination_types-fwd.hh>
#include <paludis/util/named_value.hh>
#include <paludis/util/private_implementation_pattern.hh>
#include <paludis/name.hh>
#include <paludis/package_id-fwd.hh>
#include <tr1/memory>

namespace paludis
{
    namespace n
    {
        struct replacing;
        struct repository;
    }

    namespace resolver
    {
        struct Destination
        {
            NamedValue<n::replacing, std::tr1::shared_ptr<const PackageIDSequence> > replacing;
            NamedValue<n::repository, RepositoryName> repository;

            void serialise(Serialiser &) const;

            static const std::tr1::shared_ptr<Destination> deserialise(
                    Deserialisation & d) PALUDIS_ATTRIBUTE((warn_unused_result));
        };

        class PALUDIS_VISIBLE Destinations :
            private PrivateImplementationPattern<Destinations>
        {
            public:
                Destinations();
                ~Destinations();

                const std::tr1::shared_ptr<const Destination> by_type(const DestinationType)
                    const PALUDIS_ATTRIBUTE((warn_unused_result));

                void set_destination_type(const DestinationType, const std::tr1::shared_ptr<const Destination> &);

                void serialise(Serialiser &) const;

                static const std::tr1::shared_ptr<Destinations> deserialise(
                        Deserialisation & d) PALUDIS_ATTRIBUTE((warn_unused_result));
        };
    }

#ifdef PALUDIS_HAVE_EXTERN_TEMPLATE
    extern template class PrivateImplementationPattern<resolver::Destinations>;
#endif

}

#endif
