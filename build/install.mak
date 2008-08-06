# Makefile for the install project on the MSW platform
# Creates the installation tree in the $(INST_ROOT) ditrectory

all: $(INST_ROOT) MAKE_INF
  rem Delete old files:
  del /Q $(INST_ROOT)\*.exe  $(INST_ROOT)\*.dll  $(INST_ROOT)\*.lib $(INST_ROOT)\*.chm
  rem Copy project binaries
  copy $(OUT_DIR)\$(NAME_PREFIX)gcli$(NAME_SUFFIX).exe $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)ccli$(NAME_SUFFIX).exe $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)krnl$(NAME_SUFFIX).dll $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)krnl$(NAME_SUFFIX).lib $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)krnllib$(NAME_SUFFIX).lib $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)odbcw$(NAME_SUFFIX).dll $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)sql$(NAME_SUFFIX).exe $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)svc$(NAME_SUFFIX).exe $(INST_ROOT)
  copy $(OUT_DIR)\$(NAME_PREFIX)mng$(NAME_SUFFIX).exe $(INST_ROOT)
  rem Copy external libraries
  if exist $(OUT_DIR)\libintl3.dll  copy $(OUT_DIR)\libintl3.dll  $(INST_ROOT)
  if exist $(OUT_DIR)\libiconv2.dll copy $(OUT_DIR)\libiconv2.dll $(INST_ROOT)
  rem Create language directories and copy language files
  for %%L in ($(LANGS)) do if exist ..\src\gcli\%%L.mo if not exist $(INST_ROOT)\%%L mkdir $(INST_ROOT)\%%L
  for %%L in ($(LANGS)) do if exist ..\src\gcli\%%L.mo if not exist $(INST_ROOT)\%%L\LC_MESSAGES mkdir $(INST_ROOT)\%%L\LC_MESSAGES
  for %%L in ($(LANGS)) do if exist ..\src\gcli\%%L.mo del /Q $(INST_ROOT)\%%L\LC_MESSAGES\*.*
  for %%L in ($(LANGS)) do if exist ..\src\gcli\%%L.mo copy ..\src\gcli\%%L.mo $(INST_ROOT)\%%L\LC_MESSAGES\$(NAME_PREFIX)gcli$(NAME_SUFFIX).mo
  for %%L in ($(LANGS)) do if exist ..\src\mng\%%L.mo  copy ..\src\mng\%%L.mo $(INST_ROOT)\%%L\LC_MESSAGES\$(NAME_PREFIX)mng$(NAME_SUFFIX).mo
  rem Copy WX language files (compile them manually in the $(WXDIR)\locale directory using msgfmt or poedit):
  for %%L in ($(LANGS)) do if exist ..\$(WXDIR)\locale\%%L.mo  copy ..\$(WXDIR)\locale\%%L.mo $(INST_ROOT)\%%L\LC_MESSAGES\wxstd.mo
  rem Copy help files from the doc\help directory
  copy ..\doc\help\help-en.chm $(INST_ROOT)\$(NAME_PREFIX)sql$(NAME_SUFFIX)en.chm
  for %%L in ($(LANGS)) do if exist ..\doc\help\help-%%L.chm copy ..\doc\help\help-%%L.chm $(INST_ROOT)\$(NAME_PREFIX)sql$(NAME_SUFFIX)%%L.chm

$(INST_ROOT):
  mkdir $(INST_ROOT)

MAKE_INF: $(INST_ROOT)
  rem <<$(INST_ROOT)\version.inf
CONF_NAME=$(CONF_NAME)
OUT_DIR=$(OUT_DIR)
NAME_PREFIX=$(NAME_PREFIX)
NAME_SUFFIX=$(NAME_SUFFIX)
LANGS=$(LANGS)
<<KEEP

