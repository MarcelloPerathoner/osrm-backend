// Cucumber before/after hooks for test setup, teardown, and environment initialization
import { BeforeAll, Before, After, AfterAll, setDefaultTimeout } from '@cucumber/cucumber';
import { setParallelCanAssign } from '@cucumber/cucumber';

// Import the custom World constructor (registers itself via setWorldConstructor)
import './world.js';
import { env } from './env.js';

// Set global timeout for all steps and hooks
setDefaultTimeout(
  (process.env.CUCUMBER_TIMEOUT && parseInt(process.env.CUCUMBER_TIMEOUT)) ||
    5000,
);

// Do not run @isolated scenarios in parallel
const myCustomRule = function (pickleInQuestion, picklesInProgress) {
  for (const tag of pickleInQuestion.tags) {
    if (tag.name === '@isolated')
      return picklesInProgress.length == 0;
  }
  // No other restrictions
  return true;
};

setParallelCanAssign((pickleInQuestion, picklesInProgress) => {
  return myCustomRule(pickleInQuestion, picklesInProgress);
});

BeforeAll({timeout: 4999}, (callback) => {
  env.initializeEnv(callback);
});

Before({timeout: 4998}, async function (scenario) {
  // Initialize the World instance for this test case
  await this.init(scenario);
});

After({timeout: 4997}, async function (scenario) {
  // Cleanup the World instance after this test case
  await this.cleanup(scenario);
});

AfterAll({timeout: 4996}, (callback) => {
  callback();
});
