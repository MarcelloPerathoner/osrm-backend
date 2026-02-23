OSRM HTTP server
================

The binary `osrm-routed` is a basic HTTP/1.0 server that supports a 'keep-alive'
extension. Persistent connections are limited to 512 requests per connection and allow
no more than 5 seconds between requests.


.. toctree::
   :maxdepth: 2

   http.md
   common.rst
   route.rst
   nearest.rst
   match.rst
   table.rst
