dnl vim: set ft=m4 et :
dnl This file is used by Makefile.am.m4 and paludis.hh.m4. You should
dnl use the provided autogen.bash script to do all the hard work.
dnl
dnl This file is used to avoid having to make lots of repetitive changes in
dnl Makefile.am every time we add a source or test file. The first parameter is
dnl the base filename with no extension; later parameters can be `hh', `cc',
dnl `test', `impl', `testscript'. Note that there isn't much error checking done
dnl on this file at present...

add(`about',                             `hh', `test')
add(`action',                            `hh', `cc', `fwd', `se')
add(`condition_tracker',                 `hh', `cc')
add(`contents',                          `hh', `cc', `fwd')
add(`dep_label',                         `hh', `cc', `fwd')
add(`dep_list',                          `hh', `cc', `fwd', `sr', `test', `fwd')
add(`dep_list_exceptions',               `hh', `cc')
add(`dep_list_options',                  `hh', `cc', `se')
add(`dep_spec',                          `hh', `cc', `test', `fwd')
add(`dep_spec_flattener',                `hh', `cc')
add(`dep_tree',                          `hh', `cc', `fwd')
add(`dep_tag',                           `hh', `cc', `fwd', `sr')
add(`distribution',                      `hh', `cc', `fwd')
add(`environment',                       `hh', `fwd', `cc')
add(`environment_implementation',        `hh', `cc')
add(`environment_maker',                 `hh', `cc')
add(`find_unused_packages_task',         `hh', `cc')
add(`fuzzy_finder',                      `hh', `cc', `test')
add(`formatter',                         `hh', `fwd', `cc')
add(`handled_information',               `hh', `fwd', `cc')
add(`hashed_containers',                 `hh', `cc', `test')
add(`hook',                              `hh', `cc', `fwd',`se', `sr')
add(`hooker',                            `hh', `cc', `test', `testscript')
add(`host_tuple_name',                   `hh', `cc', `sr', `test')
add(`install_task',                      `hh', `cc', `se')
add(`literal_metadata_key',              `hh', `cc')
add(`mask',                              `hh', `cc', `fwd')
add(`match_package',                     `hh', `cc')
add(`merger',                            `hh', `cc', `fwd', `se', `test', `testscript')
add(`merger_entry_type',                 `hh', `cc', `se')
add(`metadata_key',                      `hh', `cc', `se', `fwd')
add(`metadata_key_holder',               `hh', `cc', `fwd')
add(`name',                              `hh', `cc', `fwd', `test', `sr', `se')
add(`ndbam',                             `hh', `cc', `fwd')
add(`ndbam_merger',                      `hh', `cc')
add(`ndbam_unmerger',                    `hh', `cc')
add(`override_functions',                `hh', `cc')
add(`package_database',                  `hh', `cc', `fwd', `test', `se')
add(`package_id',                        `hh', `cc', `fwd', `se')
add(`paludis',                           `hh')
add(`qa',                                `hh', `cc', `fwd', `se', `sr')
add(`query',                             `hh', `cc', `fwd')
add(`query_delegate',                    `hh', `cc', `fwd')
add(`query_visitor',                     `hh', `cc')
add(`range_rewriter',                    `hh', `cc', `test')
add(`report_task',                       `hh', `cc')
add(`repository',                        `hh', `fwd', `cc')
add(`repository_maker',                  `hh', `cc')
add(`repository_name_cache',             `hh', `cc', `test', `testscript')
add(`set_file',                          `hh', `cc', `se', `sr', `test', `testscript')
add(`show_suggest_visitor',              `hh', `cc')
add(`slot_requirement',                  `hh', `fwd', `cc')
add(`stage_builder_task',                `hh', `cc')
add(`stage_options',                     `sr')
add(`stringify_formatter',               `hh', `cc', `fwd', `impl', `test')
add(`stripper',                          `hh', `cc', `fwd', `test', `testscript')
add(`syncer',                            `hh', `cc')
add(`sync_task',                         `hh', `cc')
add(`tasks_exceptions',                  `hh', `cc')
add(`uninstall_list',                    `hh', `cc', `se', `sr', `test')
add(`uninstall_task',                    `hh', `cc')
add(`unmerger',                          `hh', `cc')
add(`user_dep_spec',                     `hh', `cc', `se', `fwd', `test')
add(`version_operator',                  `hh', `cc', `fwd', `se', `test')
add(`version_requirements',              `hh', `cc', `fwd', `sr')
add(`version_spec',                      `hh', `cc', `fwd', `test')

