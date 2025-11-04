BIN := gpio-toggle
SRC := main.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(BIN)
