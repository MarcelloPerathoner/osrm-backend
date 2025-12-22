// Custom World class for OSRM test environment using modern Cucumber.js v13 patterns
import path from 'path';
import { World, setWorldConstructor } from '@cucumber/cucumber';

import * as OSM from '../lib/osm.js';
import OSRMLoader from '../lib/osrm_loader.js';
import { env } from '../support/env.js';

import Cache from './cache.js';
import Data from './data.js';
import Http from './http.js';
import Route from './route.js';
import Run from './run.js';
import Options from './options.js';
import Fuzzy from './fuzzy.js';
import SharedSteps from './shared_steps.js';

class OSRMWorld extends World {
  // Private instances of support classes for clean composition
  #data;
  #http;
  #route;
  #run;
  #sharedSteps;
  #fuzzy;
  #options;

  constructor(options) {
    // Get built-in Cucumber helpers: this.attach, this.log, this.parameters
    super(options);

    // Initialize service instances with access to world
    this.#data = new Data(this);
    this.#http = new Http(this);
    this.#route = new Route(this);
    this.#run = new Run(this);
    this.#sharedSteps = new SharedSteps(this);
    this.#fuzzy = new Fuzzy(this);
    this.#options = new Options(this);

    // Copy methods from services to world for compatibility
    this.#copyMethodsFromServices();

    // Initialize core objects
    this.osrmLoader = new OSRMLoader(this);
    this.OSMDB = new OSM.DB();

    // Copy properties that need direct access
    this.FuzzyMatch = this.#fuzzy.FuzzyMatch;
  }

  // Copy methods from service classes
  #copyMethodsFromServices() {
    [
      this.#data,
      this.#http,
      this.#route,
      this.#run,
      this.#sharedSteps,
      this.#options,
    ].forEach((service) => {
      Object.getOwnPropertyNames(Object.getPrototypeOf(service)).forEach(
        (name) => {
          if (name !== 'constructor' && typeof service[name] === 'function') {
            this[name] = service[name].bind(this);
          }
        },
      );
    });
  }

  // Clean getter access to services
  get data() {
    return this.#data;
  }
  get http() {
    return this.#http;
  }
  get route() {
    return this.#route;
  }
  get run() {
    return this.#run;
  }
  get sharedSteps() {
    return this.#sharedSteps;
  }
  get fuzzy() {
    return this.#fuzzy;
  }
  get options() {
    return this.#options;
  }

  // Initialize the world for a specific test case
  // This method is called from Before hook since constructors can't be async
  init(testCase, callback) {
    this.cache = new Cache(env, testCase.pickle.uri, callback);
    this.setupCurrentScenario(this.cache, testCase);

    callback();
  }

  setupCurrentScenario(cache, testCase) {
    this.profile = env.OSRM_PROFILE || env.DEFAULT_PROFILE;
    this.profileFile = path.join(env.PROFILES_PATH, `${this.profile}.lua`);
    this.osrmLoader.setLoadMethod(env.DEFAULT_LOAD_METHOD);
    this.setGridSize(env.DEFAULT_GRID_SIZE);
    this.setOrigin(env.DEFAULT_ORIGIN);
    this.queryParams = {};
    this.extractArgs = '';
    this.contractArgs = '';
    this.partitionArgs = '';
    this.customizeArgs = '';
    this.loaderArgs = '';
    this.environment = Object.assign({}, env.DEFAULT_ENVIRONMENT);
    this.resetOSM();

    const scenarioID = cache.getScenarioID(testCase);

    this.scenarioCacheFile  = cache.getScenarioCacheFile(scenarioID);
    this.processedCacheFile = cache.getProcessedCacheFile(scenarioID);
    this.inputCacheFile     = cache.getInputCacheFile(scenarioID);
    this.rasterCacheFile    = cache.getRasterCacheFile(scenarioID);
    this.speedsCacheFile    = cache.getSpeedsCacheFile(scenarioID);
    this.penaltiesCacheFile = cache.getPenaltiesCacheFile(scenarioID);
    this.profileCacheFile   = cache.getProfileCacheFile(scenarioID);
    this.scenarioLogFile    = cache.setupLogFile(scenarioID);
  }

  // Cleanup method called from After hook
  cleanup(callback) {
    this.resetOptionsOutput();
    if (this.osrmLoader) {
      this.osrmLoader.shutdown(() => {
        callback();
      });
    } else {
      callback();
    }
  }
}

// Register the custom World constructor
setWorldConstructor(OSRMWorld);
