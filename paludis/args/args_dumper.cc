/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Stephen Bennett <spb@gentoo.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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

#include <paludis/args/args_dumper.hh>
#include <paludis/args/args_option.hh>
#include <paludis/args/switch_arg.hh>
#include <paludis/args/alias_arg.hh>

#include <sstream>

using namespace paludis;
using namespace paludis::args;

ArgsDumper::ArgsDumper(std::ostream & os) :
    _os(os)
{
}

void ArgsDumper::visit(const ArgsOption * const a)
{
    std::stringstream p;
    p << "  --" << a->long_name();
    if (a->short_name())
        p << ", -" << a->short_name();
    if (p.str().length() < 24)
        p << std::string(24 - p.str().length(), ' ');
    _os << p.str();
    _os << " " << a->description() << std::endl;
}

#define VISIT(type) void ArgsDumper::visit(const type * const a) \
    { visit(static_cast<const ArgsOption *>(a)); }

VISIT(SwitchArg)
VISIT(StringArg)
VISIT(IntegerArg)
VISIT(AliasArg)

void ArgsDumper::visit(const EnumArg * const a)
{
    visit(static_cast<const ArgsOption *>(a));
    for(EnumArg::AllowedArgIterator it = a->begin_allowed_args(), it_end = a->end_allowed_args();
            it != it_end; ++it)
    {
        std::stringstream p;
        p << "      " << (*it).first;
        if (p.str().length() < 26)
            p << std::string(26 - p.str().length(), ' ');
        _os << p.str();
        _os << " " << (*it).second;
        if ((*it).first == a->default_arg())
            _os << " (default)";
        _os << std::endl;
    }
}
