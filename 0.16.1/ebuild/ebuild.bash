#!/bin/bash
# vim: set sw=4 sts=4 et :

# Copyright (c) 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

unalias -a
unset GZIP BZIP BZIP2 CDPATH GREP_OPTIONS GREP_COLOR GLOBIGNORE
eval unset LANG ${!LC_*}

if [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] ; then
    export SANDBOX_PREDICT="${SANDBOX_PREDICT+${SANDBOX_PREDICT}:}"
    export SANDBOX_PREDICT="${SANDBOX_PREDICT}/proc/self/maps:/dev/console:/dev/random"
    export SANDBOX_WRITE="${SANDBOX_WRITE+${SANDBOX_WRITE}:}"
    export SANDBOX_WRITE="${SANDBOX_WRITE}/dev/shm:/dev/stdout:/dev/stderr:/dev/null:/dev/tty"
    export SANDBOX_WRITE="${SANDBOX_WRITE}:${PALUDIS_TMPDIR}:/var/cache"
    export SANDBOX_WRITE="${SANDBOX_WRITE}:/proc/self/attr:/proc/self/task:/selinux/context"
    export SANDBOX_ON="1"
    export SANDBOX_BASHRC="/dev/null"
    export BASH_ENV
fi
export REAL_CHOST="${CHOST}"

shopt -s expand_aliases
shopt -s extglob

EBUILD_KILL_PID=$$
alias die='diefunc "$FUNCNAME" "$LINENO"'
alias assert='_pipestatus="${PIPESTATUS[*]}"; [[ -z "${_pipestatus//[ 0]/}" ]] || diefunc "$FUNCNAME" "$LINENO" "$_pipestatus"'
trap 'echo "die trap: exiting with error." 1>&2 ; exit 250' 15

export EBUILD_PROGRAM_NAME="$0"

diefunc()
{
    local func="$1" line="$2"
    shift 2
    echo 1>&2
    echo "!!! ERROR in ${CATEGORY:-?}/${PF:-?}:" 1>&2
    echo "!!! In ${func:-?} at line ${line:-?}" 1>&2
    echo "!!! ${*:-(no message provided)}" 1>&2
    echo 1>&2

    echo "!!! Call stack:" 1>&2
    for (( n = 1 ; n < ${#FUNCNAME[@]} ; ++n )) ; do
        funcname=${FUNCNAME[${n}]}
        sourcefile=${BASH_SOURCE[${n}]}
        lineno=${BASH_LINENO[$(( n - 1 ))]}
        echo "!!!    * ${funcname} (${sourcefile}:${lineno})" 1>&2
    done
    echo 1>&2

    if [[ -n "${PALUDIS_EXTRA_DIE_MESSAGE}" ]] ; then
        echo "${PALUDIS_EXTRA_DIE_MESSAGE}" 1>&2
        echo 1>&2
    fi

    kill ${EBUILD_KILL_PID}
    exit 249
}

if [[ -n "${PALUDIS_EBUILD_DIR_FALLBACK}" ]] ; then
    export PATH="${PALUDIS_EBUILD_DIR_FALLBACK}/utils:${PATH}"
fi
export PATH="${PALUDIS_EBUILD_DIR}/utils:${PATH}"
EBUILD_MODULES_DIR=$(canonicalise $(dirname $0 ) )
[[ -d ${EBUILD_MODULES_DIR} ]] || die "${EBUILD_MODULES_DIR} is not a directory"
export PALUDIS_EBUILD_MODULES_DIR="${EBUILD_MODULES_DIR}"

ebuild_load_module()
{
    source "${EBUILD_MODULES_DIR}/${1}.bash" || die "Error loading module ${1}"
}

ebuild_load_module echo_functions
ebuild_load_module kernel_functions
ebuild_load_module sandbox
ebuild_load_module portage_stubs
ebuild_load_module list_functions
ebuild_load_module multilib_functions
ebuild_load_module install_functions
ebuild_load_module build_functions
ebuild_load_module eclass_functions
ebuild_load_module work_around_broken_utilities

export PALUDIS_HOME="$(canonicalise ${PALUDIS_HOME:-${HOME}} )"

ebuild_source_profile()
{
    if [[ -f ${1}/parent ]] ; then
        while read line ; do
            grep --silent '^[[:space:]]*#' <<<"${line}" && continue
            grep --silent '[^[:space:]]' <<<"${line}" || continue
            ebuild_source_profile $(canonicalise ${1}/${line} )
        done <${1}/parent
    fi

    if [[ -f ${1}/make.defaults ]] ; then
        eval "$(sed -e 's/^\([a-zA-Z0-9\-_]\+=\)/export \1/' ${1}/make.defaults )" \
            || die "Couldn't source ${1}/make.defaults"
    fi

    if [[ -f ${1}/bashrc ]] ; then
        source ${1}/bashrc || die "Couldn't source ${1}/bashrc"
    fi
}

save_vars="USE USE_EXPAND USE_EXPAND_HIDDEN ${USE_EXPAND}"
default_save_vars="CONFIG_PROTECT CONFIG_PROTECT_MASK"

for var in ${save_vars} ${default_save_vars} ; do
    ebuild_notice "debug" "Saving ${var}=${!var}"
    eval "export save_var_${var}='${!var}'"
done

if [[ -n "${PALUDIS_PROFILE_DIRS}" ]] ; then
    for var in ${PALUDIS_PROFILE_DIRS} ; do
        ebuild_source_profile $(canonicalise "${var}")
    done
elif [[ -n "${PALUDIS_PROFILE_DIR}" ]] ; then
    ebuild_source_profile $(canonicalise "${PALUDIS_PROFILE_DIR}")
fi

unset ${save_vars}

for f in ${PALUDIS_BASHRC_FILES} ; do
    if [[ -f ${f} ]] ; then
        ebuild_notice "debug" "Loading bashrc file ${f}"
        source ${f}
    else
        ebuild_notice "debug" "Skipping bashrc file ${f}"
    fi
done

for var in ${save_vars} ; do
    if [[ -n ${!var} ]] ; then
        die "${var} should not be set in bashrc. Aborting."
    fi
done

for var in ${save_vars} ; do
    eval "export ${var}=\${save_var_${var}}"
done

for var in ${default_save_vars} ; do
    if [[ -z ${!var} ]] ; then
        eval "export ${var}=\${save_var_${var}}"
    else
        ebuild_notice "debug" "Not restoring ${var}"
    fi
done

[[ -z "${CBUILD}" ]] && export CBUILD="${CHOST}"

ebuild_load_ebuild()
{
    if [[ -n "${PALUDIS_LOAD_ENVIRONMENT}" ]] ; then
        [[ -d ${PALUDIS_TMPDIR} ]] \
            || die "You need to create PALUDIS_TMPDIR (${PALUDIS_TMPDIR})."

        local save_PALUDIS_EXTRA_DIE_MESSAGE="${PALUDIS_EXTRA_DIE_MESSAGE}"
        local real_PALUDIS_EXTRA_DIE_MESSAGE="
!!! Could not extract the saved environment file. This is usually
!!! caused by a broken environment.bz2 that was generated by an old
!!! Portage version. The file that needs repairing is:
!!!     ${PALUDIS_LOAD_ENVIRONMENT}
!!! Try copying this file, bunzip2ing it and sourcing it using a new
!!! bash shell (do not continue to use said shell afterwards). You
!!! should get an error that give you a rough idea of where the
!!! problem lies.
"
        export PALUDIS_EXTRA_DIE_MESSAGE="${real_PALUDIS_EXTRA_DIE_MESSAGE}"

        bunzip2 < "${PALUDIS_LOAD_ENVIRONMENT}" > ${PALUDIS_TMPDIR}/environment-${CATEGORY}-${PF} \
            || die "Can't extract ${PALUDIS_LOAD_ENVIRONMENT}"

        sed -i \
            -e '/^diefunc ()/,/^}/d' \
            -e '/^perform_hook ()/,/^}/d' \
            -e '/^inherit ()/,/^}/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?ROOTPATH=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PATH=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?T=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PALUDIS_TMPDIR=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PALUDIS_EBUILD_LOG_LEVEL=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PORTDIR=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?FILESDIR=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?ECLASSDIR=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?DISTDIR=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PALUDIS_EBUILD_DIR=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PALUDIS_COMMAND=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?PALUDIS_HOME=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?EBUILD_KILL_PID=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?ROOT=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?SANDBOX/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?BASH_/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?SHELLOPTS/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?..\?ID=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?LD_/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?CATEGORY=/d' \
            -e '/^\(declare \(-[^ ]\+ \)\?\)\?\(PN\|PV\|P\|PVR\|PF\)=/d' \
            "${PALUDIS_TMPDIR}/environment-${CATEGORY}-${PF}"

        export PALUDIS_EXTRA_DIE_MESSAGE="${real_PALUDIS_EXTRA_DIE_MESSAGE}"

        echo source "${PALUDIS_TMPDIR}/environment-${CATEGORY}-${PF}" 1>&2
        source "${PALUDIS_TMPDIR}/environment-${CATEGORY}-${PF}" \
            || die "Can't load saved environment"

        export PALUDIS_EXTRA_DIE_MESSAGE="${save_PALUDIS_EXTRA_DIE_MESSAGE}"

        rm "${PALUDIS_TMPDIR}/environment-${CATEGORY}-${PF}"
    fi

    export EBUILD="${1}"
    unset IUSE DEPEND RDEPEND PDEPEND KEYWORDS

    if [[ "${CATEGORY}" == "virtual" ]] ; then
        if [[ -f "${1}" ]] ; then
            source ${1} || die "Error sourcing ebuild '${1}'"
        elif [[ -e "${1}" ]] ; then
            die "'${1}' exists but is not a regular file"
        fi
    else
        [[ -f "${1}" ]] || die "Ebuild '${1}' is not a file"
        source ${1} || die "Error sourcing ebuild '${1}'"
    fi
    [[ ${RDEPEND-unset} == "unset" ]] && RDEPEND="${DEPEND}"

    IUSE="${IUSE} ${E_IUSE}"
    DEPEND="${DEPEND} ${E_DEPEND}"
    RDEPEND="${RDEPEND} ${E_RDEPEND}"
    PDEPEND="${PDEPEND} ${E_PDEPEND}"
    KEYWORDS="${KEYWORDS} ${E_KEYWORDS}"
    [[ ${EAPI-unset} == "unset" ]] && EAPI="0"
}

perform_hook()
{
    export HOOK=${1}
    ebuild_notice "debug" "Starting hook '${HOOK}'"

    local old_sandbox_on="${SANDBOX_ON}"
    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && export SANDBOX_ON="0"

    local hook_dir
    for hook_dir in ${PALUDIS_HOOK_DIRS} ; do
        [[ -d "${hook_dir}/${HOOK}" ]] || continue
        local hook_file
        for hook_file in "${hook_dir}/${HOOK}/"*.bash ; do
            [[ -e "${hook_file}" ]] || continue
            ebuild_notice "debug" "Starting hook script '${hook_file}' for '${HOOK}'"
            if ! ( source "${hook_file}" ) ; then
                ebuild_notice "warning" "Hook '${hook_file}' returned failure"
            else
                ebuild_notice "debug" "Hook '${hook_file}' returned success"
            fi
        done
    done

    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && export SANDBOX_ON="${old_sandbox_on}"
    true
}

ebuild_main()
{
    if ! [[ -e /proc/self ]] && [[ "$(uname -s)" == Linux ]] ; then
        ebuild_notice "warning" "/proc appears to be unmounted or unreadable."
        ebuild_notice "warning" "This will cause problems."
    fi

    local action ebuild="$1"
    shift

    for action in $@ ; do
        case ${action} in
            metadata|variable|init|fetch|merge|unmerge|tidyup|strip|init_bin|unpack_bin|fetch_bin)
                ebuild_load_module builtin_${action}
            ;;

            unpack|compile|install|test)
                ebuild_load_module src_${action}
            ;;

            setup|config|nofetch|preinst|postinst|prerm|postrm)
                ebuild_load_module pkg_${action}
            ;;

            *)
                ebuild_load_module usage_error
                ebuild_f_usage_error "Unknown action '${action}'"
                exit 1
            ;;
        esac
    done

    if [[ $1 == metadata ]] || [[ $1 == variable ]] ; then
        perform_hook ebuild_${action}_pre
        if [[ $1 != variable ]] || [[ -n "${ebuild}" ]] ; then
            for f in cut tr date ; do
                eval "export ebuild_real_${f}=\"$(which $f )\""
                eval "${f}() { ebuild_notice qa 'global scope ${f}' ; $(which $f ) \"\$@\" ; }"
            done
            PATH="" ebuild_load_ebuild "${ebuild}"
        fi
        if ! ebuild_f_${1} ; then
            perform_hook ebuild_${action}_fail
            die "${1} failed"
        fi
        perform_hook ebuild_${action}_post
    else
        if [[ "${ebuild%.ebin}" == "${ebuild}" ]] ; then
            ebuild_load_ebuild "${ebuild}"
        fi
        for action in $@ ; do
            export EBUILD_PHASE="${action}"
            perform_hook ebuild_${action}_pre
            if ! ebuild_f_${action} ; then
                perform_hook ebuild_${action}_fail
                die "${action} failed"
            fi
            if [[ ${action} == "init" ]] ; then
                ebuild_load_ebuild "${ebuild}"
            fi
            perform_hook ebuild_${action}_post
        done
    fi
}

ebuild_main $@

