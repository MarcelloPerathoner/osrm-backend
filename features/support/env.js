// Sets up global environment constants and configuration for test execution
import crypto from 'crypto';
import fs from 'node:fs';
import http from 'node:http';
import https from 'node:https';
import path from 'path';
import util from 'util';

import { OSRMDatastoreLoader, OSRMDirectLoader, OSRMmmapLoader } from '../lib/osrm_loader.js';

/** Global environment for all scenarios. */
class Env {
  // Initializes all environment constants and paths for test execution
  beforeAll(worldParameters) {
    this.TIMEOUT = parseInt(process.env.CUCUMBER_TIMEOUT) || 5000;
    this.HTTP_TIMEOUT = parseInt(process.env.CUCUMBER_HTTP_TIMEOUT) || 3000;
    this.ROOT_PATH = process.cwd();

    this.TEST_PATH = path.resolve(this.ROOT_PATH, 'test');
    this.CACHE_PATH = path.resolve(this.TEST_PATH, 'cache');
    this.LOGS_PATH = path.resolve(this.TEST_PATH, 'logs');

    this.PROFILES_PATH = path.resolve(this.ROOT_PATH, 'profiles');
    this.FIXTURES_PATH = path.resolve(this.ROOT_PATH, 'unit_tests/fixtures');
    this.BIN_PATH =
      process.env.OSRM_BUILD_DIR || path.resolve(this.ROOT_PATH, 'build');
    this.PLATFORM_WINDOWS = process.platform.match(/^win.*/);
    this.DEFAULT_ENVIRONMENT = process.env;
    this.DEFAULT_PROFILE = 'bicycle';
    this.DEFAULT_INPUT_FORMAT = 'osm';
    this.DEFAULT_LOAD_METHOD = worldParameters.loadMethod || 'datastore';
    this.ROUTING_ALGORITHM = (worldParameters.algorithm || 'ch').toUpperCase();
    this.DEFAULT_ORIGIN = [1, 1];
    this.OSM_USER = 'osrm';
    this.OSM_UID = 1;
    this.OSM_TIMESTAMP = '2000-01-01T00:00:00Z';
    this.WAY_SPACING = 100;
    this.DEFAULT_GRID_SIZE = 100; // meters

    this.CUCUMBER_WORKER_ID = parseInt(process.env.CUCUMBER_WORKER_ID || '0');
    this.OSRM_PORT = (parseInt(process.env.OSRM_PORT || '5000') + this.CUCUMBER_WORKER_ID).toString();
    this.OSRM_IP = process.env.OSRM_IP || '127.0.0.1';

    this.HOST = `http://${this.OSRM_IP}:${this.OSRM_PORT}`;

    if (this.HOST.startsWith('https')) {
      this.client = https;
      this.agent = new https.Agent ({
        timeout: this.HTTP_TIMEOUT,
        defaultPort: this.OSRM_PORT,
      });
    } else {
      this.client = http;
      this.agent = new http.Agent ({
        timeout: this.HTTP_TIMEOUT,
        defaultPort: this.OSRM_PORT,
      });
    }

    this.OSRM_PROFILE = process.env.OSRM_PROFILE;
    this.DATASET_NAME = `cucumber${this.CUCUMBER_WORKER_ID}`;

    if (this.PLATFORM_WINDOWS) {
      this.TERMSIGNAL = 9;
      this.EXE = '.exe';
    } else {
      this.TERMSIGNAL = 'SIGTERM';
      this.EXE = '';
    }
    this.CI = process.env.GITHUB_ACTIONS != undefined;

    // a log file for the long running process osrm-routed
    this.globalLogfile = fs.openSync(
      path.resolve(this.LOGS_PATH, `cucumber-global-${this.CUCUMBER_WORKER_ID}.log`),
      'a');

    // heuristically detect .so/.a/.dll/.lib suffix
    this.LIB = ['lib%s.a', 'lib%s.so', '%s.dll', '%s.lib'].find((format) => {
      try {
        const lib = `${this.BIN_PATH}/${util.format(format, 'osrm')}`;
        fs.accessSync(lib, fs.constants.F_OK);
      } catch {
        return false;
      }
      return true;
    });

    if (this.LIB === undefined) {
      throw new Error(
        '*** Unable to detect dynamic or static libosrm libraries',
      );
    }

    /** binaries responsible for the cached files */
    this.extractionBinaries = [];
    /** libraries responsible for the cached files */
    this.libraries = [];
    /** binaries that must be present */
    this.requiredBinaries = [];

    for (const i of ['extract', 'contract', 'customize', 'partition']) {
      const bin = path.join(this.BIN_PATH, `osrm-${i}${this.EXE}`);
      this.extractionBinaries.push(bin);
      this.requiredBinaries.push(bin);
    }
    for (const i of 'routed'.split()) {
      this.requiredBinaries.push(path.join(this.BIN_PATH, `osrm-${i}${this.EXE}`));
    }
    for (const i of ['_extract', '_contract', '_customize', '_partition', '']) {
      const lib = path.join(this.BIN_PATH, util.format(this.LIB, `osrm${i}`));
      this.libraries.push(lib);
    }

    if (!fs.existsSync(this.TEST_PATH)) {
      callback(new Error(`*** Test folder doesn't exist: ${this.TEST_PATH}`));
    };

    /** A hash of all osrm binaries and lua profiles */
    this.osrmHash = this.getOSRMHash();

    this.setLoadMethod(this.DEFAULT_LOAD_METHOD);
  }

  async afterAll() {
    await this.osrmLoader.afterAll();
    fs.closeSync(this.globalLogfile);
    this.globalLogfile = null;
  }

  globalLog(msg) {
    fs.writeSync(this.globalLogfile, msg);
  }

  setLoadMethod(method) {
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

  getProfilePath(profile) {
    return path.resolve(this.PROFILES_PATH, `${profile}.lua`);
  }

  // returns a hash of all OSRM code side dependencies
  // that is: all osrm binaries and all lua profiles
  getOSRMHash() {
    const dependencies = this.extractionBinaries.concat(this.libraries);

    const addLuaFiles = function (directory) {
      const luaFiles = fs.readdirSync(path.normalize(directory))
        .filter((f) => !!f.match(/\.lua$/))
        .map((f) => path.normalize(`${directory}/${f}`));
      Array.prototype.push.apply(dependencies, luaFiles);
    };

    addLuaFiles(this.PROFILES_PATH);
    addLuaFiles(`${this.PROFILES_PATH}/lib`);

    const checksum = crypto.createHash('md5');
    for (const file of dependencies) {
      checksum.update(fs.readFileSync(file));
    }
    return checksum.digest('hex');
  }
}

/** Global environment */
export const env = new Env();
