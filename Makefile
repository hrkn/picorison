prefix=/usr/local
includedir=$(prefix)/include

check: test

test: test-core test-core-int64
	./test-core
	./test-core-int64

test-core: picojson.h test.cc picotest/picotest.c picotest/picotest.h
	$(CXX) -std=c++11 -Wall test.cc picotest/picotest.c -o $@

test-core-int64: picojson.h test.cc picotest/picotest.c picotest/picotest.h
	$(CXX) -std=c++11 -Wall -DPICOJSON_USE_INT64 test.cc picotest/picotest.c -o $@

clean:
	rm -f test-core test-core-int64

install:
	install -d $(DESTDIR)$(includedir)
	install -p -m 0644 picojson.h $(DESTDIR)$(includedir)

uninstall:
	rm -f $(DESTDIR)$(includedir)/picojson.h

clang-format: picojson.h examples/github-issues.cc examples/iostream.cc examples/streaming.cc
	clang-format -i $?

.PHONY: test check clean install uninstall clang-format
