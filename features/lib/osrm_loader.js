// osrm-routed process management and data loading strategies (datastore, mmap, direct)
import fs from 'fs';
import waitOn from 'wait-on';
import { env } from '../support/env.js';
import { errorReason } from '../lib/utils.js';

// Base class for managing OSRM routing server process lifecycle
class OSRMBaseLoader {
  constructor(world) {
    this.world = world;
    this.child = null;
    this.args = [];
  }

  // Starts OSRM server and waits for it to accept connections
  launch() {
    if (this.osrmIsRunning())
      throw new Error('osrm-routed already running!');

    this.child = this.world.runBin(
      'osrm-routed',
      this.args,
      this.world.environment
    );
    this.child.on('exit', (code) => {
      if (code < 0 && this.child.signal != 'SIGINT') {
        throw new Error(`osrm-routed ${errorReason(err)}: ${err.cmd}`);
      }
    });
    return this.waitForConnection();
  }

  // Terminates OSRM server process gracefully
  shutdown() {
    if (this.osrmIsRunning()) {
      const child = this.child;
      const p = new Promise((resolve) => {
        child.on('exit', resolve);
      });
      child.kill('SIGINT');
      return p;
    }
    return Promise.resolve();
  }

  osrmIsRunning() {
    return this.child && !this.child.killed;
  }

  async load(_ctx) {}

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
}

// Loads data directly from .osrm files into memory
export class OSRMDirectLoader extends OSRMBaseLoader {
  async load(ctx) {
    this.args = [
      ctx.inputFile,
      '-p',
      env.OSRM_PORT,
      '-i',
      env.OSRM_IP,
      '-a',
      env.ROUTING_ALGORITHM,
    ].concat(ctx.loaderArgs);
    await this.shutdown();
    await this.launch();
  }
}

// Uses memory-mapped files for efficient data access
export class OSRMmmapLoader extends OSRMBaseLoader {
  async load(ctx) {
    this.args = [
      ctx.inputFile,
      '-p',
      env.OSRM_PORT,
      '-i',
      env.OSRM_IP,
      '-a',
      env.ROUTING_ALGORITHM,
      '--mmap',
    ].concat(ctx.loaderArgs);
    await this.shutdown();
    await this.launch();
  }
}

// Loads data into shared memory for multiple processes to access
export class OSRMDatastoreLoader extends OSRMBaseLoader {
  async load(ctx) {
    this.world.runBinSync(
      'osrm-datastore',
      [
        '--dataset-name',
        env.DATASET_NAME,
        ctx.inputFile,
      ].concat(ctx.loaderArgs),
      { env: this.world.environment }
    );
    this.args = [
      '--shared-memory',
      '--dataset-name',
      env.DATASET_NAME,
      '-p',
      env.OSRM_PORT,
      '-i',
      env.OSRM_IP,
      '-a',
      env.ROUTING_ALGORITHM,
    ]; // FIXME ???? .concat(ctx.loaderArgs);
    await this.shutdown();
    await this.launch();
    this.world.setupOutputLog(
      this.child,
      fs.createWriteStream(this.world.scenarioLogFile, { flags: 'a' }),
    );
  }
}

/** throws error if osrm-routed is up */
export async function testOsrmDown() {
  const host = `${env.OSRM_IP}:${env.OSRM_PORT}`;
  const waitOptions = {
    resources: [`tcp:${host}`],
    delay:    0, // initial delay in ms
    interval: 10, // poll interval in ms
    timeout:  env.TIMEOUT, // timeout in ms
    reverse: true,
  };
  await waitOn(waitOptions).catch(() => { throw new Error(
    `*** osrm-routed is already running on ${host}.`
  );});
}
