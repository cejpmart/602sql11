Makefile: $(top_dir)/build/$(ownsubdir)/Makefile.in ../config.status
	cd ..; ./config.status

%.d: %.cpp
	set -e; $(CC) -M $(CPPFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

%.d: %.c
	set -e; $(CC) -M $(CPPFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

