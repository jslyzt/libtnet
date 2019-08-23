BINDIR=build

.PHONY: all debug release builddir clean package

all: release package

debug: builddir
	cd ${BINDIR} && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc)

release: builddir
	cd ${BINDIR} && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)

builddir:
	mkdir -p ${BINDIR}

clean:
	rm -fr ${BINDIR}

package:
	cd ${BINDIR} && rm -fr CMake* cmake_* Makefile src test
