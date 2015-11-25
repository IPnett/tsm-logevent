

TARGETS=	dsmlogevent

CPPFLAGS=	-I/opt/tivoli/tsm/client/api/bin64/sample
LDFLAGS=	-L/opt/tivoli/tsm/client/api/bin64
LIBS=		-lApiTSM64

all: $(TARGETS)

dsmlogevent: dsmlogevent.o
	$(CC) -o $@ $(LDFLAGS) $(LIBS) $<

clean:
	rm -rf *.o $(TARGETS)
