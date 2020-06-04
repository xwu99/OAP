CC := clang
CXX := clang++
RM := rm -rf

CCL_ROOT=/home/xiaochang/Works/mlsl2/build/_install

CFLAGS := -g -Wall -fPIC
INCS := -I $(JAVA_HOME)/include \
        -I $(JAVA_HOME)/include/linux \
        -I $(CCL_ROOT)/include \
        -I $(DAALROOT)/include \
        -I ./javah \
        -I ./

LIBS := -L$(CCL_ROOT)/lib -lccl \
        -L$(DAALROOT)/lib/intel64 -ldaal_core -ldaal_thread \
        -L$(TBBROOT)/lib -ltbb -ltbbmalloc

CPP_SRCS += \
./OneCCL.cpp ./OneDAL.cpp ./KMeansDALImpl.cpp

OBJS += \
./OneCCL.o ./OneDAL.o ./KMeansDALImpl.o

# Output Binary
OUTPUT = ../../../target/scala-2.12/classes/libMLlibDAL.so

#clang++ -Wall -w kmeans_oneccl.cpp -o kmeans_oneccl.exe -I$(CCL_ROOT)/include -I./ -Wl,--start-group $(DAALROOT)/lib/intel64/libdaal_core.so $(DAALROOT)/lib/intel64/libdaal_thread.so -L$(TBBROOT)/lib -L$(CCL_ROOT)/lib -ltbb -ltbbmalloc -ldl -lccl -Wl,--end-group


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
