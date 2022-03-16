TARGET := raytracer

CXX      := g++
VERSION  := -std=c++20
CXXFLAGS := $(VERSION) -Wall -Wextra -pedantic \
				-Wno-unused-result -Wfloat-equal \
				-Wconversion -Wformat-signedness \
				-Wduplicated-cond -Wlogical-op -Wredundant-decls
# CXXFLAGS += -DNDEBUG -O3 -Wno-unused-variable # -Ofast -flto -march=native -s
CXXFLAGS += -DGLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2 \
				-O -ggdb -fno-omit-frame-pointer \
				-fsanitize=undefined,float-divide-by-zero,float-cast-overflow \
				-fstack-protector-all -fno-sanitize-recover=all

INC := -I./src -I./deps/stb -I./deps/assimp/include -I./deps/obj
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
