CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Werror -g \
          -I./dem \
          -I./dcm \
          -I./dcm/iso14229 \
          -I./platform \
          -I./safety \
          -I./transport/isotp \
          -I./test/unity \
          -DPLATFORM_LINUX

CC11    = gcc
C11FLAGS = -std=c11 -Wall -Wextra -g \
           -I./dem \
           -I./dcm \
           -I./dcm/iso14229 \
           -I./platform \
           -I./safety \
           -I./transport/isotp \
           -I./test/unity \
           -DPLATFORM_LINUX

UNITY   = test/unity/unity.c
PLAT    = platform/platform_linux.c

DEM_CORE    = dem/dem_core.c dem/dem_debounce.c
DEM_STORAGE = dem/dem_dtc.c dem/dem_nvm.c
DCM_SRC     = dcm/iso14229/iso14229.c dcm/dcm_callbacks.c dcm/dcm_main.c

S1_SRC = $(DEM_CORE) $(PLAT) $(UNITY) \
         test/test_dem_core.c

S2_SRC = $(DEM_CORE) $(DEM_STORAGE) $(PLAT) $(UNITY) \
         test/test_dem_storage.c

test_dem_core: $(S1_SRC)
	$(CC) $(CFLAGS) $^ -o bin/$@ -lpthread
	@echo ""
	@echo ">>> Running DEM core tests..."
	@echo ""
	./bin/$@

test_dem_storage: $(S2_SRC)
	$(CC) $(CFLAGS) $^ -o bin/$@ -lpthread
	@echo ""
	@echo ">>> Running DEM storage tests..."
	@echo ""
	./bin/$@

.PHONY: all clean lint

all: test_dem_core test_dem_storage

clean:
	rm -f bin/*
	rm -f /tmp/dem_nvm_*.bin

lint:
	cppcheck --enable=all --std=c99 \
	         --suppress=missingIncludeSystem \
	         -I./dem -I./platform \
	         dem/ platform/ 2>&1
