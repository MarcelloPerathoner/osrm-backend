#!/bin/bash
set -eou pipefail

function usage {
    echo "Usage: $0 -f <folder> -r <results_folder> -s <scripts_folder> -b <binaries_folder> -o <osm_pbf> -g <gps_traces>"
    exit 1
}

ROOT_FOLDER=`pwd`
SCRIPTS_FOLDER=${ROOT_FOLDER}/scripts/ci
RESULTS_FOLDER=${ROOT_FOLDER}/test/logs
BINARIES_FOLDER=${OSRM_BUILD_DIR}

while getopts ":f:r:s:b:o:g:" opt; do
  case $opt in
    f) ROOT_FOLDER="$OPTARG"
    ;;
    r) RESULTS_FOLDER="$OPTARG"
    ;;
    s) SCRIPTS_FOLDER="$OPTARG"
    ;;
    b) BINARIES_FOLDER="$OPTARG"
    ;;
    o) OSM_PBF="$OPTARG"
    ;;
    g) GPS_TRACES="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
        usage
    ;;
    :) echo "Option -$OPTARG requires an argument." >&2
        usage
    ;;
  esac
done

if [ -z "${ROOT_FOLDER:-}" ] || [ -z "${RESULTS_FOLDER:-}" ] || [ -z "${SCRIPTS_FOLDER:-}" ] || [ -z "${BINARIES_FOLDER:-}" ] || [ -z "${OSM_PBF:-}" ] || [ -z "${GPS_TRACES:-}" ]; then
    usage
fi

BENCHMARKS_FOLDER=${OSRM_BENCHMARKS_BUILD_DIR:-${BINARIES_FOLDER}/src/benchmarks}
TEST_DATA_FOLDER=${ROOT_FOLDER}/test/data
LIB_FOLDER=${ROOT_FOLDER}/lib
TMP_FOLDER=${ROOT_FOLDER}/tmp
EXE=${EXE:-}

mkdir -p $TMP_FOLDER

function measure_peak_ram_and_time {
    COMMAND=$1
    case $(uname) in
      Darwin)
        gtime -f "Time: %es Peak RAM: %MKB" $COMMAND
      ;;
      Linux)
        /usr/bin/time -f "Time: %es Peak RAM: %MKB" $COMMAND
      ;;
      *)
        $COMMAND
      ;;
    esac
}

function summary {
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
        printf '%b' "$1" >> $GITHUB_STEP_SUMMARY
    fi
}

function run_benchmarks_for_folder {
    mkdir -p $RESULTS_FOLDER

    echo "Running match-bench MLD"
    "$BENCHMARKS_FOLDER/match-bench${EXE}" "$TEST_DATA_FOLDER/mld/monaco.osrm" mld > "$RESULTS_FOLDER/match_mld.bench"
    echo "Running match-bench CH"
    "$BENCHMARKS_FOLDER/match-bench${EXE}" "$TEST_DATA_FOLDER/ch/monaco.osrm" ch > "$RESULTS_FOLDER/match_ch.bench"
    echo "Running route-bench MLD"
    "$BENCHMARKS_FOLDER/route-bench${EXE}" "$TEST_DATA_FOLDER/mld/monaco.osrm" mld > "$RESULTS_FOLDER/route_mld.bench"
    echo "Running route-bench CH"
    "$BENCHMARKS_FOLDER/route-bench${EXE}" "$TEST_DATA_FOLDER/ch/monaco.osrm" ch > "$RESULTS_FOLDER/route_ch.bench"
    echo "Running alias"
    "$BENCHMARKS_FOLDER/alias-bench${EXE}" > "$RESULTS_FOLDER/alias.bench"
    echo "Running json-render-bench"
    "$BENCHMARKS_FOLDER/json-render-bench${EXE}"  "$TEST_DATA_FOLDER/portugal_to_korea.json" > "$RESULTS_FOLDER/json-render.bench"
    echo "Running packedvector-bench"
    "$BENCHMARKS_FOLDER/packedvector-bench${EXE}" 10 100000 > "$RESULTS_FOLDER/packedvector.bench"
    echo "Running rtree-bench"
    "$BENCHMARKS_FOLDER/rtree-bench${EXE}" "$TEST_DATA_FOLDER/monaco.osrm.ramIndex" "$TEST_DATA_FOLDER/monaco.osrm.fileIndex" "$TEST_DATA_FOLDER/monaco.osrm.nbg_nodes" > "$RESULTS_FOLDER/rtree.bench"

    ln -sf `realpath $OSM_PBF` "$TMP_FOLDER/data.osm.pbf"

    pushd $TMP_FOLDER

    echo "Running osrm-extract"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-extract${EXE} -p $ROOT_FOLDER/profiles/car.lua data.osm.pbf" > "$RESULTS_FOLDER/osrm_extract.bench" 2>&1
    echo "Running osrm-partition"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-partition${EXE} data.osrm" > "$RESULTS_FOLDER/osrm_partition.bench" 2>&1
    echo "Running osrm-customize"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-customize${EXE} data.osrm" > "$RESULTS_FOLDER/osrm_customize.bench" 2>&1
    echo "Running osrm-contract"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-contract${EXE}  data.osrm" > "$RESULTS_FOLDER/osrm_contract.bench" 2>&1

    pwd
    ls -al

    popd

    # if [[ -f "$LIB_FOLDER/binding_napi_v8/node_osrm.node" ]]; then
    #   for ALGORITHM in ch mld; do
    #       for BENCH in nearest table trip route match; do
    #           echo "Running node $BENCH $ALGORITHM"
    #           node "$SCRIPTS_FOLDER/bench.js" "$LIB_FOLDER/index.js" "$TMP_FOLDER/data.osrm" $ALGORITHM $BENCH 5 "$GPS_TRACES" \
    #               > "$RESULTS_FOLDER/node_${BENCH}_${ALGORITHM}.bench" || true
    #       done
    #   done
    # fi
    #
    # for ALGORITHM in ch mld; do
    #     for BENCH in nearest table trip route match; do
    #         echo "Running random $BENCH $ALGORITHM"
    #         "$BENCHMARKS_FOLDER/bench${EXE}" "$TMP_FOLDER/data.osrm" $ALGORITHM $BENCH 5 "$GPS_TRACES" \
    #             > "$RESULTS_FOLDER/random_${BENCH}_${ALGORITHM}.bench" || true
    #     done
    # done

    summary "### e2e benchmarks\n"
    summary "Mean time for answering 100 randomly generated requests, in ms.\n"
    summary "| Algorithm | Route | Nearest | Trip | Table | Match |\n"
    summary "| --------- | -----:| -------:| ----:| -----:| -----:|\n"

    for ALGORITHM in ch mld; do
        "$BINARIES_FOLDER/osrm-routed${EXE}" --algorithm $ALGORITHM "$TMP_FOLDER/data.osrm" > /dev/null 2>&1 &
        OSRM_ROUTED_PID=$!

        # wait for osrm-routed to start
        if ! curl --retry-delay 3 --retry 10 --retry-all-errors \
                "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true" > /dev/null 2>&1; then
            echo "osrm-routed failed to start for algorithm $ALGORITHM"
            kill -9 $OSRM_ROUTED_PID
            continue
        fi

        summary "| ${ALGORITHM}"

        for METHOD in route nearest trip table match; do
            echo "Running e2e benchmark for $METHOD $ALGORITHM"
            TIME=`python "$SCRIPTS_FOLDER/e2e_benchmark_simplified.py" --method $METHOD --gps_traces "$GPS_TRACES"`
            echo $TIME > "$RESULTS_FOLDER/e2e_${METHOD}_${ALGORITHM}.bench"
            summary " | $TIME"
        done

        summary " |\n"
        kill -9 $OSRM_ROUTED_PID
    done

    rm -f "$TMP_FOLDER/data.osm.pbf" "$TMP_FOLDER"/data.osrm.*
}

run_benchmarks_for_folder
