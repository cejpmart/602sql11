rem Script for building PCRE using MS VC tools. Call vcvars.bat before this script.
cl -DSUPPORT_UTF8 -DSUPPORT_UCP -I. dftables.c
dftables.exe pcre_chartables.c