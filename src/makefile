!include <win32.mak>

PRJNAME=indent

LIB_FILES= KERNEL32.lib USER32.lib SHELL32.LIB ADVAPI32.LIB

CPP_FILES= indent.cpp

OBJS= $(CPP_FILES:.cpp=.obj)

all: ver x_exe

.cpp.obj:
	$(cc) -Ox $(cflags) $(cvarsdll) -DSTRICT -GX -YX -DNODEBUG $<

ver: 
	-SubWCRev . version.hxx tmp\version.h

x_exe: $(OBJS)
	$(link) /dll /NODEFAULTLIB:LIBCMT.LIB /RELEASE /VERSION:2.4 -out:$(PRJNAME).dll $(OBJS) $(LIB_FILES) /DEF:$(PRJNAME).def
	-copy *.dll ..\setup

clean:
	-del *.obj
	-del *.pch
	-del *.exp
	-del *.lib
	-del *.res
