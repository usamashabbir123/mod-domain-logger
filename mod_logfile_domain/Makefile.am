ACLOCAL_AMFLAGS = -I m4

moddir = $(libdir)/freeswitch/mod
confdir = $(sysconfdir)/freeswitch/autoload_configs

AM_CPPFLAGS = $(FREESWITCH_CFLAGS)

mod_LTLIBRARIES = mod_logfile_domain.la
mod_logfile_domain_la_SOURCES  = mod_logfile_domain.c
mod_logfile_domain_la_CFLAGS   = $(AM_CFLAGS) $(FREESWITCH_CFLAGS)
mod_logfile_domain_la_LIBADD   = $(FREESWITCH_LIBS)
mod_logfile_domain_la_LDFLAGS  = -avoid-version -module -no-undefined -shared

conf_DATA = conf/autoload_configs/logfile_domain.conf.xml
	@echo "CMake build complete! Output in build/lib/"
