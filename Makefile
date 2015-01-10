CC			=	@gcc
CPP			=	@g++
RM			=	@rm

TARGET		=	mem_dumper
PLUGINS_DIR	=	plugins
PLUGINS_CPP	=	$(wildcard $(PLUGINS_DIR)/*.cpp)
PLUGINS_O	=	$(patsubst %.cpp,%.o, $(PLUGINS_CPP))
PLUGINS_SO	=	$(patsubst %.cpp,%.so, $(PLUGINS_CPP))

.PHONY: plugins

new: clean all

all: $(TARGET) $(PLUGINS_SO) $(PLUGINS_O)

$(TARGET): $(TARGET).o
	$(CC) -o $@ $^ -g -Wall -std=gnu99 -ldl -lcrypto -Wl,--export-dynamic

$(PLUGINS_DIR)/%.so: $(PLUGINS_DIR)/%.o
	$(CPP) $^ -o $@ -shared

$(PLUGINS_DIR)/%.o: $(PLUGINS_DIR)/%.cpp
	$(CPP) -c $< -o $@ -pedantic -g -Wall -fPIC -I.

clean:
	$(RM) -rf $(TARGET) *.o *.a $(PLUGINS_O) $(PLUGINS_SO)