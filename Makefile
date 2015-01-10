CC			=	@gcc
RM			=	@rm

TARGET		=	mem_dumper
PLUGINS_DIR	=	plugins
C_PLUGINS_C	=	$(wildcard $(PLUGINS_DIR)/*.c)
C_PLUGINS_O	=	$(patsubst %.c,%.o, $(C_PLUGINS_C))
C_PLUGINS_SO	=	$(patsubst %.c,%.so, $(C_PLUGINS_C))

.PHONY: plugins

new: clean all

all: $(TARGET) $(C_PLUGINS_SO) $(C_PLUGINS_O)

$(TARGET): utils.o $(TARGET).o
	$(CC) -o $@ $^ -g -Wall -std=gnu99 -ldl -lcrypto -Wl,--export-dynamic

$(PLUGINS_DIR)/%.so: $(PLUGINS_DIR)/%.o
	$(CC) $^ -o $@ -shared

$(PLUGINS_DIR)/%.o: $(PLUGINS_DIR)/%.c
	$(CC) -c $< -o $@ -pedantic -g -Wall -std=gnu99 -fpic -I.

clean:
	$(RM) -rf $(TARGET) *.o *.a $(C_PLUGINS_O) $(C_PLUGINS_SO)