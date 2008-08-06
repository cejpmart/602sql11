rem Script for building PCRE using MS VC tools. Call vcvars.bat before this script.
rem /MT - use multithread static C library, /Fo, /Fd directories for obj and pdb, /Zi debug info, /O optimization
rem debug build -------------------------------------------------------
mkdir debug
cl -DSUPPORT_UTF8 -DSUPPORT_UCP -DPOSIX_MALLOC_THRESHOLD=10 -DPCRE_STATIC -I. /MTd /Zi /Od /Fo"debug/" /Fd"debug/" /c pcre_chartables.c pcre_compile.c pcre_config.c pcre_dfa_exec.c pcre_exec.c pcre_fullinfo.c pcre_get.c pcre_globals.c pcre_info.c pcre_maketables.c pcre_newline.c pcre_ord2utf8.c pcre_refcount.c pcre_study.c pcre_tables.c pcre_try_flipped.c pcre_ucp_searchfuncs.c pcre_valid_utf8.c pcre_version.c pcre_xclass.c
cd debug
lib /OUT:pcre.lib pcre_chartables.obj pcre_compile.obj pcre_config.obj pcre_dfa_exec.obj pcre_exec.obj pcre_fullinfo.obj pcre_get.obj pcre_globals.obj pcre_info.obj pcre_maketables.obj pcre_newline.obj pcre_ord2utf8.obj pcre_refcount.obj pcre_study.obj pcre_tables.obj pcre_try_flipped.obj pcre_ucp_searchfuncs.obj pcre_valid_utf8.obj pcre_version.obj pcre_xclass.obj
cd ..
rem release build -----------------------------------------------------
mkdir release
cl -DSUPPORT_UTF8 -DSUPPORT_UCP -DPOSIX_MALLOC_THRESHOLD=10 -DPCRE_STATIC -I. /MT /O2 /Fo"release/" /Fd"release/"  /c pcre_chartables.c pcre_compile.c pcre_config.c pcre_dfa_exec.c pcre_exec.c pcre_fullinfo.c pcre_get.c pcre_globals.c pcre_info.c pcre_maketables.c pcre_newline.c pcre_ord2utf8.c pcre_refcount.c pcre_study.c pcre_tables.c pcre_try_flipped.c pcre_ucp_searchfuncs.c pcre_valid_utf8.c pcre_version.c pcre_xclass.c
cd release
lib /OUT:pcre.lib pcre_chartables.obj pcre_compile.obj pcre_config.obj pcre_dfa_exec.obj pcre_exec.obj pcre_fullinfo.obj pcre_get.obj pcre_globals.obj pcre_info.obj pcre_maketables.obj pcre_newline.obj pcre_ord2utf8.obj pcre_refcount.obj pcre_study.obj pcre_tables.obj pcre_try_flipped.obj pcre_ucp_searchfuncs.obj pcre_valid_utf8.obj pcre_version.obj pcre_xclass.obj
cd ..
