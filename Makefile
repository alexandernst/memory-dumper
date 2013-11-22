CC			=	@gcc
RM			=	@rm

TARGET		=	mem_dumper
PLUGINS_DIR	=	plugins
PLUGINS_C	=	$(wildcard $(PLUGINS_DIR)/*.c)
PLUGINS_O	=	$(patsubst %.c,%.o, $(PLUGINS_C))
PLUGINS_SO	=	$(patsubst %.c,%.so, $(PLUGINS_C))

.PHONY: plugins

new: clean all

all: $(TARGET) $(PLUGINS_SO) $(PLUGINS_O)

$(TARGET): utils.o $(TARGET).o
	$(CC) -o $@ $^ -g -Wall -std=gnu99 -ldl -lcrypto -Wl,--export-dynamic

$(PLUGINS_DIR)/%.so: $(PLUGINS_DIR)/%.o
	$(CC) $^ -o $@ -shared

$(PLUGINS_DIR)/%.o: $(PLUGINS_DIR)/%.c
	$(CC) -c $< -o $@ -pedantic -g -Wall -std=gnu99 -fpic -I.

clean:
	$(RM) -rf $(TARGET) *.o *.a $(PLUGINS_O) $(PLUGINS_SO)