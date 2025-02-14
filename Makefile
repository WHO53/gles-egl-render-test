PKGS = wayland-client glesv2 pangocairo egl wayland-egl

GLES_TEST_SOURCES += render.c
GLES_TEST_HEADERS += render.h

CFLAGS += -std=gnu99 -Wall -g -DWITH_WAYLAND_SHM
CFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS)) -lm -lutil -lrt -lz

WAYLAND_HEADERS = $(wildcard proto/*.xml)

HDRS = $(WAYLAND_HEADERS:.xml=-client-protocol.h)
WAYLAND_SRC = $(HDRS:.h=.c)

gles2_SOURCES = $(GLES_TEST_SOURCES) $(WAYLAND_SRC) gles2.c
gles2_OBJECTS = $(gles2_SOURCES:.c=.o)

gles3_SOURCES = $(GLES_TEST_SOURCES) $(WAYLAND_SRC) gles3.c
gles3_OBJECTS = $(gles3_SOURCES:.c=.o)

all: gles2 gles3

gles2: $(gles2_OBJECTS)
	$(CC) -o gles2 $(gles2_OBJECTS) $(LDFLAGS) -DWITH_GLES2

gles3: $(gles3_OBJECTS)
	$(CC) -o gles3 $(gles3_OBJECTS) $(LDFLAGS) -DWITH_GLES3

proto/%-client-protocol.c: proto/%.xml
	wayland-scanner code < $? > $@

proto/%-client-protocol.h: proto/%.xml
	wayland-scanner client-header < $? > $@

$(gles2_OBJECTS) $(gles3_OBJECTS): $(HDRS) $(GLES_TEST_HEADERS)

clean:
	rm -f $(gles2_OBJECTS) $(gles3_OBJECTS) $(HDRS) $(WAYLAND_SRC) gles2 gles3

format:
	clang-format -i $(GLES_TEST_SOURCES) $(GLES_TEST_HEADERS)
