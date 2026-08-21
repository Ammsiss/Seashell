BUILD ?= seashell

include config/$(BUILD).mk

.PHONY: test clean

pty:
	$(MAKE) BUILD=seashell
	$(MAKE) BUILD=pty

unit:
	$(MAKE) BUILD=unit

clean:
	rm -rf seashell pty_test unit_test build compile_commands.json
