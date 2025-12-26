// Custom World class for OSRM test environment using modern Cucumber.js v13 patterns
import path from 'path';
import { World, setWorldConstructor } from '@cucumber/cucumber';

import * as OSM from '../lib/osm.js';
import { env } from '../support/env.js';

import Cache from './cache.js';
import Data from './data.js';
import Route from './route.js';
import Fuzzy from './fuzzy.js';
import SharedSteps from './shared_steps.js';

class OSRMWorld extends World {
  // Private instances of support classes for clean composition
  #data;
  #route;
  #sharedSteps;
  #fuzzy;

  constructor(options) {
    // Get built-in Cucumber helpers: this.attach, this.log, this.parameters
    super(options);
    this.loadMethod = null;

    // Initialize service instances with access to world
    this.#data = new Data(this);
    this.#route = new Route(this);
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
      this.#route,
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
  get route() {
    return this.#route;
  }
  get sharedSteps() {
    return this.#sharedSteps;
  }
  get fuzzy() {
    return this.#fuzzy;
  }

  before(scenario) {
    this.cache = new Cache(env, scenario);
    this.setupCurrentScenario(this.cache, scenario);
    this.resetChildOutput();
    return Promise.resolve();
  }

  after(scenario) {
    return env.osrmLoader.after(scenario);
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
    // will be expanded eg. for OSRM_RASTER_SOURCE
    this.environment   = Object.assign({}, env.DEFAULT_ENVIRONMENT);
    this.resetOSM();

    const scenarioID = cache.getScenarioID(scenario);

    this.scenarioCacheFile  = cache.getScenarioCacheFile(scenarioID);
    this.processedCacheFile = cache.getProcessedCacheFile(scenarioID);
    this.inputCacheFile     = cache.getInputCacheFile(scenarioID);
    this.rasterCacheFile    = cache.getRasterCacheFile(scenarioID);
    this.speedsCacheFile    = cache.getSpeedsCacheFile(scenarioID);
    this.penaltiesCacheFile = cache.getPenaltiesCacheFile(scenarioID);
    this.profileCacheFile   = cache.getProfileCacheFile(scenarioID);
  }

  /**
   * Saves the output from a completed child process for testing against.
   *
   * @param {child_process} child The child process
   */
  saveChildOutput(child) {
    this.stderr = child.stderr.toString();
    this.stdout = child.stdout.toString();
    this.exitCode = child.status;
    this.termSignal = child.signal;
  }

  resetChildOutput() {
    this.stdout = '';
    this.stderr = '';
    this.exitCode = null;
    this.termSignal = null;
  }

  /**
   * Replaces placeholders in gherkin commands
   *
   * eg. it replaces {osm_file} with the input file path.
   */
  expandOptions(options) {
    const table = {
      'osm_file'          : this.inputCacheFile,
      'processed_file'    : this.processedCacheFile,
      'profile_file'      : this.profileFile,
      'rastersource_file' : this.rasterCacheFile,
      'speeds_file'       : this.speedsCacheFile,
      'penalties_file'    : this.penaltiesCacheFile,
      'timezone_names'    : env.PLATFORM_WINDOWS ? 'win' : 'iana'
    };

    function replacer(_match, p1) {
      return table[p1] || p1;
    }

    options = options.replaceAll(/\{(\w+)\}/g, replacer);
    return options.split(/\s+/);
  }
}

// Register the custom World constructor
setWorldConstructor(OSRMWorld);
