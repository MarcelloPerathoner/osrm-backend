// Cucumber before/after hooks for test setup, teardown, and environment initialization
import { BeforeAll, Before, After, AfterAll, setDefaultTimeout } from '@cucumber/cucumber';

// Import the custom World constructor (registers itself via setWorldConstructor)
import './world.js';
import { env } from './env.js';

// Set global timeout for all steps and hooks
setDefaultTimeout(
  (process.env.CUCUMBER_TIMEOUT && parseInt(process.env.CUCUMBER_TIMEOUT)) ||
    5000,
);

BeforeAll((callback) => {
  env.initializeEnv(callback);
});

Before(function (testCase, callback) {
  // Initialize the World instance for this test case
  this.init(testCase, callback);
});

After(function (testCase, callback) {
  // Cleanup the World instance after this test case
  this.cleanup(callback);
});

AfterAll((callback) => {
  callback();
});
