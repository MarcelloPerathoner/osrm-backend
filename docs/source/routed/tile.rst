.. _tile:

Tile Service
============

.. http:get:: /tile/v1/(profile)/tile(x,y,zoom).mvt

   This service generates `Mapbox Vector Tiles
   <https://www.mapbox.com/developers/vector-tiles/>`_ that can be viewed with a
   vector-tile capable slippy-map viewer.  The tiles contain road geometries and
   metadata that can be used to examine the routing graph.  The tiles are generated
   directly from the data in-memory, so are in sync with actual routing results, and let
   you examine which roads are actually routable, and what weights they have applied.

   .. dropdown:: Example Request

      .. code:: bash

         # This fetches a Z=13 tile for downtown San Francisco:
         curl 'http://router.project-osrm.org/tile/v1/car/tile(1310,3166,13).mvt'

   .. dropdown:: Example response

      .. figure:: ../images/example-tile-response.png

         example rendered tile

         http://map.project-osrm.org/debug/#14.33/52.5212/13.3919

   :param integer x:
   :param integer y:
   :param integer zoom:
      The `x`, `y`, and `zoom` values are the same as described at
      https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames, and are supported by vector
      tile viewers like `Mapbox GL JS <https://www.mapbox.com/mapbox-gl-js/api/>`_.

   The response object is either a binary encoded blob with a `Content-Type` of
   `application/x-protobuf`, or a `404` error.  Note that OSRM is hard-coded to only
   return tiles from zoom level 12 and higher (to avoid accidentally returning extremely
   large vector tiles).

   Vector tiles contain two layers:

   **speeds layer**

   .. list-table::
      :header-rows: 1

      * - Property
        - Type
        - Description
      * - `speed`
        - `integer`
        - the speed on that road segment, in km/h
      * - `is_small`
        - `boolean`
        - whether this segment belongs to a small (< 1000 node) `strongly connected
          component <https://en.wikipedia.org/wiki/Strongly_connected_component>`_
      * - `datasource`
        - `string`
        - the source for the speed value (normally `lua profile` unless you're using the
          `traffic update feature
          <https://github.com/Project-OSRM/osrm-backend/wiki/Traffic>`_, in which case
          it contains the stem of the filename that supplied the speed value for this
          segment
      * - `duration`
        - `float`
        - how long this segment takes to traverse, in seconds.  This value is to
          calculate the total route ETA.
      * - `weight`
        - `integer`
        - how long this segment takes to traverse, in units (may differ from `duration`
          when artificial biasing is applied in the Lua profiles).  ACTUAL ROUTING USES
          THIS VALUE.
      * - `name`
        - `string`
        - the name of the road this segment belongs to
      * - `rate`
        - `float`
        - the value of `length/weight` - analogous to `speed`, but using the `weight`
          value rather than `duration`, rounded to the nearest integer
      * - `is_startpoint`
        - `boolean`
        - whether this segment can be used as a start/endpoint for routes

   **turns layer**

   .. list-table::
      :header-rows: 1

      * - Property
        - Type
        - Description
      * - `bearing_in`
        - `integer`
        - the absolute bearing that approaches the intersection.  -180 to +180, 0 =
          North, 90 = East
      * - `turn_angle`
        - `integer`
        - the angle of the turn, relative to the `bearing_in`.  -180 to +180, 0 =
          straight ahead, 90 = 90-degrees to the right
      * - `cost`
        - `float`
        - the time we think it takes to make that turn, in seconds.  May be negative,
          depending on how the data model is constructed (some turns get a "bonus").
      * - `weight`
        - `float`
        - the weight we think it takes to make that turn.  May be negative, depending on
          how the data model is constructed (some turns get a "bonus"). ACTUAL ROUTING
          USES THIS VALUE
      * - `type`
        - `string`
        - the type of this turn - values like `turn`, `continue`, etc.  See the
          `StepManeuver` for a partial list, this field also exposes internal turn types
          that are never returned with an API response
      * - `modifier`
        - `string`
        - the direction modifier of the turn (`left`, `sharp left`, etc)
