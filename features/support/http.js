// HTTP client utilities for making API requests to OSRM routing server
import { env } from './env.js';

export default class Http {
  constructor(world) {
    this.world = world;
  }

  paramsToString(params) {
    let paramString = '';
    if (params.coordinates !== undefined) {
      // FIXME this disables passing the output if its a default
      // Remove after #2173 is fixed.
      const outputString =
        params.output && params.output !== 'json' ? `.${params.output}` : '';
      paramString = params.coordinates.join(';') + outputString;
      delete params.coordinates;
      delete params.output;
    }
    if (Object.keys(params).length) {
      paramString += `?${Object.keys(params)
        .map((k) => `${k}=${params[k]}`)
        .join('&')}`;
    }

    return paramString;
  }

  sendRequest(baseUri, parameters, callback) {
    const params = this.paramsToString(parameters);
    const query = baseUri + (params.length ? `/${params}` : '');
    const req = env.client.get (query, { agent: env.agent }, (res) => {
      let data = '';

      // Collect data chunks
      res.on('data', (chunk) => {
        data += chunk;
      });

      // Handle end of response
      res.on('end', () => {
        callback(null, res, data);
      });
    });

    // Handle errors
    req.on('error', (err) => {
      callback(err);
    });

    req.end();
  }
}
