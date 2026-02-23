OSRM HTTP server
================

The binary `osrm-routed` is a basic HTTP/1.0 server that supports a 'keep-alive'
extension. Persistent connections are limited to 512 requests per connection and allow
no more than 5 seconds between requests.


.. toctree::
   :maxdepth: 2

   route.rst
   nearest.rst
   match.rst
   table.rst
   trip.rst
   tile.rst
   common.rst
   results_json.md
   results_flatbuffers.md
   environment_variables.rst
