// General utility functions for timeouts, decimal formatting, and file operations
import fs from 'fs';
import child_process from 'child_process';

// Creates timeout wrapper that calls callback with error if operation exceeds time limit
function Timeout(ms, options) {
  return function (cb) {
    let called = false;
    const timer = setTimeout(() => {
      if (!called) {
        called = true;
        cb(options.err || new Error(`Operation timed out after ${ms}ms`));
      }
    }, ms);

    return function (...args) {
      if (!called) {
        called = true;
        clearTimeout(timer);
        cb(...args);
      }
    };
  };
}

// Creates directory recursively, callback-style wrapper for mkdir
function createDir(dir, callback) {
  fs.mkdir(dir, { recursive: true })
    .then(() => callback(null))
    .catch((err) => callback(err));
}

// Ensures numeric values have decimal point for OSM XML compatibility
const ensureDecimal = (i) => {
  if (parseInt(i) === i) return i.toFixed(1);
  else return i;
};

// Formats error information from child process exits
const errorReason = (err) => {
  return err.signal
    ? `killed by signal ${err.signal}`
    : `exited with code ${err.code}`;
};

/**
 * Returns a promise that all osrm binaries are present and in good working order.
 * @param {Env} env
 */
function verifyExistenceOfBinaries(env) {
  for (const binPath of env.requiredBinaries) {
    if (!fs.existsSync(binPath)) {
      return Promise.reject(new Error(`*** ${binPath} is missing. Build failed?`));
    }
    const res = child_process.spawnSync(binPath, ['--help']);
    if (res.error) {
      return Promise.reject(res.error);
    };
  };
  return Promise.resolve();
}

export { createDir, ensureDecimal, errorReason, Timeout, verifyExistenceOfBinaries };
