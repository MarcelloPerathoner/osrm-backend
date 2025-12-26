// See: https://github.com/cucumber/cucumber-js/blob/main/docs/profiles.md

export default function() {

  const loadMethod = process.env.OSRM_LOAD_METHOD || 'datastore';
  const algorithm = process.env.ROUTING_ALGORITHM || 'mld';
  const baseFormat = process.env.GITHUB_ACTIONS ? 'summary' : 'progress';
  const jsonFormat = `json:test/logs/cucumber.${algorithm}.${loadMethod}.log.json`;
  const htmlFormat = `html:test/logs/cucumber.${algorithm}.${loadMethod}.log.html`;

  const common = {
    strict: true,
    import: [
      'features/support/',
      'features/step_definitions/',
      'features/lib/'
    ],
    format: [baseFormat, htmlFormat, jsonFormat]
  }

  return {
    // Default profile
    default: {
      ...common,
      tags: 'not @stress and not @todo and not @mld',
    },

    // Additional profiles
    all: {
      ...common,
    },

    ch: {
      ...common,
      tags: 'not @stress and not @todo and not @mld',
    },

    todo: {
      ...common,
      tags: '@todo',
    },

    mld: {
      ...common,
      tags: 'not @stress and not @todo and not @ch',
    }
  }
};
