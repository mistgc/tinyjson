compile: build
	ninja -v -j8 -C build

build: CMakeLists.txt clean
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja

clean:
	rm -rf build

check: build 
	find ${PWD} -name *.cc | xargs clang-check -p ${PWD}/build

test: compile
	@printf '\nRunning test case:\n'
	@build/test/tinyjson_test

PHONY: clean build check compile test
