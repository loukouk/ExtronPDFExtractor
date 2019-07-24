CXX = gcc
CFLAGS = -Wall -pedantic-errors -O2 -std=gnu99
TARGET = pkp2pdf


default:
	${CXX} ${CFLAGS} ${TARGET}.c -o ${TARGET}.exe

debug:
	${CXX} ${CFLAGS} ${TARGET}.c -o ${TARGET}.exe -DEBUG