# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmvectorize

SRCDIR=	lib/Transforms/Vectorize
SRCS=	BBVectorize.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	Vectorize.cpp

TGHDRS=	Attributes \
	Intrinsics

.include "../clang.lib.mk"
