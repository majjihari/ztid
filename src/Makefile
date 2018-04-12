EXEC=ztid
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)

CXXFLAGS=-std=c++98 -I . -fPIC -m64 -pthread -fmessage-length=0 -gno-record-gcc-switches -g -O2
LDFLAGS=-lcrypto

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) $(CFLAGS) -c $<

clean:
	$(RM) *.o

mrproper: clean
	$(RM) $(EXEC)
