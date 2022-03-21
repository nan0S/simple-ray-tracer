TARGET := raytracer

CXX      := g++
VERSION  := -std=c++20
CXXFLAGS := $(VERSION) -Wall -Wextra -Wno-missing-field-initializers
CXXFLAGS += -DNDEBUG -O3 -Wno-unused-variable # -Ofast -flto -march=native -s
# CXXFLAGS += -O -ggdb -fno-omit-frame-pointer

INC := -I./src -I./deps/stb
LIB := -lGLEW -lGL -lglfw -lGLU -lassimp
SRC := $(shell find src -name '*.cpp')
OBJ := $(patsubst %.cpp,build/%.o,$(SRC))
DEP := $(patsubst %.cpp,build/%.d,$(SRC))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIB)

-include $(DEP)

build/%.o: %.cpp Makefile
	@mkdir -p $(shell dirname $@)
	$(CXX) -MMD -MP $(CXXFLAGS) -c -o $@ $< $(INC)

build/%.o: %.c Makefile
	@mkdir -p $(shell dirname $@)
	$(CXX) -MMD -MP $(CXXFLAGS) -c -o $@ $< $(INC)

format:
	@(shopt -s nullglob; \
	  clang-format -i *.cpp *.hpp *.c *.h *.cu *.cuh)

clean:
	rm -rf build $(TARGET) compile_commands.json
