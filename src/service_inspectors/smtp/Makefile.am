AUTOMAKE_OPTIONS=foreign no-dependencies

file_list = \
smtp_config.h \
smtp.cc \
smtp.h \
smtp_util.cc \
smtp_util.h \
smtp_xlink2state.cc \
smtp_xlink2state.h \
smtp_normalize.cc \
smtp_normalize.h \
smtp_paf.cc \
smtp_paf.h \
smtp_module.cc \
smtp_module.h

if STATIC_INSPECTORS
noinst_LIBRARIES = libsmtp.a
libsmtp_a_SOURCES = $(file_list)
else
shlibdir = $(pkglibdir)/inspectors
shlib_LTLIBRARIES = libsmtp.la
libsmtp_la_CXXFLAGS = $(AM_CXXFLAGS) -DBUILDING_SO
libsmtp_la_LDFLAGS = -export-dynamic -shared
libsmtp_la_SOURCES = $(file_list)
endif

AM_CXXFLAGS = @AM_CXXFLAGS@

