HEADERS := $(shell find include -name \*.[ch]pp)
SRC := $(shell find test -name \*.[ch]pp) $(HEADERS)

all:
	@echo Please, use CMake instead.

format:
	@echo Formatting source...
	@clang-format -i -style=file $(SRC)

.PHONY: format
