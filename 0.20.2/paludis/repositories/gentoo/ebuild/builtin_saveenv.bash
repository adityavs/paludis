#!/bin/bash
# vim: set sw=4 sts=4 et :

# Copyright (c) 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

builtin_saveenv()
{
    [[ -d "${PALUDIS_LOADSAVEENV_DIR}" ]] || die "\$PALUDIS_LOADSAVEENV_DIR (\"${PALUDIS_LOADSAVEENV_DIR}\") not a directory"
    [[ -f "${PALUDIS_LOADSAVEENV_DIR}/loadsaveenv" ]] && rm -f "${PALUDIS_LOADSAVEENV_DIR}/loadsaveenv"
    ( set ; export -p ) | sed \
        -e '/^\(declare -[rx]\+ \)\?SANDBOX_/d' \
        -e '/^\(declare -[rx]\+ \)\?.\?[UP]ID/d' \
        -e '/^\(declare -[rx]\+ \)\?BASH_VERSINFO/d' \
        -e '/^\(declare -[rx]\+ \)\?PALUDIS_LOADSAVEENV_DIR/d' \
        -e '/^\(declare -[rx]\+ \)\?PALUDIS_DO_NOTHING_SANDBOXY/d' \
        -e '/^\(declare -[rx]\+ \)\?SHELLOPTS/d' \
        -e '/^\(declare -[rx]\+ \)\?EBUILD_KILL_PID/d' \
        -e 's:^declare -rx:declare -x:' \
        -e 's:^declare -x :export :' \
        > ${PALUDIS_LOADSAVEENV_DIR}/loadsaveenv
}

ebuild_f_saveenv()
{
    local old_sandbox_write="${SANDBOX_WRITE}"
    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && \
        SANDBOX_WRITE="${SANDBOX_WRITE+${SANDBOX_WRITE}:}${PALUDIS_LOADSAVEENV_DIR%/}/"

    if hasq "saveenv" ${RESTRICT} ; then
        ebuild_section "Skipping builtin_saveenv (RESTRICT)"
    elif hasq "saveenv" ${SKIP_FUNCTIONS} ; then
        ebuild_section "Skipping builtin_saveenv (SKIP_FUNCTIONS)"
    else
        ebuild_section "Starting builtin_saveenv"
        builtin_saveenv
        ebuild_section "Done builtin_saveenv"
    fi

    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && SANDBOX_WRITE="${old_sandbox_write}"
    true
}



