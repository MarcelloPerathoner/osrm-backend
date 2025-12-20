// OSRM binary process management and data loading strategies (datastore, mmap, direct)
import fs from 'fs';
import waitOn from 'wait-on';
import util from 'util';
import { Timeout, errorReason } from './utils.js';

// Base class for managing OSRM routing server process lifecycle
class OSRMBaseLoader {
  constructor(scope) {
    this.scope = scope;
    this.child = null;
  }

  // Starts OSRM server and waits for it to accept connections
  launch(callback) {
    this.osrmUp(() => {
      this.waitForConnection(callback);
    });
  }

  // Terminates OSRM server process gracefully
  shutdown(callback) {
    if (!this.osrmIsRunning()) return callback();

    const limit = Timeout(this.scope.TIMEOUT, {
      err: new Error('*** Shutting down osrm-routed timed out.'),
    });

    this.osrmDown(limit(callback));
  }

  osrmIsRunning() {
    return this.child && !this.child.killed;
  }

  osrmDown(callback) {
    if (this.osrmIsRunning()) {
      this.child.on('exit', (_code, _signal) => {
        callback();
      });
      this.child.kill('SIGINT');
    } else callback();
  }

  waitForConnection(callback) {
    const waitOptions = {
      resources: [`tcp:${this.scope.OSRM_IP}:${this.scope.OSRM_PORT}`],
      delay:    10, // initial delay in ms
      interval: 10, // poll interval in ms
      timeout:  this.scope.TIMEOUT, // timeout in ms
    };
    waitOn(waitOptions).then(callback, () => callback(new Error(
      `Could not connect to osrm-routed after ${waitOptions.timeout} ms.`
    )));
  }
}

// Loads data directly from .osrm files into memory
class OSRMDirectLoader extends OSRMBaseLoader {
  constructor(scope) {
    super(scope);
  }

  load(ctx, callback) {
    this.inputFile = ctx.inputFile;
    this.loaderArgs = ctx.loaderArgs;
    this.shutdown(() => {
      this.launch(callback);
    });
  }

  osrmUp(callback) {
    if (this.osrmIsRunning())
      return callback(new Error('osrm-routed already running!'));

    const command_arguments = util.format(
      '%s -p %d -i %s -a %s %s',
      this.inputFile,
      this.scope.OSRM_PORT,
      this.scope.OSRM_IP,
      this.scope.ROUTING_ALGORITHM,
      this.loaderArgs,
    );
    this.child = this.scope.runBin(
      'osrm-routed',
      command_arguments,
      this.scope.environment,
      (err) => {
        if (err && err.signal !== 'SIGINT') {
          this.child = null;
          throw new Error(
            util.format('osrm-routed %s: %s', errorReason(err), err.cmd),
          );
        }
      },
    );
    callback();
  }
}

// Uses memory-mapped files for efficient data access
class OSRMmmapLoader extends OSRMBaseLoader {
  constructor(scope) {
    super(scope);
  }

  load(ctx, callback) {
    this.inputFile = ctx.inputFile;
    this.loaderArgs = ctx.loaderArgs;
    this.shutdown(() => {
      this.launch(callback);
    });
  }

  osrmUp(callback) {
    if (this.osrmIsRunning())
      return callback(new Error('osrm-routed already running!'));

    const command_arguments = util.format(
      '%s -p %d -i %s -a %s --mmap %s',
      this.inputFile,
      this.scope.OSRM_PORT,
      this.scope.OSRM_IP,
      this.scope.ROUTING_ALGORITHM,
      this.loaderArgs,
    );
    this.child = this.scope.runBin(
      'osrm-routed',
      command_arguments,
      this.scope.environment,
      (err) => {
        if (err && err.signal !== 'SIGINT') {
          this.child = null;
          throw new Error(
            util.format('osrm-routed %s: %s', errorReason(err), err.cmd),
          );
        }
      },
    );
    callback();
  }
}

// Loads data into shared memory for multiple processes to access
class OSRMDatastoreLoader extends OSRMBaseLoader {
  constructor(scope) {
    super(scope);
  }

  load(ctx, callback) {
    this.inputFile = ctx.inputFile;
    this.loaderArgs = ctx.loaderArgs;

    this.loadData((err) => {
      if (err) return callback(err);
      if (!this.osrmIsRunning()) this.launch(callback);
      else {
        this.scope.setupOutputLog(
          this.child,
          fs.createWriteStream(this.scope.scenarioLogFile, { flags: 'a' }),
        );
        callback();
      }
    });
  }

  loadData(callback) {
    const command_arguments = util.format(
      '--dataset-name=%s %s %s',
      this.scope.DATASET_NAME,
      this.inputFile,
      this.loaderArgs,
    );
    this.scope.runBin(
      'osrm-datastore',
      command_arguments,
      this.scope.environment,
      (err) => {
        if (err)
          return callback(
            new Error(`*** osrm-datastore exited with ${err.code}: ${err}`),
          );
        callback();
      },
    );
  }

  osrmUp(callback) {
    if (this.osrmIsRunning()) return callback();

    const command_arguments = util.format(
      '--dataset-name=%s -s -i %s -p %d -a %s',
      this.scope.DATASET_NAME,
      this.scope.OSRM_IP,
      this.scope.OSRM_PORT,
      this.scope.ROUTING_ALGORITHM,
    );
    this.child = this.scope.runBin(
      'osrm-routed',
      command_arguments,
      this.scope.environment,
      (err) => {
        if (err && err.signal !== 'SIGINT') {
          this.child = null;
          throw new Error(
            util.format('osrm-routed %s: %s', errorReason(err), err.cmd),
          );
        }
      },
    );

    // we call the callback here, becuase we don't want to wait for the child process to finish
    callback();
  }
}

class OSRMLoader {
  constructor(scope) {
    this.scope = scope;
    this.sharedLoader = new OSRMDatastoreLoader(this.scope);
    this.directLoader = new OSRMDirectLoader(this.scope);
    this.mmapLoader = new OSRMmmapLoader(this.scope);
    this.method = scope.DEFAULT_LOAD_METHOD;
  }

  load(inputFile, callback) {
    if (!this.loader) {
      this.loader = { shutdown: (cb) => cb() };
    }
    if (this.method === 'datastore') {
      this.loader.shutdown((err) => {
        if (err) return callback(err);
        this.loader = this.sharedLoader;
        this.sharedLoader.load(inputFile, callback);
      });
    } else if (this.method === 'directly') {
      this.loader.shutdown((err) => {
        if (err) return callback(err);
        this.loader = this.directLoader;
        this.directLoader.load(inputFile, callback);
      });
    } else if (this.method === 'mmap') {
      this.loader.shutdown((err) => {
        if (err) return callback(err);
        this.loader = this.mmapLoader;
        this.mmapLoader.load(inputFile, callback);
      });
    } else {
      callback(new Error(`*** Unknown load method ${method}`));
    }
  }

  setLoadMethod(method) {
    this.method = method;
  }

  shutdown(callback) {
    if (!this.loader) return callback();

    this.loader.shutdown(callback);
  }

  up() {
    return this.loader ? this.loader.osrmIsRunning() : false;
  }
}

export default OSRMLoader;
