DATA_NAME?=monaco
OSRM_BUILD_DIR?=../../build
PROFILE_ROOT:=../../profiles
SCRIPT_ROOT:=../../scripts
OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract$(EXE)
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract$(EXE)
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition$(EXE)
OSRM_CUSTOMIZE:=$(OSRM_BUILD_DIR)/osrm-customize$(EXE)
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed$(EXE)
POLY2REQ:=$(SCRIPT_ROOT)/poly2req.js
MD5SUM:=$(SCRIPT_ROOT)/md5sum.js
TIMER:=$(SCRIPT_ROOT)/timer.js
PROFILE:=$(PROFILE_ROOT)/car.lua
TMP:=`node $(SCRIPT_ROOT)/get-tmp-dir.js`

all: monaco

monaco: verify ch/monaco.osrm.hsgr mld/monaco.osrm.partition

berlin: ch/berlin.osrm.hsgr mld/berlin.osrm.partition

clean:
	-rm -r *.osrm.*
	-rm -r ch mld

ch/%.osrm: %.osrm
	mkdir -p ch
	cp $<.* ch/

mld/%.osrm: %.osrm
	mkdir -p mld
	cp $<.* mld/

%.osrm: %.osm.pbf $(PROFILE) $(OSRM_EXTRACT)
	@echo "Running osrm-extract..."
	$(TIMER) "osrm-extract\t$@" $(OSRM_EXTRACT) $< -p $(PROFILE)

ch/%.hsgr: ch/% $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(TIMER) "osrm-contract\t$@" $(OSRM_CONTRACT) $<

mld/%.partition: mld/% $(PROFILE) $(OSRM_PARTITION)
	@echo "Running osrm-partition..."
	$(TIMER) "osrm-partition\t$@" $(OSRM_PARTITION) $<
	$(TIMER) "osrm-customize\t$@" $(OSRM_CUSTOMIZE) $<

%.md5sum: %.osm.pbf
	$(MD5SUM) $< > $@

%.requests: %.poly
	$(POLY2REQ) $< > $@

benchmark: data $(DATA_NAME).requests
	@echo "Running benchmark..."
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=CH ch/$(DATA_NAME).osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries\tCH" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=MLD mld/$(DATA_NAME).osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries\tMLD" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@echo "**** timings ***"
	@cat $(TMP)/osrm.timings
	@echo "****************"

checksum: monaco.md5sum

verify:
	@echo "Verifying data file integrity..."
	$(MD5SUM) -c monaco.md5sum

.PHONY: clean checksum verify benchmark data
