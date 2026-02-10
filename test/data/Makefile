# Builds the monaco and berlin datasets into
#
# - test/data/ch
# - test/data/mld
#
# Note: the files test/data/*.orsm.* may also come from library-extract-tests.

DATASETS = monaco berlin

all: monaco

monaco: verify  # verify first, extract later

OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract$(EXE)
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract$(EXE)
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition$(EXE)
OSRM_CUSTOMIZE:=$(OSRM_BUILD_DIR)/osrm-customize$(EXE)
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed$(EXE)
MD5SUM:=$(PROJECT_SOURCE_DIR)/scripts/md5sum.js
PROFILE:=$(PROJECT_SOURCE_DIR)/profiles/car.lua

%.osrm.edges: %.osm.pbf $(PROFILE) $(OSRM_EXTRACT)
	@echo "Running osrm-extraction ..."
	$(OSRM_EXTRACT) $< -p $(PROFILE) > /dev/null

ch/%.osrm.hsgr: %.osrm.edges $(OSRM_CONTRACT)
	mkdir -p ch
	cp *.osrm.* ch/
	@echo "Running osrm-contract ..."
	cd ch && $(OSRM_CONTRACT) $*.osrm > /dev/null

mld/%.osrm.mldgr: %.osrm.edges $(OSRM_PARTITION) $(OSRM_CUSTOMIZE)
	mkdir -p mld
	cp *.osrm.* mld/
	@echo "Running osrm-partition ..."
	cd mld && $(OSRM_PARTITION) $*.osrm > /dev/null
	@echo "Running osrm-customize ..."
	cd mld && $(OSRM_CUSTOMIZE) $*.osrm > /dev/null

define OSRM_template =

$(1): ch/$(1).osrm.hsgr mld/$(1).osrm.mldgr

clean-$(1):
	-rm -rf $(1)

clean: clean-$(1)

endef

$(foreach ds,$(DATASETS),$(eval $(call OSRM_template,$(ds))))

berlin.osm.pbf:
	curl https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf -o $@ --location --silent

%.md5sum: %.osm.pbf
	$(MD5SUM) $< > $@

clean:
	-rm -rf ch mld *.osrm.*

checksum: monaco.md5sum

verify:
	@echo -n "Verifying data file integrity "
	@$(MD5SUM) -c monaco.md5sum

.PHONY: all clean checksum verify $(DATASETS)
