CC			=	@gcc
CPP			=	@g++
RM			=	@rm

TARGET		=	mem_dumper
PLUGINS_DIR	=	plugins
LIBS_DIR	=	libs
PLUGINS_CPP	=	$(wildcard $(PLUGINS_DIR)/*.cpp)
PLUGINS_O	=	$(patsubst %.cpp,%.o, $(PLUGINS_CPP))
PLUGINS_SO	=	$(patsubst %.cpp,%.so, $(PLUGINS_CPP))

.PHONY: plugins

new: clean all

all: $(TARGET) $(PLUGINS_SO) $(PLUGINS_O)

$(TARGET): $(TARGET).o
	$(CC) -o $@ $^ -g -Wall -std=gnu99 -ldl -lcrypto -Wl,--export-dynamic

$(PLUGINS_DIR)/%.so: $(PLUGINS_DIR)/%.o
	$(CXX) $^ -o $@ -shared -L$(LIBS_DIR)/cpp-bitstring -Wl,-R$(LIBS_DIR)/cpp-bitstring -lcpp-bitstring

$(PLUGINS_DIR)/%.o: $(PLUGINS_DIR)/%.cpp
	$(CXX) -c $< -o $@ -g -Wall -std=c++11 -pedantic -fPIC -I.

clean:
	$(RM) -rf $(TARGET) *.o *.a $(PLUGINS_O) $(PLUGINS_SO)