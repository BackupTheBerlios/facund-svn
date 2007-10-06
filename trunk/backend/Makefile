PROG=	facund-be

CFLAGS+=-pthread -I${.CURDIR}/../lib
LDADD+=	-lbsdxml -lutil -lmd ${.OBJDIR}/../lib/libfacund.a

MAN=

WARNS?=	6

.include <bsd.prog.mk>
