ls2k-unlock-io: ls2k-unlock-io.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f ls2k-unlock-io

install: ls2k-unlock-io
	install -D -m 755 ./ls2k-unlock-io $(DESTDIR)/usr/sbin/ls2k-unlock-io

all: ls2k-unlock-io

.PHONY: all install
