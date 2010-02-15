# Note: all dependencies of programs on files which are built by this Makefile
# have to be stated explicitly, even though they are also in the generated
# Makefiles. This is because (a) the generated Makefiles do not know how to
# build them, and (b) the prerequisites are not known to the generated
# Makefiles (for example a change in graphics/logo.png would not cause a
# rebuild of logo.xpm, if logo.xpm wasn't required by startkladde here).


##################
## Target lists ##
##################

# The programs (with an own .pro file) to be built
PROGRAMS = startkladde tests

# The XPM images used by the program
XPMS = build/kvkbd.xpm build/logo.xpm


#####################
## General targets ##
#####################

# By default, build just the startkladde program
.PHONY: default
default: startkladde

# Build all: the programs and the plugins
.PHONY: all
all: $(PROGRAMS) plugins


#########################
## Generic build rules ##
#########################

# The individual Makefiles are made from the corresponding project files by
# invoking qmake
Makefile_%: %.pro
	qmake-qt4 $<

# The programs are made by invoking the appropriate Makefile (see the note at
# the beginning of the file)
.PHONY: $(PROGRAMS)
$(PROGRAMS): %: Makefile_% version
	$(MAKE) -f Makefile_$@

$(XPMS): build/%.xpm: graphics/%.png
	mkdir -p build
	convert $< $@
	mv $@ $@.tmp
	sed 's/char/const char/g' <$@.tmp >$@
	rm $@.tmp


# Additional dependencies (see note at beginning of file)
startkladde: $(XPMS)


########################
## Individual targets ##
########################

Makefile_startkladde: build/migrations.pro

build:
	mkdir -p build

build/migrations.pro: migrations.pro.erb build
	erb $< >$@


.PHONY: version
version: version/version.h

version/version.h: version/major version/minor
	cd version; ./make_version; cd ..

programs: $(PROGRAMS)


####################
## Subdirectories ##
####################

.PHONY: plugins
plugins:
	$(MAKE) -C $@


#############
## Cleanup ##
#############

.PHONY: clean
clean: Makefile_startkladde Makefile_tests build/logo.xpm build/kvkbd.xpm
	$(MAKE) -f Makefile_startkladde distclean
	$(MAKE) -f Makefile_tests distclean
	$(MAKE) -C plugins clean
	rm -f Makefile_startkladde Makefile_tests
	rm -f version/version.h
	rm -rf build


##########
## Misc ##
##########

.PHONY: run
run: startkladde
	./startkladde
     
