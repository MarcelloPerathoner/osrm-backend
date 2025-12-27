// HTTP client utilities for making API requests to OSRM routing server
import { env } from './env.js';

export default class Http {
  constructor(world) {
    this.world = world;
  }

  sendRequest(url, callback) {
    const req = env.client.get (url, { agent: env.agent }, (res) => {
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

    // Handle timeout
    req.on('timeout', (err) => {
      req.destroy();
      callback(err);
    });

    // Handle errors
    req.on('error', (err) => {
      callback(err);
    });

    req.end();
  }
}
