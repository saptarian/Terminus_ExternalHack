BUILD_DIR = build
OUTPUT = $(BUILD_DIR)\Terminus_ExternalHack.exe
SRCS = util.cpp gmanager.cpp entry.cpp custom.cpp main.cpp sample.rc
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)

CXX = i686-w64-mingw32-g++
CXXFLAGS = -O2 -W -Wall -std=c++20 -mthreads -MD -MP \
	-D__WXMSW__ -D_UNICODE -DNDEBUG
LDFLAGS = -Wl,--subsystem,windows -static \
	-static-libgcc -static-libstdc++ -mwindows
CXXINCLUDES = -I.. -I..\include -I..\lib\Win32\Release\mswu \
	-ID:\suspect\me\CProject\json\include \
	-IC:\Users\LENOVO\.conda\envs\py379\include
WXLIBS=-L..\lib\Win32\Release -lwxmsw32u_core -lwxbase32u \
	-lwxtiff -lwxjpeg -lwxpng -lwxzlib -lwxregexu -lwxexpat
WINLIBS=-lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinspool -lwinmm \
	-lshell32 -lshlwapi -lcomctl32 -lole32 -loleaut32 -luuid -lrpcrt4 \
	-ladvapi32 -lversion -lws2_32 -lwininet -loleacc -luxtheme

### Targets: ###

all: $(OUTPUT)

clean:
	-if exist $(BUILD_DIR)\*.o del $(BUILD_DIR)\*.o
	-if exist $(OUTPUT) del $(OUTPUT)

$(OUTPUT): $(OBJS) $(BUILD_DIR)\sample.rc.o
	$(foreach f,$(subst \,/,$(OBJS)),$(shell echo $f >> $(subst \,/,$@).rsp.tmp))
	@move /y $@.rsp.tmp $@.rsp >nul
	$(CXX) -o $@ @$@.rsp $(LDFLAGS) $(WXLIBS) $(WINLIBS)
	@-del $@.rsp
	$(OUTPUT)

$(BUILD_DIR)/sample.rc.o: ..\sample.rc
	windres -i$< -o$@ --define wxUSE_DPI_AWARE_MANIFEST=2 --define wxUSE_NO_MANIFEST=0 $(CXXINCLUDES)

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CXXINCLUDES)

.PHONY: all clean