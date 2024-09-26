NAME=test
BIN=${NAME}
SRC=.

PKGS = wayland-client glesv2 pangocairo egl wayland-egl

WVKBD_SOURCES += $(wildcard $(SRC)/*.c)
WVKBD_HEADERS += $(wildcard $(SRC)/*.h)

CFLAGS += -std=gnu99 -Wall -g -DWITH_WAYLAND_SHM
CFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS)) -lm -lutil -lrt -lz

WAYLAND_HEADERS = $(wildcard proto/*.xml)

HDRS = $(WAYLAND_HEADERS:.xml=-client-protocol.h)
WAYLAND_SRC = $(HDRS:.h=.c)
SOURCES = $(WVKBD_SOURCES) $(WAYLAND_SRC)

OBJECTS = $(SOURCES:.c=.o)

all: ${BIN}

proto/%-client-protocol.c: proto/%.xml
	wayland-scanner code < $? > $@

proto/%-client-protocol.h: proto/%.xml
	wayland-scanner client-header < $? > $@

$(OBJECTS): $(HDRS) $(WVKBD_HEADERS)

$(NAME): $(OBJECTS)
	$(CC) -o $(NAME) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(HDRS) $(WAYLAND_SRC) ${BIN}

format:
	clang-format -i $(WVKBD_SOURCES) $(WVKBD_HEADERS)
