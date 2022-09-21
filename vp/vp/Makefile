src = $(wildcard *.cpp)

LDFLAGS = -std=c++11
SYSTEMC= -isystem${SYSTEMC_HOME}/include -L. -L${SYSTEMC_HOME}/lib-linux64 -Wl,-rpath=${SYSTEMC_HOME}/lib-linux64 -lsystemc
LDLIBS= -lasound
decoder: $(src)
	g++ $(LDFLAGS) $(SYSTEMC) -o $@ $^ $(LDLIBS) -lm

.PHONY: clean
clean:
	rm -f $(obj) decoder
	
	
