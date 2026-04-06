CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Werror -g \
          -I./inc \
          -I./inc \
          -I./inc/iso14229 \
          -I./inc \
          -I./safety \
          -I./inc/isotp \
          -I./test/unity \
          -DPLATFORM_LINUX

CC11     = gcc
C11FLAGS = -std=c11 -Wall -Wextra -g \
           -D_POSIX_C_SOURCE=200809L \
           -I./inc \
           -I./inc \
           -I./inc/iso14229 \
           -I./inc \
           -I./safety \
           -I./inc/isotp \
           -I./test/unity \
           -DPLATFORM_LINUX

UNITY   = test/unity/unity.c
PLAT    = platform/platform_linux.c

DEM_CORE    = dem/dem_core.c dem/dem_debounce.c dem/dem_aging.c dem/dem_dtc.c
DEM_STORAGE = dem/dem_dtc.c dem/dem_nvm.c
DCM_SRC     = dcm/iso14229/iso14229.c \
              dcm/dcm_did_table.c \
              dcm/dcm_routine_table.c \
              dcm/dcm_callbacks.c

# Sprint 1 — DEM core tests
S1_SRC = $(DEM_CORE) $(PLAT) $(UNITY) test/test_dem_core.c

# Sprint 2 — DEM storage tests
S2_SRC = $(DEM_CORE) $(DEM_STORAGE) $(PLAT) $(UNITY) test/test_dem_storage.c

# Sprint 4 — DCM UDS loopback tests
S4_SRC = $(DEM_CORE) $(DEM_STORAGE) $(DCM_SRC) $(PLAT) $(UNITY) \
         test/test_dcm_uds.c

# Sprint 5 — DID table tests
S5_SRC = dcm/dcm_did_table.c $(UNITY) test/test_dcm_did.c

# Sprint 7 — Routine table tests (coming next)
S7_SRC = dcm/dcm_did_table.c dcm/dcm_routine_table.c \
         $(UNITY) test/test_dcm_routine.c

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

test_dcm_uds: $(S4_SRC)
	$(CC11) $(C11FLAGS) -DUDS_TP_ISOTP_MOCK $^ -o bin/$@ -lpthread
	@echo ""
	@echo ">>> Running DCM UDS loopback tests..."
	@echo ""
	./bin/$@

test_dcm_did: $(S5_SRC)
	$(CC) $(CFLAGS) -I./inc $^ -o bin/$@ -lpthread
	@echo ""
	@echo ">>> Running DID table tests..."
	@echo ""
	./bin/$@

test_dcm_routine: $(S7_SRC)
	$(CC) $(CFLAGS) -I./inc $^ -o bin/$@ -lpthread
	@echo ""
	@echo ">>> Running routine table tests..."
	@echo ""
	./bin/$@

.PHONY: all clean lint

all: test_dem_core test_dem_storage test_dcm_uds test_dcm_did

clean:
	rm -f bin/*
	rm -f /tmp/dem_nvm_*.bin

lint:
	cppcheck --enable=all --std=c99 \
	         --suppress=missingIncludeSystem \
	         -I./inc -I./inc -I./inc \
	         dem/ platform/ dcm/ 2>&1

S_AGING = dem/dem_core.c dem/dem_debounce.c dem/dem_dtc.c \
          dem/dem_nvm.c dem/dem_aging.c \
          platform/platform_linux.c \
          test/unity/unity.c test/test_dem_aging.c

test_dem_aging: $(S_AGING)
	$(CC) $(CFLAGS) -I./inc $^ -o bin/$@ -lpthread
	@echo ""
	@echo ">>> Running aging+healing tests..."
	@echo ""
	./bin/$@
