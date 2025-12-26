// See: https://github.com/cucumber/cucumber-js/blob/main/docs/profiles.md

export default function() {

  const common = {
    strict: true,
    import: [
      'features/support/',
      'features/step_definitions/',
      'features/lib/'
    ],
    worldParameters: {
      loadMethod: 'datastore',
      algorithm:  'ch',
    },
    format: ['progress'],
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

    todo: {
      ...common,
      tags: '@todo',
    },

    // algorithms
    ch: {
      ...common,
      tags: 'not @stress and not @todo and not @mld',
      worldParameters: {'algorithm': 'ch'},
    },

    mld: {
      ...common,
      tags: 'not @stress and not @todo and not @ch',
      worldParameters: {'algorithm': 'mld'},
    },

    // data load methods
    datastore: {
      worldParameters: {'loadMethod': 'datastore'},
    },

    directly: {
      worldParameters: {'loadMethod': 'directly'},
    },

    mmap: {
      worldParameters: {'loadMethod': 'mmap'},
    },
  }
};
