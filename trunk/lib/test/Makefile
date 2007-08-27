PROG=	tests
SRCS=	main.c test_facund_connection.c

CFLAGS+=	-I/usr/local/include
LDADD+=		-L/usr/local/lib -lcheck ${.OBJDIR}/../libfacund.a

MAN=

.include <bsd.prog.mk>
