// Manages test data caching system with hashing for performance optimization
import crypto from 'crypto';
import fs from 'fs';
import { mkdir } from 'fs/promises';
import util from 'util';
import path from 'path';
import { rm } from 'fs/promises';
import { formatterHelpers } from '@cucumber/cucumber';

export default class Cache {
  constructor(env) {
    this.env = env;
  }

  // Initializes caching system with OSRM binary hash
  initializeCache() {
    this.osrmHash = this.getOSRMHash();
  }

  initializeFeature(uri, callback) {
    // setup cache for feature data
    // if OSRM_PROFILE is set to force a specific profile, then
    // include the profile name in the hash of the profile file

    const content = fs.readFileSync(uri);
    const checksum = crypto.createHash('md5');
    checksum.update(content + (this.env.OSRM_PROFILE || ''));
    const hash = checksum.digest('hex');

    // shorten uri to be realtive to 'features/'
    const featurePath = path.relative(path.resolve('./features'), uri);
    // bicycle/bollards/{HASH}/
    const featureID = path.join(featurePath, hash);
    const featureCacheDirectory = this.getFeatureCacheDirectory(featureID);
    const featureProcessedCacheDirectory =
      this.getFeatureProcessedCacheDirectory(
        featureCacheDirectory,
        this.osrmHash,
      );
    this.featureIDs[uri] = featureID;
    this.featureCacheDirectories[uri] = featureCacheDirectory;
    this.featureProcessedCacheDirectories[uri] =
      featureProcessedCacheDirectory;
    mkdir(featureProcessedCacheDirectory, { recursive: true });
    this.cleanupFeatureCache(featureCacheDirectory, hash);
    this.cleanupProcessedFeatureCache(featureProcessedCacheDirectory, this.osrmHash, callback);
  };

  // computes all paths for every feature
  // Sets up cache directories and hashes for all test features
  initializeFeatures() {
    this.featureIDs = {};
    this.featureCacheDirectories = {};
    this.featureProcessedCacheDirectories = {};
  }

  cleanupProcessedFeatureCache(directory, osrmHash, callback) {
    const parentPath = path.resolve(path.join(directory, '..'));
    fs.readdirSync(parentPath).forEach((f) => {
      const filePath = path.join(parentPath, f);
      fs.stat(filePath, (err, stat) => {
        if (err) return callback(err);
        if (stat.isDirectory() && filePath.search(osrmHash) < 0) {
          rm(filePath, { recursive: true, force: true });
        }
      });
    });
  }

  cleanupFeatureCache(directory, featureHash) {
    const parentPath = path.resolve(path.join(directory, '..'));
    fs.readdirSync(parentPath)
      .filter((name) => name !== featureHash)
      .forEach((f) => {
        rm(path.join(parentPath, f), { recursive: true, force: true });
      });
  }

  setupFeatureCache(feature) {
    const uri = feature.getUri();
    this.featureID = this.featureIDs[uri];
    this.featureCacheDirectory = this.featureCacheDirectories[uri];
    this.featureProcessedCacheDirectory =
      this.featureProcessedCacheDirectories[uri];
  }

  // returns a hash of all OSRM code side dependencies
  // that is: all osrm binaries and all lua profiles
  getOSRMHash() {
    const env = this.env;
    const dependencies = [
      env.OSRM_EXTRACT_PATH,
      env.OSRM_CONTRACT_PATH,
      env.OSRM_CUSTOMIZE_PATH,
      env.OSRM_PARTITION_PATH,
      env.LIB_OSRM_EXTRACT_PATH,
      env.LIB_OSRM_CONTRACT_PATH,
      env.LIB_OSRM_CUSTOMIZE_PATH,
      env.LIB_OSRM_PARTITION_PATH,
    ];

    const addLuaFiles = function (directory) {
      const luaFiles = fs.readdirSync(path.normalize(directory))
        .filter((f) => !!f.match(/\.lua$/))
        .map((f) => path.normalize(`${directory}/${f}`));
      Array.prototype.push.apply(dependencies, luaFiles);
    };

    addLuaFiles(env.PROFILES_PATH);
    addLuaFiles(`${env.PROFILES_PATH}/lib`);

    const checksum = crypto.createHash('md5');
    for (const file of dependencies) {
      checksum.update(fs.readFileSync(file));
    }
    return checksum.digest('hex');
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

  // test/cache/bicycle/bollards/{HASH}/
  getFeatureCacheDirectory(featureID) {
    return path.join(this.env.CACHE_PATH, featureID);
  }

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/
  getFeatureProcessedCacheDirectory(featureCacheDirectory, osrmHash) {
    return path.join(featureCacheDirectory, osrmHash);
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
