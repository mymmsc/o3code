# the samples is tcp sock5

noinst_PROGRAMS  = o3code
o3code_SOURCES   = bli_base.c bli_code.c bli_gf256.c bli_random.c netcode.c test-netcode.c
o3code_INCLUDES  = -I$(top_srcdir)/include -I$(top_srcdir)/event
o3code_CFLAGS    =  $(CFLAGS) $(o3code_INCLUDES)
o3code_LDADD     = $(top_srcdir)/base/libbase.la -lpthread

