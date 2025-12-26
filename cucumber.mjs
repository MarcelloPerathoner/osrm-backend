// See: https://github.com/cucumber/cucumber-js/blob/main/docs/profiles.md

const common = {
  strict: true,
  import: [
    'features/support/',
    'features/step_definitions/',
    'features/lib/'
  ],
}

// Default profile
export default {
  ...common,
  tags: 'not @stress and not @todo and not @mld',
};

// Additional profiles
export const all = {
  ...common,
};

export const ch = {
  ...common,
  tags: 'not @stress and not @todo and not @mld',
  format: ['progress', 'json:test/logs/cucumber.log.json'],
};

export const todo = {
  ...common,
  tags: '@todo',
};

export const mld = {
  ...common,
  tags: 'not @stress and not @todo and not @ch',
  format: ['progress', 'json:test/logs/cucumber.log.json'],
};
