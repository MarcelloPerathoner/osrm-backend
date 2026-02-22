.. js:class:: OSRM(options)

   Creates an OSRM instance.  An OSRM instance requires a `.osrm.*` dataset (`.osrm.*`
   because it contains several files), which is prepared by the OSRM toolchain.  You can
   create such a `.osrm.*` dataset by running the OSRM binaries we ship in
   `node_modules/osrm/lib/binding_napi_v8/` and default profiles (e.g. for setting
   speeds and determining road types to route on) in `node_modules/osrm/profiles/`:

   .. code:: bash

      lib/binding_napi_v8/osrm-extract data.osm.pbf -p profiles/car.lua
      lib/binding_napi_v8/osrm-contract data.osrm

   Consult the `osrm-backend <http.html>`_ documentation for further details.

   Once you have a complete `network.osrm.*` dataset, you can calculate routes in
   javascript with this object.

   .. code:: js

      const osrm = new OSRM('network.osrm');

   :param Object|String options: Options for creating an OSRM object or path to the
      `.osrm` file.

   .. js:class:: options

      Options parameter to the constructor.

      .. js:attribute:: algorithm

         String: The algorithm to use for routing. Can be 'CH', or 'MLD'. Default is 'CH'.
         Make sure you prepared the dataset with the correct toolchain.

      .. js:attribute:: shared_memory

         Boolean: Connects to the persistent shared memory datastore.  This requires you to
         run `osrm-datastore` prior to creating an `OSRM` object.

      .. js:attribute:: dataset_name

         String: Connects to the persistent shared memory datastore defined
         by `--dataset_name` option when running `osrm-datastore`.
         This requires you to run `osrm-datastore --dataset_name` prior to creating an `OSRM` object.

      .. js:attribute:: memory_file

         String: **DEPRECATED** Old behaviour: Path to a file on disk to store the memory using mmap.
         Current behaviour: setting this value is the same as setting `mmap_memory: true`.

      .. js:attribute:: mmap_memory

         Boolean: Map on-disk files to virtual memory addresses (mmap), rather than loading into RAM.

      .. js:attribute:: path

         String: The path to the `.osrm` files. This is mutually exclusive with
         setting `shared_memory` to true.

      .. js:attribute:: disable_feature_dataset

         Array:  Disables a feature dataset from being loaded into memory if not needed.
         Options: `ROUTE_STEPS`, `ROUTE_GEOMETRY`.

      .. js:attribute:: max_locations_trip

         Number: Max. locations supported in trip query (default: unlimited).

      .. js:attribute:: max_locations_viaroute

         Number: Max. locations supported in viaroute query (default: unlimited).

      .. js:attribute:: max_locations_distance_tabl

         Number: Max. locations supported in distance table query (default: unlimited).

      .. js:attribute:: max_locations_map_matchin

         Number: Max. locations supported in map-matching query (default: unlimited).

      .. js:attribute:: max_radius_map_matchin

         Number: Max. radius size supported in map matching query (default: 5).

      .. js:attribute:: max_results_nearest

         Number: Max. results supported in nearest query (default: unlimited).

      .. js:attribute:: max_alternatives

         Number: Max. number of alternatives supported in alternative routes query (default: 3).

      .. js:attribute:: default_radius

         Number: Default radius for queries (default: unlimited).


   .. js:method:: route(options[, plugin_config], callback)

      Returns the fastest route between two or more coordinates while visiting the waypoints in order.

      :param Object options:       Object containing parameters for the route query.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: object containing an array of `Waypoint`_ objects representing all
                waypoints in order AND an array of `Route`_ objects ordered by
                descending recommendation rank.

      .. code:: js

         const osrm = new OSRM("berlin-latest.osrm");
         osrm.route({coordinates: [[52.519930,13.438640], [52.513191,13.415852]]}, function(err, result) {
           if(err) throw err;
           console.log(result.waypoints); // array of Waypoint objects representing all waypoints in order
           console.log(result.routes); // array of Route objects ordered by descending recommendation rank
         });

      .. js:class:: route_options

         .. js:attribute:: coordinates

            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]`
            values, in decimal degrees.

         .. js:attribute:: bearings

            Array: Limits the search to segments with given bearing in degrees towards true
            north in clockwise direction.  Can be `null` or an array of `[{value},{range}]`
            with `integer 0 .. 360,integer 0 .. 180`.

         .. js:attribute:: radiuses

            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `null` (unlimited, default) or `double >= 0`.

         .. js:attribute:: hints

            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         .. js:attribute:: exclude

            Array: List of classes to avoid, order does not matter.

         .. js:attribute:: generate_hints=true

            Boolean: Whether or not to return a hint which can be used in subsequent requests.

         .. js:attribute:: alternatives=false

            Boolean: Search for alternative routes.

         .. js:attribute:: alternatives=0

            Number: Search for up to this many alternative routes. *Please note that even if
            alternative routes are requested, a result cannot be guaranteed.*

         .. js:attribute:: steps=false

            Boolean: Return route steps for each route leg.

         .. js:attribute:: annotations=false

            Array|Boolean: An array contaning strings of `duration`, `nodes`, `distance`,
            `weight`, `datasources`, `speed` or a boolean for enabling/disabling all.

         .. js:attribute:: geometries=polyline

            String: Returned route geometry format (influences overview and per step). Can also be `geojson`.

         .. js:attribute:: overview=simplified

            String: Add overview geometry either `full`, `simplified` according to highest
            zoom level it could be display on, or not at all (`false`).

         .. js:attribute:: continue_straight

            Boolean: Forces the route to keep going straight at waypoints and don't do a uturn
            even if it would be faster. Default value depends on the profile.

         .. js:attribute:: approaches

            Array: Restrict the direction on the road network at a waypoint, relative to the
            input coordinate.  Can be `null` (unrestricted, default), `curb` or `opposite`.

         .. js:attribute:: waypoints

            Array: Indices to coordinates to treat as waypoints. If not supplied, all
            coordinates are waypoints.  Must include first and last coordinate index.

         .. js:attribute:: format

            String: Which output format to use, either `json`, or
            `flatbuffers <https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api/flatbuffers>`_.

         .. js:attribute:: snapping

            String: Which edges can be snapped to, either `default`, or `any`.  `default` only
            snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping
            to any edge in the routing graph.

         .. js:attribute:: skip_waypoints=false

            Boolean: Removes waypoints from the response. Waypoints are still calculated, but
            not serialized. Could be useful in case you are interested in some other part of
            response and do not want to transfer waste data.


   .. js:method:: nearest(options[, plugin_config], callback)

      Snaps a coordinate to the street network and returns the nearest *n* matches.

      Note: `coordinates` in the general options only supports a single
      `{longitude},{latitude}` entry.

      :param Object options: Object containing parameters for the nearest query.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: object containing `waypoints`.

         waypoints
            array of `Waypoint`_ objects sorted by distance to the input coordinate.
            Each object has an additional `distance` property, which is the distance in meters to the supplied input coordinate.

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
           coordinates: [[13.388860,52.517037]],
           number: 3,
           bearings: [[0,20]]
         };
         osrm.nearest(options, function(err, response) {
           console.log(response.waypoints); // array of Waypoint objects
         });

      .. js:class:: nearest_options

         .. js:attribute:: coordinates

            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         .. js:attribute:: bearings

            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         .. js:attribute:: radiuses

            Array: Limits the coordinate snapping to streets in the given radius in meters. Can be `null` (unlimited, default) or `double >= 0`.

         .. js:attribute:: hints

            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         .. js:attribute:: generate_hints=true

             Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         .. js:attribute:: number=1

            Number: Number of nearest segments that should be returned. Must be an integer greater than or equal to `1`.

         .. js:attribute:: approaches

            Array: Restrict the direction on the road network at a waypoint, relative to the input coordinate. Can be `null` (unrestricted, default), `curb` or `opposite`.

         .. js:attribute:: format

            String: Which output format to use, either `json`, or
            `flatbuffers <https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api/flatbuffers>`_.

         .. js:attribute:: snapping

            String: Which edges can be snapped to, either `default`, or `any`.
            `default` only snaps to edges marked by the profile as `is_startpoint`,
            `any` will allow snapping to any edge in the routing graph.


   .. js:method:: table(options[, plugin_config], callback)

      Computes duration table for the given locations. Allows for both symmetric and
      asymmetric tables.  Optionally returns distance table.

      :param Object options:       Object literal containing parameters for the table query.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: object containing `durations`, `distances`, `sources`, and `destinations`.

         durations
            array of arrays that stores the matrix in row-major order. `durations[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
            Values given in seconds.
         distances
            array of arrays that stores the matrix in row-major order. `distances[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
            Values given in meters.
         sources
            array of `Waypoint`_ objects describing all sources in order.
         destinations
            array of `Waypoint`_ objects describing all destinations in order.
         fallback_speed_cells (optional)
            if `fallback_speed` is used, will be an array of arrays of `row,column` values, indicating which cells contain estimated values.

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
           coordinates: [
             [13.388860,52.517037],
             [13.397634,52.529407],
             [13.428555,52.523219]
           ]
         };
         osrm.table(options, function(err, response) {
           console.log(response.durations); // array of arrays, matrix in row-major order
           console.log(response.distances); // array of arrays, matrix in row-major order
           console.log(response.sources); // array of Waypoint objects
           console.log(response.destinations); // array of Waypoint objects
         });

      .. js:class:: table_options

         .. js:attribute:: coordinates

            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         .. js:attribute:: bearings

            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         .. js:attribute:: radiuses

            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `null` (unlimited, default) or `double >= 0`.

         .. js:attribute:: hints

            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         .. js:attribute:: generate_hints=true

            Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         .. js:attribute:: sources

            Array: An array of `index` elements (`0 <= integer < #coordinates`) to use
            location with given index as source. Default is to use all.

         .. js:attribute:: destinations

            Array: An array of `index` elements (`0 <= integer < #coordinates`) to use
            location with given index as destination. Default is to use all.

         .. js:attribute:: approaches

            Array: Restrict the direction on the road network at a waypoint, relative to the input coordinate.
            Can be `null` (unrestricted, default), `curb` or `opposite`.

         .. js:attribute:: fallback_speed

            Number: Replace `null` responses in result with as-the-crow-flies estimates based on `fallback_speed`.
            Value is in metres/second.

         .. js:attribute:: fallback_coordinate

            String: Either `input` (default) or `snapped`.  If using a `fallback_speed`,
            use either the user-supplied coordinate (`input`), or the snapped coordinate (`snapped`)
            for calculating the as-the-crow-flies distance between two points.

         .. js:attribute:: scale_factor

            Number: Multiply the table duration values in the table by this number
            for more controlled input into a route optimization solver.

         .. js:attribute:: snapping

            String: Which edges can be snapped to, either `default`, or `any`.
            `default` only snaps to edges marked by the profile as `is_startpoint`,
            `any` will allow snapping to any edge in the routing graph.

         .. js:attribute:: annotations

            Array: Return the requested table or tables in response. Can be
            `['duration']` (return the duration matrix, default),
            `[distance']` (return the distance matrix), or
            `['duration', distance']` (return both the duration matrix and the distance matrix).


   .. js:method:: tile(coordinates[, plugin_config], callback)

      This generates `Mapbox Vector Tiles <https://mapbox.com/vector-tiles>`_ that can be
      viewed with a vector-tile capable slippy-map viewer. The tiles contain road
      geometries and metadata that can be used to examine the routing graph. The tiles are
      generated directly from the data in-memory, so are in sync with actual routing
      results, and let you examine which roads are actually routable, and what weights they
      have applied.

      :param Array coordinates:
             an array consisting of `x`, `y`, and `z` values representing tile coordinates like
             `wiki.openstreetmap.org/wiki/Slippy_map_tilenames <https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames>`_
             and are supported by vector tile viewers like `Mapbox GL JS <https://www.mapbox.com/mapbox-gl-js/api/>`_.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: buffer containing a Protocol-Buffer-encoded vector tile.

      .. code:: js

         const osrm = new OSRM('network.osrm');
         osrm.tile([0, 0, 0], function(err, response) {
           if (err) throw err;
           fs.writeFileSync('./tile.vector.pbf', response); // write the buffer to a file
         });




   .. js:method:: match(coordinates[, plugin_config], callback)

      Map matching matches given GPS points to the road network in the most plausible way.
      Please note the request might result multiple sub-traces. Large jumps in the
      timestamps (>60s) or improbable transitions lead to trace splits if a complete
      matching could not be found. The algorithm might not be able to match all points.
      Outliers are removed if they can not be matched successfully.

      :param Object options:       Object literal containing parameters for the match query.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: object containing `tracepoints` and `matchings`.

         tracepoints
            Array of `Waypoint`_ objects representing all points of the trace in order.
            If the trace point was omitted by map matching because it is an outlier, the entry will be null.
            Each `Waypoint` object has the following additional properties,

            1. `matchings_index`: Index to the `Route`_ object in matchings
               the sub-trace was matched to,
            2. `waypoint_index`: Index of the waypoint inside the matched route.
            3. `alternatives_count`: Number of probable alternative matchings for this trace point.
               A value of zero indicate that this point was matched unambiguously.
               Split the trace at these points for incremental map matching.

         matchings
            is an array of `Route`_ objects that assemble the trace. Each `Route` object has an additional `confidence` property,
            which is the confidence of the matching. float value between `0` and `1`. `1` is very confident that the matching is correct.

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
             coordinates: [[13.393252,52.542648],[13.39478,52.543079],[13.397389,52.542107]],
             timestamps: [1424684612, 1424684616, 1424684620]
         };
         osrm.match(options, function(err, response) {
             if (err) throw err;
             console.log(response.tracepoints); // array of Waypoint objects
             console.log(response.matchings); // array of Route objects
         });


      .. js:class:: match_options

         .. js:attribute:: coordinates

            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         .. js:attribute:: bearings

            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         .. js:attribute:: hints

            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         .. js:attribute:: generate_hints=true

            Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         .. js:attribute:: steps=false

            Boolean: Return route steps for each route.

         .. js:attribute:: annotations=false

            Array|Boolean: An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`,
            `speed` or boolean for enabling/disabling all.

         .. js:attribute:: geometries=polyline

            String: Returned route geometry format (influences overview and per step). Can also be `geojson`.

         .. js:attribute:: overview=simplified

            String: Add overview geometry either `full`, `simplified` according to highest zoom level
            it could be display on, or not at all (`false`).

         .. js:attribute:: timestamps

            Array<Number>: Timestamp of the input location (integers, UNIX-like timestamp).

         .. js:attribute:: radiuses

            Array: Standard deviation of GPS precision used for map matching. If applicable use GPS accuracy.
            Can be `null` for default value `5` meters or `double >= 0`.

         .. js:attribute:: gaps=split

            String: Allows the input track splitting based on huge timestamp gaps between points. Either `split` or `ignore`.

         .. js:attribute:: tidy=false

            Boolean: Allows the input track modification to obtain better matching quality for noisy tracks.

         .. js:attribute:: waypoints

            Array: Indices to coordinates to treat as waypoints.  If not supplied, all coordinates are waypoints.
            Must include first and last coordinate index.

         .. js:attribute:: snapping

            String: Which edges can be snapped to, either `default`, or `any`. `default` only snaps to edges
            marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.


   .. js:method:: trip(coordinates[, plugin_config], callback)

      The trip plugin solves the Traveling Salesman Problem using a greedy heuristic
      (farthest-insertion algorithm) for 10 or more waypoints and uses brute force for less
      than 10 waypoints. The returned path does not have to be the shortest path, as TSP is
      NP-hard it is only an approximation.

      Note that all input coordinates have to be connected for the trip service to work.
      Currently, not all combinations of `roundtrip`, `source` and `destination` are
      supported.  Right now, the following combinations are possible:

      ========= ====== =========== =========
      roundtrip source destination supported
      ========= ====== =========== =========
      true      first  last        **yes**
      true      first  any         **yes**
      true      any    last        **yes**
      true      any    any         **yes**
      false     first  last        **yes**
      false     first  any         no
      false     any    last        no
      false     any    any         no
      ========= ====== =========== =========

      :param Object options:       Object literal containing parameters for the trip query.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:

      :returns: an object containing:

         waypoints
            an array of `Waypoint`_ objects representing all waypoints in input order.
            Each Waypoint object has the following additional properties,

            1) `trips_index`: index to trips of the sub-trip the point was matched to, and
            2) `waypoint_index`: index of the point in the trip.

         trips
            an array of `Route`_ objects that assemble the trace.

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
           coordinates: [
             [13.36761474609375, 52.51663871100423],
             [13.374481201171875, 52.506191342034576]
           ],
           source: "first",
           destination: "last",
           roundtrip: false
         }
         osrm.trip(options, function(err, response) {
           if (err) throw err;
           console.log(response.waypoints); // array of Waypoint objects
           console.log(response.trips); // array of Route objects
         });


      .. js:class:: trip_options

         .. js:attribute:: coordinates

            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         .. js:attribute:: bearings

            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         .. js:attribute:: radiuses

            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `double >= 0` or `null` (unlimited, default).

         .. js:attribute:: hints

            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         .. js:attribute:: generate_hints=true

            Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         .. js:attribute:: steps=false

            Boolean: Return route steps for each route.

         .. js:attribute:: annotations=false

            Array|Boolean: An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`,
            `speed` or boolean for enabling/disabling all.

         .. js:attribute:: geometries=polyline

            String: Returned route geometry format (influences overview and per step). Can also be `geojson`.

         .. js:attribute:: overview=simplified

            String: Add overview geometry either `full`, `simplified`

         .. js:attribute:: roundtrip=true

            Boolean: Return route is a roundtrip.

         .. js:attribute:: source=any

            String: Return route starts at `any` or `first` coordinate.

         .. js:attribute:: destination=an

            String: Return route ends at `any` or `last` coordinate.

         .. js:attribute:: approache

            Array: Restrict the direction on the road network at a waypoint, relative to the input coordinate.
            Can be `null` (unrestricted, default), `curb` or `opposite`.

         .. js:attribute:: snapping

            String: Which edges can be snapped to, either `default`, or `any`.
            `default` only snaps to edges marked by the profile as `is_startpoint`,
            `any` will allow snapping to any edge in the routing graph.


.. js:class:: plugin_config

   All plugins support a second additional parameter to configure some NodeJS specific
   behaviours.

   .. js:attribute:: format

      String: The format of the result object to various API calls. Valid options are `object`
      (default if `options.format` is `json`), which returns a standard Javascript object,
      as described above, and `buffer` (default if `options.format` is `flatbuffers`),
      which will return a NodeJS `Buffer <https://nodejs.org/api/buffer.html>`_ object,
      containing a JSON string or Flatbuffers object.
      The latter has the advantage that it can be immediately serialized to disk/sent over the
      network, and the generation of the string is performed outside the main NodeJS event loop.
      This option is ignored by the `tile` plugin. Also note that `options.format` set to
      `flatbuffers` cannot be used with `plugin_config.format` set to `object`. `json_buffer` is
      deprecated alias for `buffer`.

.. code:: js

   const osrm = new OSRM('network.osrm');
   const options = {
     coordinates: [
       [13.36761474609375, 52.51663871100423],
       [13.374481201171875, 52.506191342034576]
     ]
   };
   osrm.route(options, { format: "buffer" }, function(err, response) {
     if (err) throw err;
     console.log(response.toString("utf-8"));
   });

Responses
---------

Route
~~~~~

Represents a route through (potentially multiple) waypoints.

`osrm-backend <http.html#route-object>`_

RouteLeg
~~~~~~~~

Represents a route between two waypoints.

`osrm-backend <http.html#routeleg-object>`_

RouteStep
~~~~~~~~~

A step consists of a maneuver such as a turn or merge, followed by a distance of travel along a
single way to the subsequent step.

`osrm-backend <http.html#routestep-object>`_

StepManeuver
~~~~~~~~~~~~

`osrm-backend <http.html#stepmaneuver-object>`_

Waypoint
~~~~~~~~

Represents a waypoint on a route.

`osrm-backend <http.html#waypoint-object>`_



