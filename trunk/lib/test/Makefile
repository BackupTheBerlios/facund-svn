PROG=	tests
SRCS=	main.c test_facund_connection.c test_facund_object.c \
	test_facund_response.c test_facund_server.c

WARNS?=		6
CLEANFILES+=	tests.core

# libfacund
CFLAGS+=	-I${.CURDIR}/..
LDADD+=		${.OBJDIR}/../libfacund.a -lbsdxml

# libcheck
CFLAGS+=	-I/usr/local/include
LDADD+=		-L/usr/local/lib -lcheck

MAN=

.include <bsd.prog.mk>
