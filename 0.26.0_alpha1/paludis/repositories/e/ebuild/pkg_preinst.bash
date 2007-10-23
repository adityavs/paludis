#!/bin/bash
# vim: set sw=4 sts=4 et :

# Copyright (c) 2006, 2007 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
#
# Based in part upon ebuild.sh from Portage, which is Copyright 1995-2005
# Gentoo Foundation and distributed under the terms of the GNU General
# Public License v2.
#
# This file is part of the Paludis package manager. Paludis is free software;
# you can redistribute it and/or modify it under the terms of the GNU General
# Public License, version 2, as published by the Free Software Foundation.
#
# Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA

pkg_preinst()
{
    :
}

ebuild_f_preinst()
{
    local old_sandbox_write="${SANDBOX_WRITE}"
    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && SANDBOX_WRITE="${SANDBOX_WRITE+${SANDBOX_WRITE}:}${ROOT%/}/"

    if hasq "preinst" ${RESTRICT} ; then
        ebuild_section "Skipping pkg_preinst (RESTRICT)"
    elif hasq "preinst" ${SKIP_FUNCTIONS} ; then
        ebuild_section "Skipping pkg_preinst (SKIP_FUNCTIONS)"
    else
        if [[ $(type -t pre_pkg_preinst ) == "function" ]] ; then
            ebuild_section "Starting pre_pkg_preinst"
            pre_pkg_preinst
            ebuild_section "Done pre_pkg_preinst"
        fi

        ebuild_section "Starting pkg_preinst"
        pkg_preinst
        ebuild_section "Done pkg_preinst"

        if [[ $(type -t post_pkg_preinst ) == "function" ]] ; then
            ebuild_section "Starting post_pkg_preinst"
            post_pkg_preinst
            ebuild_section "Done post_pkg_preinst"
        fi
    fi

    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && SANDBOX_WRITE="${old_sandbox_write}"
    true
}

