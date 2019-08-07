GPP      = g++ -std=gnu++14 -g -O0 -Wall -Wextra
MKDEP    = g++ -std=gnu++14 -MM

MKFILE   = Makefile
DEPFILE  = Makefile.dep
SOURCES  = string_set.cpp main.cpp auxlib.cpp lyutils.cpp \
			astree.cpp yylex.cpp yyparse.cpp symtable.cpp
HEADERS  = string_set.h auxlib.h lyutils.h atress.h symtable.h
OBJECTS  = ${SOURCES:.cpp=.o}
EXECBIN  = oc
SRCFILES = ${HEADERS} ${SOURCES} ${MKFILE}

all : dep ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GPP} ${OBJECTS} -o ${EXECBIN}

yylex.o : yylex.cpp
	${GPP} -Wno-sign-compare -c $<

%.o : %.cpp
	${GPP} -c $<

yylex.cpp : scanner.l
	flex -oyylex.cpp scanner.l 2>yylex.log
	- grep -v '^ ' yylex.log

yyparse.cpp : parser.y
	bison --defines=yyparse.h -oyyparse.cpp parser.y
	- mv $*output $*.log

ci :
	git add ${SRCFILES}

clean :
	-rm ${OBJECTS} ${DEPFILE} yyparse.cpp yyparse.h yyparse.output

spotless : clean
	- rm ${EXECBIN}

${DEPFILE} : ${SOURCES}
	${MKDEP} ${SOURCES} >${DEPFILE}

dep :
	- rm ${DEPFILE}
	${MAKE} --no-print-directory ${DEPFILE}
