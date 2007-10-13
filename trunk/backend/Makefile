PROG=	facund-be
SRCS=	facund-be.c facund-comms.c

CFLAGS+=-pthread -I${.CURDIR}/../lib
LDADD+=	-lbsdxml -lutil -lmd ${.OBJDIR}/../lib/libfacund.a

MAN=

WARNS?=	6

.include <bsd.prog.mk>
