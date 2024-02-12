CXX := g++-13
CXXFLAGS := -std=c++23
LDFLAGS := -lstdc++

SRCDIR := src
OBJDIR := obj

SRC := $(wildcard $(SRCDIR)/*.cc)
OBJ := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(SRC))
BIN := netcp

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	@echo "Enlazando $^ --> $@"
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	@echo "Compilando $< --> $@"
	@mkdir -p $(OBJDIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Limpiando..."
	@rm -rf $(OBJDIR) $(BIN)

send:
	./netcp -o testfile

recieve:
	./netcp -l testfile2

test_1K:
	dd if=/dev/urandom of=testfile bs=1K count=1 iflag=fullblock
