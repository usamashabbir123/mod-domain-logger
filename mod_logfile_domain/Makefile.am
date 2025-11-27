include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_logfile_domain

mod_LTLIBRARIES = mod_logfile_domain.la
mod_logfile_domain_la_SOURCES  = mod_logfile_domain.c
mod_logfile_domain_la_CFLAGS   = $(AM_CFLAGS)
mod_logfile_domain_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_logfile_domain_la_LDFLAGS  = -avoid-version -module -no-undefined -shared

$(MODULE_OBJECT): $(MODULE_SOURCE)
	$(CC) $(CFLAGS) -I$(FREESWITCH_INCLUDE) -c $(MODULE_SOURCE) -o $(MODULE_OBJECT)

$(MODULE_TARGET): $(MODULE_OBJECT)
	$(CC) -shared $(MODULE_OBJECT) -o $(MODULE_TARGET) $(LDFLAGS)
	@echo "Module built successfully: $(MODULE_TARGET)"

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
