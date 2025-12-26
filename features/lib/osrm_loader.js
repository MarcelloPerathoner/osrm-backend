// osrm-routed process management and data loading strategies (datastore, mmap, direct)
import child_process from 'child_process';
import waitOn from 'wait-on';

import { env } from '../support/env.js';
import { runBinSync, mkBinPath } from '../support/run.js';
import { errorReason } from '../lib/utils.js';

/**
 * A class for running osrm-routed. Subclasses implement alternate ways of data loading.
 */
class OSRMBaseLoader {
  constructor() {
    this.child = null;
  }

  /**
   * Starts OSRM server and waits for it to accept connections
   *
   * @param log A log function. Inside a scenario we can log to the world.log()
   * function, but world.log() cannot be used between scenarios. We must log
   * long-running processes to the global log.
   */
  spawn(scenario, args, log) {
    if (this.osrmIsRunning())
      throw new Error('osrm-routed already running!');

    const cmd = mkBinPath('osrm-routed');
    const argsAsString = args.join(' ');
    log(`*** running ${cmd} ${argsAsString}\n`);

    this.child = child_process.spawn(
      cmd,
      args,
      { env : scenario.environment },
    );

    this.child.on('exit', (code) => {
      log(`*** osrm-routed exited with code ${code}\n`);
      this.child = null;
      if (code != 0) {
        throw new Error(`osrm-routed ${errorReason(err)}: ${err.cmd}`);
      }
    });

    return this.waitForConnection();
  }

  // Terminates OSRM server process gracefully
  kill() {
    if (this.osrmIsRunning()) {
      const child = this.child;
      const p = new Promise((resolve) => {
        child.on('exit', resolve);
      });
      child.kill();
      return p;
    }
    return Promise.resolve();
  }

  /** Returns a promise resolved when the server is up. */
  waitForConnection() {
    const waitOptions = {
      resources: [`tcp:${env.OSRM_IP}:${env.OSRM_PORT}`],
      delay:    10, // initial delay in ms
      interval: 10, // poll interval in ms
      timeout:  env.TIMEOUT, // timeout in ms
    };
    const p = waitOn(waitOptions);
    p.catch(() => { throw new Error(
      `Could not connect to osrm-routed after ${waitOptions.timeout} ms.`
    ); });
    return p;
  }

  osrmIsRunning() {
    return this.child; //  && !this.child.killed;
  }

  // public interface

  /** Called at the init of the cucumber run */
  beforeAll() { return Promise.resolve(); }
  /** Called at the start of each scenario */
  before(_scenario) {
    return Promise.resolve();
  }
  /** Called at the end of each scenario */
  after(_scenario) {
    return this.kill();
  }
  /** Called at the end of the cucumber run */
  afterAll() { return Promise.resolve(); }
}

/** This loader tells osrm-routed to load data directly from .osrm files into memory */
export class OSRMDirectLoader extends OSRMBaseLoader {
  before(scenario) {
    return this.spawn(scenario, [
      scenario.processedCacheFile,
      '-p',
      env.OSRM_PORT,
      '-i',
      env.OSRM_IP,
      '-a',
      env.ROUTING_ALGORITHM,
    ].concat(scenario.loaderArgs),
    scenario.log);
  }
}

/** This loader tells osrm-routed to use memory-mapped files. */
export class OSRMmmapLoader extends OSRMBaseLoader {
  before(scenario) {
    return this.spawn(scenario, [
      scenario.processedCacheFile,
      '-p',
      env.OSRM_PORT,
      '-i',
      env.OSRM_IP,
      '-a',
      env.ROUTING_ALGORITHM,
      '--mmap',
    ].concat(scenario.loaderArgs),
    scenario.log);
  }
}

/**
 * This loader keeps one and the same osrm-routed running for the whole cucumber run. It
 * uses osrm-datastore to load new data into osrm-routed.
 */
export class OSRMDatastoreLoader extends OSRMBaseLoader {
  constructor() {
    super();
    this.current_scenario = null;
  }

  /**
   * Custom log function
   *
   * For long-running osrm-routed switch the log to the current scenario
   */
  async logSync(msg) {
    if (this.current_scenario)
      await this.current_scenario.log(msg);
    else
      env.globalLog(msg);
  }

  before(scenario) {
    this.current_scenario = scenario;
    this.semaphore = new Promise((resolve) => this.resolve = resolve);
    runBinSync(
      'osrm-datastore',
      [
        '--dataset-name',
        env.DATASET_NAME,
        scenario.processedCacheFile,
      ].concat(scenario.loaderArgs),
      { env : scenario.environment },
      scenario.log
    );

    if (this.osrmIsRunning())
      return this.semaphore;

    // workaround for annoying misfeature: if there are no datastores osrm-routed
    // chickens out, so we cannot just start osrm-routed in beforeAll where it naturally
    // belonged, but must load a datastore first.

    const args = [
      '--shared-memory',
      '--dataset-name',
      env.DATASET_NAME,
      '-p',
      env.OSRM_PORT,
      '-i',
      env.OSRM_IP,
      '-a',
      env.ROUTING_ALGORITHM,
    ]; // .concat(scenario.loaderArgs));

    const cmd = mkBinPath('osrm-routed');
    const argsAsString = args.join(' ');
    this.logSync(`*** running ${cmd} ${argsAsString}\n`);

    this.child = child_process.spawn(
      cmd,
      args,
      { env : scenario.environment },
    );

    this.child.stdout.on('data', (data) => {
      if (data.includes('updated facade'))
        this.resolve();
    });

    // we MUST consume these or the osrm-routed process will block eventually
    this.child.stderr.on('data', (data) => this.logSync(`osrm-routed stderr:\n${data}`));
    this.child.stdout.on('data', (data) => this.logSync(`osrm-routed stdout:\n${data}`));

    this.child.on('exit', (code, signal) => {
      this.child = null;
      if (code != null) {
        this.logSync(`*** osrm-routed exited with code ${code}\n`);
        if (code != 0)
          throw new Error(`osrm-routed ${errorReason(err)}: ${err.cmd}`);
      }
      if (signal != null) {
        this.logSync(`*** osrm-routed exited with signal ${signal}\n`);
      }
    });
    return this.waitForConnection();
  }
  after () {
    this.current_scenario = null;
    return Promise.resolve();
  }
  afterAll () {
    this.current_scenario = null;
    return this.kill();
  }
}

/** throws error if osrm-routed is up */
// FIXME: don't bother if osrm-routed is already running, just use the next port
export function testOsrmDown() {
  const host = `${env.OSRM_IP}:${env.OSRM_PORT}`;
  const waitOptions = {
    resources: [`tcp:${host}`],
    delay:       0, // initial delay in ms
    interval:  100, // poll interval in ms
    timeout:  1000, // timeout in ms
    reverse: true,
  };
  return waitOn(waitOptions).catch(() => { throw new Error(
    `*** osrm-routed is already running on ${host}.`
  );});
}
