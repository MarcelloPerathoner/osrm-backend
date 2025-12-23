// Manages test data caching system with hashing for performance optimization
import crypto from 'crypto';
import fs, { mkdirSync, rmSync } from 'fs';
import util from 'util';
import path from 'path';
import { formatterHelpers } from '@cucumber/cucumber';

export default class Cache {
  constructor(env, uri) {
    this.env = env;
    // There is one cache per feature.
    //
    // The feature cache contains the .osm files cucumber generated for: "Given the node
    // map ... and the ways ..." eg. test/cache/car/access.feature/<hash>/
    //
    // The processed feature cache contains the files osrm-extract generated from the
    // files in the feature cache. It is a subdirectory of the feature cache directory
    // named after the osrmHash. eg. test/cache/car/access.feature/<hash>/<osrmHash>/

    // if OSRM_PROFILE is set to force a specific profile, then
    // include the profile name in the hash of the profile file
    const content = fs.readFileSync(uri);
    const checksum = crypto.createHash('md5');
    checksum.update(content + (this.env.OSRM_PROFILE || ''));
    const hash = checksum.digest('hex');

    // shorten uri to be realtive to 'features/'
    const featurePath = path.relative(path.resolve('./features'), uri);
    /** eg. bicycle/bollards/{hash}/ */
    this.featureID = path.join(featurePath, hash);
    /** eg. test/cache/bicycle/bollards/{hash}/ */
    this.featureCacheDirectory = path.join(this.env.CACHE_PATH, this.featureID);
    /** eg. test/cache/bicycle/bollards/{hash}/{osrm_hash}/ */
    this.featureProcessedCacheDirectory = path.join(this.featureCacheDirectory, this.env.osrmHash);

    // make a new or clean the old cache directory
    mkdirSync(this.featureProcessedCacheDirectory, { recursive: true });
    this.removeOldFeatureCaches(path.join(this.env.CACHE_PATH, featurePath), hash, this.env.osrmHash);
  };

  removeOldFeatureCaches(parent, hash, osrmHash) {
    for (const file of fs.readdirSync(parent)) {
      const fn = path.join(parent, file);
      if (file === hash) {
        this.removeOldProcessedFeatureCaches(fn, osrmHash);
      } else {
        rmSync(fn, { recursive: true, force: true });
      }
    };
  }

  removeOldProcessedFeatureCaches(parent, osrmHash) {
    for (const file of fs.readdirSync(parent)) {
      const fn = path.join(parent, file);
      const stat = fs.statSync(fn);
      if (stat.isDirectory() && file !== osrmHash) {
        rmSync(fn, { recursive: true, force: true });
      }
    };
  }

  // converts the scenario titles in file prefixes
  // Cucumber v12 API: testCase parameter contains { gherkinDocument, pickle }
  // Use formatterHelpers.PickleParser.getPickleLocation() to get line numbers like scenario.getLine() in v1
  getScenarioID(testCaseParam) {
    const { gherkinDocument, pickle } = testCaseParam;
    const name = pickle.name
      .toLowerCase()
      .replace(/[/\-'=,():*#]/g, '')
      .replace(/\s/g, '_')
      .replace(/__/g, '_')
      .replace(/\.\./g, '.')
      .substring(0, 64);

    // Get line number using Cucumber v12 API
    const { line } = formatterHelpers.PickleParser.getPickleLocation({
      gherkinDocument,
      pickle,
    });

    return util.format('%d_%s', line, name);
  }

  // test/cache/bicycle/bollards/{feature_hash}/{scenario}.log
  /** Ensures the logfile directory and removes an old logfile. */
  setupLogFile(scenarioID) {
    const logDir = path.join(this.env.LOGS_PATH, this.featureID || 'default');
    mkdirSync(logDir, { recursive: true });
    const logFile = `${path.join(logDir, scenarioID)}.log`;
    rmSync(logFile, { force: true });
    return logFile;
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_raster.asc
  getRasterCacheFile(scenarioID) {
    return `${path.join(this.featureProcessedCacheDirectory, scenarioID)}_raster.asc`;
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_speeds.csv
  getSpeedsCacheFile(scenarioID) {
    return `${path.join(this.featureProcessedCacheDirectory, scenarioID)}_speeds.csv`;
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_penalties.csv
  getPenaltiesCacheFile(scenarioID) {
    return `${path.join(this.featureProcessedCacheDirectory, scenarioID)}_penalties.csv`;
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_profile.lua
  getProfileCacheFile(scenarioID) {
    return `${path.join(this.featureProcessedCacheDirectory, scenarioID)}_profile.lua`;
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}.osm
  getScenarioCacheFile(scenarioID) {
    return `${path.join(this.featureCacheDirectory, scenarioID)}.osm`;
  }

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osrm
  getProcessedCacheFile(scenarioID) {
    return `${path.join(this.featureProcessedCacheDirectory, scenarioID)}.osrm`;
  }

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osm
  getInputCacheFile(scenarioID) {
    return `${path.join(this.featureProcessedCacheDirectory, scenarioID)}.osm`;
  }
}
