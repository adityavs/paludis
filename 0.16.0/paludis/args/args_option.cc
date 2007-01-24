/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#include "args.hh"
#include "args_error.hh"
#include <set>
#include <vector>
#include <algorithm>

/** \file
 * Implementation for ArgsOption.
 *
 * \ingroup grplibpaludisargs
 */

using namespace paludis::args;

namespace
{
    /**
     * Is an arg a particular value?
     *
     * \ingroup grplibpaludisargs
     */
    struct ArgIs
    {
        /// The argument.
        const std::string arg;

        /// Constructor.
        ArgIs(const std::string & a) :
            arg(a)
        {
        }

        /// Comparator.
        bool operator() (const std::pair<std::string, std::string> & p) const
        {
            return p.first == arg;
        }
    };
}

ArgsOption::ArgsOption(ArgsGroup * const g, const std::string & our_long_name,
        const char our_short_name, const std::string & our_description) :
    _group(g),
    _long_name(our_long_name),
    _short_name(our_short_name),
    _description(our_description),
    _specified(false)
{
    g->add(this);
    g->handler()->add_option(this, our_long_name, our_short_name);
}

ArgsOption::~ArgsOption()
{
}

SwitchArg::SwitchArg(ArgsGroup * const our_group, std::string our_long_name, char our_short_name,
        std::string our_description) :
    ArgsOption(our_group, our_long_name, our_short_name, our_description)
{
}

SwitchArg::~SwitchArg()
{
}

AliasArg::AliasArg(ArgsOption * const o, const std::string & our_long_name) :
    ArgsOption(o->group(), our_long_name, '\0', "Alias for --" + o->long_name()),
    _other(o)
{
    o->group()->handler()->add_option(o, our_long_name);
}

StringArg::StringArg(ArgsGroup * const g, const std::string & our_long_name,
        const char our_short_name, const std::string & our_description) :
    ArgsOption(g, our_long_name, our_short_name, our_description)
{
}

namespace paludis
{
    /**
     * Implementation data for StringSetArg.
     *
     * \ingroup grplibpaludisargs
     */
    template<>
    struct Implementation<StringSetArg> :
        InternalCounted<Implementation<StringSetArg> >
    {
        std::set<std::string> args;
        std::vector<std::pair<std::string, std::string> > allowed_args;
    };

    /**
     * Implementation data for StringSetArg::StringSetArgOptions.
     *
     * \ingroup grplibpaludisargs
     */
    template<>
    struct Implementation<StringSetArg::StringSetArgOptions> :
        InternalCounted<Implementation<StringSetArg::StringSetArgOptions> >
    {
        std::vector<std::pair<std::string, std::string> > options;
    };
}

StringSetArg::StringSetArg(ArgsGroup * const g, const std::string & our_long_name,
        const char our_short_name, const std::string & our_description,
        const StringSetArgOptions & opts) :
    ArgsOption(g, our_long_name, our_short_name, our_description),
    PrivateImplementationPattern<StringSetArg>(new Implementation<StringSetArg>)
{
    std::copy(opts._imp->options.begin(), opts._imp->options.end(),
            std::back_inserter(_imp->allowed_args));
}

StringSetArg::Iterator
StringSetArg::begin_args() const
{
    return Iterator(_imp->args.begin());
}

StringSetArg::Iterator
StringSetArg::end_args() const
{
    return Iterator(_imp->args.end());
}

void
StringSetArg::add_argument(const std::string & arg)
{
    if (! _imp->allowed_args.empty())
        if (_imp->allowed_args.end() == std::find_if(_imp->allowed_args.begin(),
                    _imp->allowed_args.end(), ArgIs(arg)))
            throw (BadValue("--" + long_name(), arg));

    _imp->args.insert(arg);
}

IntegerArg::IntegerArg(ArgsGroup * const our_group, const std::string & our_long_name,
                char our_short_name, const std::string & our_description) :
    ArgsOption(our_group, our_long_name, our_short_name, our_description)
{
}

namespace paludis
{
    /**
     * Implementation data for EnumArg.
     *
     * \ingroup grplibpaludisargs
     */
    template<>
    struct Implementation<EnumArg> :
        InternalCounted<Implementation<EnumArg> >
    {
        std::vector<std::pair<std::string, std::string> > allowed_args;
    };

    /**
     * Implementation data for EnumArg::EnumArgOptions.
     *
     * \ingroup grplibpaludisargs
     */
    template<>
    struct Implementation<EnumArg::EnumArgOptions> :
        InternalCounted<Implementation<EnumArg::EnumArgOptions> >
    {
        std::vector<std::pair<std::string, std::string> > options;
    };
}

StringSetArg::StringSetArgOptions::StringSetArgOptions(std::string opt, std::string desc) :
    PrivateImplementationPattern<StringSetArgOptions>(new Implementation<StringSetArgOptions>)
{
    _imp->options.push_back(std::make_pair(opt, desc));
}

StringSetArg::StringSetArgOptions &
StringSetArg::StringSetArgOptions::operator() (std::string opt, std::string desc)
{
    _imp->options.push_back(std::make_pair(opt, desc));
    return *this;
}

StringSetArg::StringSetArgOptions::StringSetArgOptions(const StringSetArg::StringSetArgOptions & o) :
    PrivateImplementationPattern<StringSetArgOptions>(new Implementation<StringSetArgOptions>)
{
    std::copy(o._imp->options.begin(), o._imp->options.end(),
            std::back_inserter(_imp->options));
}

StringSetArg::StringSetArgOptions::~StringSetArgOptions()
{
}

StringSetArg::StringSetArgOptions::StringSetArgOptions() :
    PrivateImplementationPattern<StringSetArgOptions>(new Implementation<StringSetArgOptions>)
{
}

void EnumArg::set_argument(const std::string & arg)
{
    if (_imp->allowed_args.end() == std::find_if(_imp->allowed_args.begin(),
                _imp->allowed_args.end(), ArgIs(arg)))
        throw (BadValue("--" + long_name(), arg));

    _argument = arg;
}

EnumArg::~EnumArg()
{
}

EnumArg::EnumArgOptions::EnumArgOptions(std::string opt, std::string desc) :
    PrivateImplementationPattern<EnumArgOptions>(new Implementation<EnumArgOptions>)
{
    _imp->options.push_back(std::make_pair(opt, desc));
}

EnumArg::EnumArgOptions & EnumArg::EnumArgOptions::operator() (std::string opt, std::string desc)
{
    _imp->options.push_back(std::make_pair(opt, desc));
    return *this;
}

EnumArg::EnumArgOptions::~EnumArgOptions()
{
}

EnumArg::EnumArg(ArgsGroup * const our_group, const std::string & our_long_name,
        const char our_short_name, const std::string & our_description,
        const EnumArgOptions & opts, const std::string & our_default_arg) :
    ArgsOption(our_group, our_long_name, our_short_name, our_description),
    PrivateImplementationPattern<EnumArg>(new Implementation<EnumArg>),
    _argument(our_default_arg),
    _default_arg(our_default_arg)
{
    _imp->allowed_args = opts._imp->options;
}

EnumArg::AllowedArgIterator
EnumArg::begin_allowed_args() const
{
    return AllowedArgIterator(_imp->allowed_args.begin());
}

EnumArg::AllowedArgIterator
EnumArg::end_allowed_args() const
{
    return AllowedArgIterator(_imp->allowed_args.end());
}

StringSetArg::~StringSetArg()
{
}

StringSetArg::AllowedArgIterator
StringSetArg::begin_allowed_args() const
{
    return AllowedArgIterator(_imp->allowed_args.begin());
}

StringSetArg::AllowedArgIterator
StringSetArg::end_allowed_args() const
{
    return AllowedArgIterator(_imp->allowed_args.end());
}

