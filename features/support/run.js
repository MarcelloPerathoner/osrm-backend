// Process execution utilities for running OSRM binaries and managing subprocesses
import path from 'path';
import fs from 'fs';
import child_process from 'child_process';

import { env } from '../support/env.js';

export default class Run {
  constructor(world) {
    this.world = world;
  }

  // replaces placeholders for in user supplied commands
  expandOptions(options) {
    const table = {
      '{osm_file}': this.inputCacheFile,
      '{processed_file}': this.processedCacheFile,
      '{profile_file}': this.profileFile,
      '{rastersource_file}': this.rasterCacheFile,
      '{speeds_file}': this.speedsCacheFile,
      '{penalties_file}': this.penaltiesCacheFile,
      '{timezone_names}': env.TIMEZONE_NAMES,
    };

    const opts = [];
    for (const option of options.split(/\s+/)) {
      opts.push(table[option] || option);
    }

    return opts;
  }

  setupOutputLog(process, log) {
    if (process.logFunc) {
      process.stdout.removeListener('data', process.logFunc);
      process.stderr.removeListener('data', process.logFunc);
    }

    process.logFunc = (message) => {
      log.write(message);
    };
    process.stdout.on('data', process.logFunc);
    process.stderr.on('data', process.logFunc);
  }

  mkPath(bin) {
    return path.resolve(path.join(env.BIN_PATH, `${bin}${env.EXE}`));
  }

  runBin(bin, args, environment) {
    const cmd = this.mkPath(bin);
    const argsAsString = args.join(' ');
    const log = fs.createWriteStream(this.scenarioLogFile, { flags: 'a' });
    log.write(`*** running ${cmd} ${argsAsString}\n`);

    // we need to set a large maxbuffer here because we have long running processes like osrm-routed
    // with lots of log output
    const child = child_process.spawn(
      cmd,
      args,
      { env: environment, maxBuffer: 1024 * 1024 * 1000 }
    );

    child.on('exit', (code) => {
      log.write(`*** ${bin} exited with code ${code}\n`);
      log.end();
    });

    // Don't setup output logging as it interferes with execFile's output capture
    // this.setupOutputLog(child, log);
    return child;
  }

  runBinSync(bin, args, options) {
    const cmd = this.mkPath(bin);
    const argsAsString = args.join(' ');
    const log = fs.createWriteStream(this.scenarioLogFile, { flags: 'a' });
    log.write(`*** running ${cmd} ${argsAsString}\n`);

    options.timeout = options.timeout || this.environment.TIMEOUT;

    const child = child_process.spawnSync(
      cmd,
      args,
      options
    );

    log.write(`*** ${bin} exited with code ${child.status}\n`);
    log.end();
    return child;
  }
}
