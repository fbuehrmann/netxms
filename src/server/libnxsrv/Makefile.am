lib_LTLIBRARIES = libnxsrv.la
libnxsrv_la_SOURCES = agent.cpp apinfo.cpp arp.cpp console.cpp \
                      entity_mib.cpp hdlink.cpp hostmib.cpp \
                      iflist.cpp isc.cpp main.cpp ndd.cpp \
                      nobject.cpp nxtool.cpp snmpproxy.cpp \
                      vlan.cpp
libnxsrv_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -DLIBNXSRV_EXPORTS -I@top_srcdir@/build
libnxsrv_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsrv_la_LIBADD = \
	../../db/libnxdb/libnxdb.la \
	../../libnxsl/libnxsl.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnetxms/libnetxms.la 

if USE_INTERNAL_JANSSON
libnxsrv_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
libnxsrv_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	libnxsrv.vcxproj libnxsrv.vcxproj.filters \
	libnxsrv.h \
	libnxsrv.rc \
	messages.mc \
	resource.h \
	Makefile.w32
