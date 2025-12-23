// Custom World class for OSRM test environment using modern Cucumber.js v13 patterns
import path from 'path';
import { World, setWorldConstructor } from '@cucumber/cucumber';

import * as OSM from '../lib/osm.js';
import { OSRMDatastoreLoader, OSRMDirectLoader, OSRMmmapLoader } from '../lib/osrm_loader.js';
import { env } from '../support/env.js';

import Cache from './cache.js';
import Data from './data.js';
import Http from './http.js';
import Route from './route.js';
import Run from './run.js';
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

  constructor(options) {
    // Get built-in Cucumber helpers: this.attach, this.log, this.parameters
    super(options);

    this.osrmLoader = null;
    this.loadMethod = null;

    // Initialize service instances with access to world
    this.#data = new Data(this);
    this.#http = new Http(this);
    this.#route = new Route(this);
    this.#run = new Run(this);
    this.#sharedSteps = new SharedSteps(this);
    this.#fuzzy = new Fuzzy(this);

    // Copy methods from services to world for compatibility
    this.#copyMethodsFromServices();

    // Initialize core objects
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

  // Initialize the world for a specific test case
  // This method is called from Before hook since constructors can't be async
  async init(scenario) {
    this.setLoadMethod(env.DEFAULT_LOAD_METHOD);
    this.cache = new Cache(env, scenario.pickle.uri);
    this.setupCurrentScenario(this.cache, scenario);
    this.resetChildOutput();
  }

  setupCurrentScenario(cache, scenario) {
    this.profile = env.OSRM_PROFILE || env.DEFAULT_PROFILE;
    this.profileFile = path.join(env.PROFILES_PATH, `${this.profile}.lua`);
    this.setGridSize(env.DEFAULT_GRID_SIZE);
    this.setOrigin(env.DEFAULT_ORIGIN);
    this.queryParams = {};
    this.extractArgs   = [];
    this.contractArgs  = [];
    this.partitionArgs = [];
    this.customizeArgs = [];
    this.loaderArgs    = [];
    this.environment = Object.assign({}, env.DEFAULT_ENVIRONMENT);
    this.resetOSM();

    const scenarioID = cache.getScenarioID(scenario);

    this.scenarioCacheFile  = cache.getScenarioCacheFile(scenarioID);
    this.processedCacheFile = cache.getProcessedCacheFile(scenarioID);
    this.inputCacheFile     = cache.getInputCacheFile(scenarioID);
    this.rasterCacheFile    = cache.getRasterCacheFile(scenarioID);
    this.speedsCacheFile    = cache.getSpeedsCacheFile(scenarioID);
    this.penaltiesCacheFile = cache.getPenaltiesCacheFile(scenarioID);
    this.profileCacheFile   = cache.getProfileCacheFile(scenarioID);
    this.scenarioLogFile    = cache.setupLogFile(scenarioID);
  }

  setLoadMethod(method) {
    if (method === this.method)
      return;
    if (this.osrmLoader && this.osrmLoader.osrmIsRunning())
      throw new Error(`Cannot switch data load method while server is running: ${method}`);
    this.loadMethod = method;
    if (method === 'datastore') {
      this.osrmLoader = new OSRMDatastoreLoader(this);
    } else if (method === 'directly') {
      this.osrmLoader = new OSRMDirectLoader(this);
    } else if (method === 'mmap') {
      this.osrmLoader = new OSRMmmapLoader(this);
    } else {
      this.osrmLoader = null;
      throw new Error(`No such data load method: ${method}`);
    }
  }

  saveChildOutput(child) {
    this.stdout = child.stdout.toString();
    this.stderr = child.stderr.toString();
    this.exitCode = child.status;
    this.termSignal = child.signal;
  }

  resetChildOutput() {
    this.stdout = '';
    this.stderr = '';
    this.exitCode = null;
    this.termSignal = null;
  }

  // Cleanup method called from After hook
  async cleanup(_scenario) {
    if (this.osrmLoader)
      await this.osrmLoader.shutdown();
  }
}

// Register the custom World constructor
setWorldConstructor(OSRMWorld);
