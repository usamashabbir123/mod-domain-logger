ACLOCAL_AMFLAGS = -I m4

if BUILD_MODDIR
moddir = $(MODDIR)
else
moddir = $(libdir)/freeswitch/mod
endif

AM_CPPFLAGS = $(FREESWITCH_CFLAGS)

mod_LTLIBRARIES = mod_logfile_domain.la
mod_logfile_domain_la_SOURCES  = mod_logfile_domain.c
mod_logfile_domain_la_CFLAGS   = $(AM_CFLAGS) $(FREESWITCH_CFLAGS)
mod_logfile_domain_la_LIBADD   = $(FREESWITCH_LIBS)
mod_logfile_domain_la_LDFLAGS  = -avoid-version -module -no-undefined -shared

install: $(MODULE_TARGET)
	@echo "Installing $(MODULE_TARGET) to $(INSTALL_MOD_PATH)"
	mkdir -p $(INSTALL_MOD_PATH)
	cp $(MODULE_TARGET) $(INSTALL_MOD_PATH)/
	@echo "Installing modules.conf.xml to $(INSTALL_CONF_PATH)"
	mkdir -p $(INSTALL_CONF_PATH)
	cp modules.conf.xml $(INSTALL_CONF_PATH)/mod_$(MODULE_NAME).conf.xml
	@echo "Installation complete!"

cmake-build:
	mkdir -p build
	cd build && cmake .. && make
	@echo "CMake build complete! Output in build/lib/"
