/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2008 Ciaran McCreesh
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

#include <paludis/stripper.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/kc.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/dir_iterator.hh>
#include <paludis/util/strip.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/system.hh>
#include <paludis/util/log.hh>
#include <paludis/util/tr1_functional.hh>
#include <sstream>
#include <list>
#include <algorithm>
#include <sys/stat.h>

using namespace paludis;

namespace paludis
{
    template <>
    struct Implementation<Stripper>
    {
        StripperOptions options;

        Implementation(const StripperOptions & o) :
            options(o)
        {
        }
    };
}

Stripper::Stripper(const StripperOptions & options) :
    PrivateImplementationPattern<Stripper>(new Implementation<Stripper>(options))
{
}

Stripper::~Stripper()
{
}

void
Stripper::strip()
{
    Context context("When stripping image '" + stringify(_imp->options[k::image_dir()]) + "':");

    if (_imp->options[k::debug_build()] == iado_internal)
        return;

    do_dir_recursive(_imp->options[k::image_dir()]);
}

void
Stripper::do_dir_recursive(const FSEntry & f)
{
    Context context("When stripping inside '" + stringify(f) + "':");

    if (f == _imp->options[k::debug_dir()])
        return;

    on_enter_dir(f);

    for (DirIterator d(f, DirIteratorOptions() + dio_include_dotfiles + dio_inode_sort), d_end ; d != d_end ; ++d)
    {
        if (d->is_symbolic_link())
            continue;

        if (d->is_directory())
            do_dir_recursive(*d);

        else if (d->is_regular_file())
        {
            if (d->has_permission(fs_ug_owner, fs_perm_execute) ||
                    d->has_permission(fs_ug_group, fs_perm_execute) ||
                    d->has_permission(fs_ug_others, fs_perm_execute) ||
                    (std::string::npos != d->basename().find(".so.")) ||
                    (d->basename() != strip_trailing_string(d->basename(), ".so"))
                    )
            {
                std::string t(file_type(*d));
                if (std::string::npos != t.find("SB executable") || std::string::npos != t.find("SB shared object"))
                {
                    FSEntry target(_imp->options[k::debug_dir()] / d->strip_leading(_imp->options[k::image_dir()]));
                    target = target.dirname() / (target.basename() + ".debug");
                    on_split(*d, target);
                    do_split(*d, target);
                }
                else if (std::string::npos != t.find("current ar archive"))
                {
                    on_strip(*d);
                    do_strip(*d, "-g");
                }
                else
                    on_unknown(*d);
            }
        }
    }

    on_leave_dir(f);
}

std::string
Stripper::file_type(const FSEntry & f)
{
    Context context("When finding the file type of '" + stringify(f) + "':");

    std::stringstream s;
    if (0 != run_command(Command("file '" + stringify(f) + "'").with_captured_stdout_stream(&s)))
        Log::get_instance()->message(ll_warning, lc_context) << "Couldn't work out the file type of '" << f << "'";

    return s.str();
}

void
Stripper::do_strip(const FSEntry & f, const std::string & options)
{
    Context context("When stripping '" + stringify(f) + "':");
    if (0 != run_command(Command("strip " + options + " '" + stringify(f) + "'")))
        Log::get_instance()->message(ll_warning, lc_context) << "Couldn't strip '" << f << "'";
}

void
Stripper::do_split(const FSEntry & f, const FSEntry & g)
{
    Context context("When splitting '" + stringify(f) + "' to '" + stringify(g) + "':");

    switch (_imp->options[k::debug_build()])
    {
        case iado_internal:
        case last_iado:
            return;

        case iado_none:
            do_strip(f, "");
            return;

        case iado_split:
            break;
    }

    {
        std::list<FSEntry> to_make;
        for (FSEntry d(g.dirname()) ; (! d.exists()) && (d != _imp->options[k::image_dir()]) ; d = d.dirname())
            to_make.push_front(d);

        using namespace tr1::placeholders;
        std::for_each(to_make.begin(), to_make.end(), tr1::bind(tr1::mem_fn(&FSEntry::mkdir), _1, 0755));
    }

    if (0 != run_command(Command("objcopy --only-keep-debug '" + stringify(f) + "' '" + stringify(g) + "'")))
        Log::get_instance()->message(ll_warning, lc_context) << "Couldn't copy debug information for '" << f << "'";
    else if (0 != run_command(Command("objcopy --add-gnu-debuglink='" + stringify(g) + "' '" + stringify(f) + "'")))
        Log::get_instance()->message(ll_warning, lc_context) << "Couldn't add debug link for '" << f << "'";
    else
    {
        FSEntry(g).chmod(g.permissions() & ~(S_IXGRP | S_IXUSR | S_IXOTH | S_IWOTH));
        do_strip(f, "");
    }
}

