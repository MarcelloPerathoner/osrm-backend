// Sets up global environment constants and configuration for test execution
import crypto from 'crypto';
import path from 'path';
import util from 'util';
import fs from 'fs';
import child_process from 'child_process';

import { testOsrmDown } from '../lib/osrm_loader.js';

// Sets up all constants that are valid for all features
class Env {
  // Initializes all environment constants and paths for test execution
  initializeEnv(callback) {
    this.TIMEOUT = parseInt(process.env.CUCUMBER_TIMEOUT) || 5000;
    this.ROOT_PATH = process.cwd();

    this.TEST_PATH = path.resolve(this.ROOT_PATH, 'test');
    this.CACHE_PATH = path.resolve(this.TEST_PATH, 'cache');
    this.LOGS_PATH = path.resolve(this.TEST_PATH, 'logs');

    this.PROFILES_PATH = path.resolve(this.ROOT_PATH, 'profiles');
    this.FIXTURES_PATH = path.resolve(this.ROOT_PATH, 'unit_tests/fixtures');
    this.BIN_PATH =
      process.env.OSRM_BUILD_DIR || path.resolve(this.ROOT_PATH, 'build');
    this.DATASET_NAME = 'cucumber';
    this.PLATFORM_WINDOWS = process.platform.match(/^win.*/);
    this.DEFAULT_ENVIRONMENT = process.env;
    this.DEFAULT_PROFILE = 'bicycle';
    this.DEFAULT_INPUT_FORMAT = 'osm';
    const loadMethod = process.env.OSRM_LOAD_METHOD || 'datastore';
    this.DEFAULT_LOAD_METHOD = loadMethod.match('mmap')
      ? 'mmap'
      : loadMethod.match('directly')
        ? 'directly'
        : 'datastore';
    this.DEFAULT_ORIGIN = [1, 1];
    this.OSM_USER = 'osrm';
    this.OSM_UID = 1;
    this.OSM_TIMESTAMP = '2000-01-01T00:00:00Z';
    this.WAY_SPACING = 100;
    this.DEFAULT_GRID_SIZE = 100; // meters
    // get algorithm name from the command line profile argument
    this.ROUTING_ALGORITHM = process.argv[process.argv.indexOf('-p') + 1].match(
      'mld',
    )
      ? 'MLD'
      : 'CH';
    this.TIMEZONE_NAMES = this.PLATFORM_WINDOWS ? 'win' : 'iana';

    this.OSRM_PORT = parseInt(process.env.OSRM_PORT) || 5000;
    this.OSRM_IP = process.env.OSRM_IP || '127.0.0.1';

    this.HOST = `http://${this.OSRM_IP}:${this.OSRM_PORT}`;

    this.OSRM_PROFILE = process.env.OSRM_PROFILE;

    if (this.PLATFORM_WINDOWS) {
      this.TERMSIGNAL = 9;
      this.EXE = '.exe';
    } else {
      this.TERMSIGNAL = 'SIGTERM';
      this.EXE = '';
    }

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
    this.extraction_binaries = [];
    /** libraries responsible for the cached files */
    this.libraries = [];
    /** binaries that must be present */
    this.all_binaries = [];

    for (const i of ['extract', 'contract', 'customize', 'partition']) {
      const bin = path.join(this.BIN_PATH, `osrm-${i}${this.EXE}`);
      this.extraction_binaries.push(bin);
      this.all_binaries.push(bin);
    }
    for (const i of 'routed'.split()) {
      this.all_binaries.push(path.join(this.BIN_PATH, `osrm-${i}${this.EXE}`));
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

    // we need this port to spawn our own osrm-routed
    testOsrmDown(callback);
    this.verifyExistenceOfBinaries(callback);

    callback(); // success
  }

  getProfilePath(profile) {
    return path.resolve(this.PROFILES_PATH, `${profile}.lua`);
  }

  // returns a hash of all OSRM code side dependencies
  // that is: all osrm binaries and all lua profiles
  getOSRMHash() {
    const dependencies = this.extraction_binaries.concat(this.libraries);

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

  verifyExistenceOfBinaries(callback) {
    for (const binPath of this.all_binaries) {
      if (!fs.existsSync(binPath)) {
        return callback(
          new Error(`*** ${binPath} is missing. Build failed?`)
        );
      }
      const res = child_process.spawnSync(binPath, ['--help']);
      if (res.error) {
        return callback(res.error);
      };
    };
  }
}

/** Global environment */
export const env = new Env();
