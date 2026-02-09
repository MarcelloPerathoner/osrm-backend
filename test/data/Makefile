# Builds the monaco and berlin datasets.
#
# - test/data/monaco/ch
# - test/data/monaco/mld
# - test/data/ch  -> monaco/ch   # for compatibility with earlier layout
# - test/data/mld -> monaco/mld  # "
# - test/data/berlin/ch
# - test/data/berlin/mld


PROFILE_ROOT:=$(PROJECT_SOURCE_DIR)/profiles
SCRIPT_ROOT:=$(PROJECT_SOURCE_DIR)/scripts
OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract$(EXE)
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract$(EXE)
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition$(EXE)
OSRM_CUSTOMIZE:=$(OSRM_BUILD_DIR)/osrm-customize$(EXE)
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed$(EXE)
MD5SUM:=$(SCRIPT_ROOT)/md5sum.js
TIMER:=$(SCRIPT_ROOT)/timer.js
PROFILE:=$(PROFILE_ROOT)/car.lua

all: monaco

DATASETS = monaco berlin

define OSRM_template =

$(1): $(1)/ch/$(1).osrm.hsgr $(1)/mld/$(1).osrm.mldgr

$(1)/$(1).osrm.edges: $(1).osm.pbf $$(PROFILE) $$(OSRM_EXTRACT)
	@echo "Running osrm-extraction ..."
	mkdir -p $(1)
	cd $(1) && \
	ln -sf ../$$< && \
	$$(TIMER) "osrm-extract\t$$@" $$(OSRM_EXTRACT) $$< -p $$(PROFILE)

$(1)/ch/$(1).osrm.hsgr: $(1)/$(1).osrm.edges $$(OSRM_CONTRACT)
	@echo "Running osrm-contract ..."
	mkdir -p $(1)/ch
	cd $(1)/ch && \
	cp ../*.osrm.* . && \
	$$(TIMER) "osrm-contract\t$$*" $$(OSRM_CONTRACT) $(1).osrm

$(1)/mld/$(1).osrm.mldgr: $(1)/$(1).osrm.edges $$(OSRM_PARTITION) $$(OSRM_CUSTOMIZE)
	@echo "Running osrm-partition ..."
	mkdir -p $(1)/mld
	cd $(1)/mld && \
	cp ../*.osrm.* . && \
	$$(TIMER) "osrm-partition\t$$*" $$(OSRM_PARTITION) $(1).osrm
	@echo "Running osrm-customize ..."
	cd $(1)/mld && \
	$$(TIMER) "osrm-customize\t$$*" $$(OSRM_CUSTOMIZE) $(1).osrm

clean-$(1):
	-rm -rf $(1)

clean: clean-$(1)

endef

$(foreach ds,$(DATASETS),$(eval $(call OSRM_template,$(ds))))

monaco: verify
	ln -sf monaco/ch monaco/mld .

clean:
	-rm -rf ch mld

berlin.osm.pbf:
	curl https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf -o $@ --location --silent

%.md5sum: %.osm.pbf
	$(MD5SUM) $< > $@

checksum: monaco.md5sum

verify:
	@echo -n "Verifying data file integrity "
	@$(MD5SUM) -c monaco.md5sum

.PHONY: all clean checksum verify $(DATASETS)
