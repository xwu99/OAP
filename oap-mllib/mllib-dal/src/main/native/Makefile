CC := gcc
CXX := g++
RM := rm -rf

CCL_ROOT=/home/xiaochang/Works/mlsl2/build/_install

CFLAGS := -g -Wall -fPIC -std=c++11
INCS := -I $(JAVA_HOME)/include \
        -I $(JAVA_HOME)/include/linux \
        -I $(CCL_ROOT)/include \
        -I $(DAALROOT)/include \
        -I ./javah \
        -I ./

# Use static link if possible, TBB is only available as dynamic libs
LIBS := -L$(CCL_ROOT)/lib -l:libccl.a \
        -L$(DAALROOT)/lib/intel64 -l:libdaal_core.a -l:libdaal_thread.a \
        -L$(TBBROOT)/lib -ltbb -ltbbmalloc

CPP_SRCS += \
./OneCCL.cpp ./OneDAL.cpp ./KMeansDALImpl.cpp

OBJS += \
./OneCCL.o ./OneDAL.o ./KMeansDALImpl.o

# Output Binary
OUTPUT = ../../../target/libMLlibDAL.so

all: $(OUTPUT)

# Compile
%.o: %.cpp
	@echo 'Building file: $<'
	$(CXX) $(CFLAGS) $(INCS) -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

# Link
$(OUTPUT): $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Linker'
	$(CXX) $(CFLAGS) -shared -o $(OUTPUT) $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

clean:
	-$(RM) $(OBJS) $(OUTPUT)
	-@echo ' '

.PHONY: all clean
