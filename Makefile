CC = ${CROSS_COMPILE}gcc
CXX = ${CROSS_COMPILE}g++

CCACHE = ccache
CFLAGS = -Wall -Wextra -Wno-unused-parameter -O2 -DNO_OPENSSL=1
CXXFLAGS = $(CFLAGS) -std=c++20
LDFLAGS = -lrt
LIBS = -lmuslshim -limp -lalog -lfreetype -lssl -lcrypto -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment -lconfig++ \
-lavutil -lavformat -lavfilter -lavcodec -lavdevice -lavutil -lswresample -latomic #-lswresample -lswscale -lavcodec -lavfilter -lavformat -lavdevice -lpostproc

LIBIMP_INC_DIR = ./blob/include/T31

SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.cpp)) \
          $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c)) \

$(info $(OBJECTS))

TARGET = $(BIN_DIR)/prudynt

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CCACHE) $(CXX) $(CXXFLAGS) -I$(LIBIMP_INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CCACHE) $(CC) $(CFLAGS) -I$(LIBIMP_INC_DIR) -c $< -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CCACHE) $(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

.PHONY: all clean

all: $(TARGET)

clean:
	rm -rf $(OBJ_DIR)

distclean: clean
	rm -rf $(BIN_DIR)
