PROG=	tests
SRCS=	main.c test_facund_connection.c test_facund_object.c

WARNS?=		6

# libfacund
CFLAGS+=	-I${.CURDIR}/..
LDADD+=		${.OBJDIR}/../libfacund.a

# libcheck
CFLAGS+=	-I/usr/local/include
LDADD+=		-L/usr/local/lib -lcheck

MAN=

.include <bsd.prog.mk>
