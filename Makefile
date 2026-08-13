BUILD ?= seashell

include $(BUILD).mk

.PHONY: test clean

test:
	$(MAKE) BUILD=seashell
	$(MAKE) BUILD=test

clean:
	rm -rf seashell test_all build compile_commands.json
