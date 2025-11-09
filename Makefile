BIN := gpio-toggle
SRC := main.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	setcap cap_ipc_lock=+ep $@

clean:
	rm -rf gpio-toggle
