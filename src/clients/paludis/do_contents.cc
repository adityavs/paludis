/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#include "do_contents.hh"
#include <src/output/colour.hh>
#include "command_line.hh"
#include <paludis/paludis.hh>
#include <iostream>
#include <algorithm>

using namespace paludis;
using std::cout;
using std::cerr;
using std::endl;

namespace
{
    struct ContentsDisplayer :
        ContentsVisitorTypes::ConstVisitor
    {
        void visit(const ContentsFileEntry * const e)
        {
            cout << "    " << colour(cl_file, e->name()) << endl;
        }

        void visit(const ContentsDirEntry * const e)
        {
            cout << "    " << colour(cl_dir, e->name()) << endl;
        }

        void visit(const ContentsSymEntry * const e)
        {
            cout << "    " << colour(cl_sym, e->name()) << " -> " << e->target() << endl;
        }

        void visit(const ContentsMiscEntry * const e)
        {
            cout << "    " << colour(cl_misc, e->name()) << endl;
        }

        void visit(const ContentsFifoEntry * const e)
        {
            cout << "    " << colour(cl_fifo, e->name()) << endl;
        }

        void visit(const ContentsDevEntry * const e)
        {
            cout << "    " << colour(cl_dev, e->name()) << endl;
        }
    };
}

void
do_one_contents_entry(
        const std::tr1::shared_ptr<Environment> env,
        const PackageDatabaseEntry & e)
{
    cout << "* " << colour(cl_package_name, e) << endl;

    const RepositoryContentsInterface * const contents_interface(
            env->package_database()->fetch_repository(e.repository)->
            contents_interface);
    if (contents_interface)
    {
        std::tr1::shared_ptr<const Contents> contents(contents_interface->contents(
                    e.name, e.version));
        ContentsDisplayer d;
        std::for_each(contents->begin(), contents->end(), accept_visitor(&d));
    }
    else
        cout << "    " << colour(cl_error, "(unknown)") << endl;

    cout << endl;
}

void
do_one_contents(
        const std::tr1::shared_ptr<Environment> env,
        const std::string & q)
{
    Context local_context("When handling query '" + q + "':");

    /* we might have a dep spec, but we might just have a simple package name
     * without a category. either should work. */
    std::tr1::shared_ptr<PackageDepSpec> spec(std::string::npos == q.find('/') ?
            new PackageDepSpec(env->package_database()->fetch_unique_qualified_package_name(
                    PackageNamePart(q))) :
            new PackageDepSpec(q));

    std::tr1::shared_ptr<const PackageDatabaseEntryCollection>
        entries(env->package_database()->query(query::Matches(*spec) & query::InstalledAtRoot(
                        env->root()), qo_order_by_version));

    if (entries->empty())
        throw NoSuchPackageError(q);

    for (PackageDatabaseEntryCollection::Iterator i(entries->begin()),
            i_end(entries->end()) ; i != i_end ; ++i)
        do_one_contents_entry(env, *i);
}

int
do_contents(std::tr1::shared_ptr<Environment> env)
{
    int return_code(0);

    Context context("When performing contents action from command line:");

    CommandLine::ParametersIterator q(CommandLine::get_instance()->begin_parameters()),
        q_end(CommandLine::get_instance()->end_parameters());
    for ( ; q != q_end ; ++q)
    {
        try
        {
            do_one_contents(env, *q);
        }
        catch (const AmbiguousPackageNameError & e)
        {
            cout << endl;
            cerr << "Query error:" << endl;
            cerr << "  * " << e.backtrace("\n  * ");
            cerr << "Ambiguous package name '" << e.name() << "'. Did you mean:" << endl;
            for (AmbiguousPackageNameError::OptionsIterator o(e.begin_options()),
                    o_end(e.end_options()) ; o != o_end ; ++o)
                cerr << "    * " << colour(cl_package_name, *o) << endl;
            cerr << endl;
        }
        catch (const NameError & e)
        {
            return_code |= 1;
            cout << endl;
            cerr << "Query error:" << endl;
            cerr << "  * " << e.backtrace("\n  * ") << e.message() << endl;
            cerr << endl;
        }
        catch (const PackageDatabaseLookupError & e)
        {
            return_code |= 1;
            cout << endl;
            cerr << "Query error:" << endl;
            cerr << "  * " << e.backtrace("\n  * ") << e.message() << endl;
            cerr << endl;
        }
    }

    return return_code;
}

