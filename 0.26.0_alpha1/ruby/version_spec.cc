/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
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

#include <paludis_ruby.hh>
#include <paludis/version_spec.hh>
#include <ruby.h>

using namespace paludis;
using namespace paludis::ruby;

#define RUBY_FUNC_CAST(x) reinterpret_cast<VALUE (*)(...)>(x)

namespace
{
    static VALUE c_version_spec;

    VALUE
    version_spec_init(VALUE self, VALUE)
    {
        return self;
    }

    /*
     * call-seq:
     *     VersionSpec.new(version_string) -> VersionSpec
     *
     * Creates a new VersionSpec from the given string.
     */
    VALUE
    version_spec_new(VALUE self, VALUE s)
    {
        VersionSpec * ptr(0);
        try
        {
            ptr = new VersionSpec(std::string(StringValuePtr(s)));
            VALUE tdata(Data_Wrap_Struct(self, 0, &Common<VersionSpec>::free, ptr));
            rb_obj_call_init(tdata, 1, &s);
            return tdata;
        }
        catch (const std::exception & e)
        {
            delete ptr;
            exception_to_ruby_exception(e);
        }
    }

    /*
     * call-seq:
     *     remove_revision -> VersionSpec
     *
     * Returns a VersionSpec without the revision part.
     */
    VALUE
    version_spec_remove_revision(VALUE self)
    {
        try
        {
            return version_spec_to_value(value_to_version_spec(self).remove_revision());
        }
        catch (const std::exception & e)
        {
            exception_to_ruby_exception(e);
        }
    }

    /*
     * call-seq:
     *     revision_only -> String
     *
     * Revision part only (or "r0").
     */
    VALUE
    version_spec_revision_only(VALUE self)
    {
        try
        {
            return rb_str_new2((value_to_version_spec(self).revision_only().c_str()));
        }
        catch (const std::exception & e)
        {
            exception_to_ruby_exception(e);
        }
    }

    /*
     * call-seq:
     *     bump -> VersionSpec
     *
     * This is used by the ~> operator. It returns a VersionSpec where the next to last number is one greater (e.g. 5.3.1 => 5.4).
     * Any non number parts are stripped (e.g. 1.2.3_alpha4-r5 => 1.3).
     */
    VALUE
    version_spec_bump(VALUE self)
    {
        return version_spec_to_value(value_to_version_spec(self).bump());
    }

    /*
     * call-seq:
     *     is_scm? -> true or false
     *
     * Are we an -scm package, or something pretending to be one?
     */
    VALUE version_spec_is_scm(VALUE self)
    {
        return value_to_version_spec(self).is_scm() ? Qtrue : Qfalse;
    }

    void do_register_version_spec()
    {
        /*
         * Document-class: Paludis::VersionSpec
         *
         * A VersionSpec represents a version number (for example, 1.2.3b-r1). Includes
         * Comparable[http://www.ruby-doc.org/core/classes/Comparable.html]
         */
        c_version_spec = rb_define_class_under(paludis_module(), "VersionSpec", rb_cObject);
        rb_define_singleton_method(c_version_spec, "new", RUBY_FUNC_CAST(&version_spec_new), 1);
        rb_define_method(c_version_spec, "initialize", RUBY_FUNC_CAST(&version_spec_init), 1);
        rb_define_method(c_version_spec, "<=>", RUBY_FUNC_CAST(&Common<VersionSpec>::compare), 1);
        rb_include_module(c_version_spec, rb_mComparable);
        rb_define_method(c_version_spec, "to_s", RUBY_FUNC_CAST(&Common<VersionSpec>::to_s), 0);
        rb_define_method(c_version_spec, "remove_revision", RUBY_FUNC_CAST(&version_spec_remove_revision), 0);
        rb_define_method(c_version_spec, "revision_only", RUBY_FUNC_CAST(&version_spec_revision_only), 0);
        rb_define_method(c_version_spec, "bump", RUBY_FUNC_CAST(&version_spec_bump), 0);
        rb_define_method(c_version_spec, "is_scm?", RUBY_FUNC_CAST(&version_spec_is_scm), 0);
    }
}

VersionSpec
paludis::ruby::value_to_version_spec(VALUE v)
{
    if (T_STRING == TYPE(v))
        return VersionSpec(StringValuePtr(v));
    else if (rb_obj_is_kind_of(v, c_version_spec))
    {
        VersionSpec * v_ptr;
        Data_Get_Struct(v, VersionSpec, v_ptr);
        return *v_ptr;
    }
    else
    {
        rb_raise(rb_eTypeError, "Can't convert %s into VersionSpec", rb_obj_classname(v));
    }
}

VALUE
paludis::ruby::version_spec_to_value(const VersionSpec & v)
{
    VersionSpec * vv(new VersionSpec(v));
    return Data_Wrap_Struct(c_version_spec, 0, &Common<VersionSpec>::free, vv);
}

RegisterRubyClass::Register paludis_ruby_register_version_spec PALUDIS_ATTRIBUTE((used))
    (&do_register_version_spec);



