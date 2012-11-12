CFLAGS = `pkg-config gtk+-3.0 gmodule-2.0 gel-1.0 --cflags`
LIBS = `pkg-config gtk+-3.0 gmodule-2.0 gel-1.0 --libs`

gel-draw: gel-draw.o
	$(CC) $< -o $@ $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f gel-draw.o gel-draw 

.PHONY = clean

