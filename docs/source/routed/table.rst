.. _table:

table
=====

.. http:get:: /table/v1/(profile)/(coordinates)

   Computes the duration of the fastest route between all pairs of supplied coordinates.
   Returns durations or distances or both between the coordinate pairs. Note that the
   distances are not the shortest distance between two coordinates, but rather the
   distances of the fastest routes. Durations are in seconds and distances are in
   meters.

   .. code:: http

      GET /table/v1/(profile)/(coordinates) HTTP/1.1
      Host: router.project-osrm.org

   .. dropdown:: Example Requests

      .. code:: bash

         # Returns a 3x3 duration matrix:
         curl 'http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219'

         # Returns a 1x3 duration matrix
         curl 'http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?sources=0'

         # Returns a asymmetric 3x2 duration matrix with from the polyline encoded locations `qikdcB}~dpXkkHz`:
         curl 'http://router.project-osrm.org/table/v1/driving/polyline(egs_Iq_aqAppHzbHulFzeMe`EuvKpnCglA)?sources=0;1;3&destinations=2;4'

         # Returns a 3x3 duration matrix:
         curl 'http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?annotations=duration'

         # Returns a 3x3 distance matrix for CH:
         curl 'http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?annotations=distance'

         # Returns a 3x3 duration matrix and a 3x3 distance matrix for CH:
         curl 'http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?annotations=distance,duration'

   .. dropdown:: Example Response

      .. code:: json

         {
         "sources": [
            {
               "location": [
               13.3888,
               52.517033
               ],
               "hint": "PAMAgEVJAoAUAAAAIAAAAAcAAAAAAAAArss0Qa7LNEHiVIRA4lSEQAoAAAAQAAAABAAAAAAAAADMAAAAAEzMAKlYIQM8TMwArVghAwEA3wps52D3",
               "name": "Friedrichstraße"
            },
            {
               "location": [
               13.397631,
               52.529432
               ],
               "hint": "WIQBgL6mAoAEAAAABgAAAAAAAAA7AAAAhU6PQHvHj0IAAAAAQbyYQgQAAAAGAAAAAAAAADsAAADMAAAAf27MABiJIQOCbswA_4ghAwAAXwVs52D3",
               "name": "Torstraße"
            },
            {
               "location": [
               13.428554,
               52.523239
               ],
               "hint": "7UcAgP___38fAAAAUQAAACYAAABTAAAAhSQKQrXq5kKRbiZCWJo_Qx8AAABRAAAAJgAAAFMAAADMAAAASufMAOdwIQNL58wA03AhAwMAvxBs52D3",
               "name": "Platz der Vereinten Nationen"
            }
         ],
         "durations": [
            [
               0,
               192.6,
               382.8
            ],
            [
               199,
               0,
               283.9
            ],
            [
               344.7,
               222.3,
               0
            ]
         ],
         "destinations": [
            {
               "location": [
               13.3888,
               52.517033
               ],
               "hint": "PAMAgEVJAoAUAAAAIAAAAAcAAAAAAAAArss0Qa7LNEHiVIRA4lSEQAoAAAAQAAAABAAAAAAAAADMAAAAAEzMAKlYIQM8TMwArVghAwEA3wps52D3",
               "name": "Friedrichstraße"
            },
            {
               "location": [
               13.397631,
               52.529432
               ],
               "hint": "WIQBgL6mAoAEAAAABgAAAAAAAAA7AAAAhU6PQHvHj0IAAAAAQbyYQgQAAAAGAAAAAAAAADsAAADMAAAAf27MABiJIQOCbswA_4ghAwAAXwVs52D3",
               "name": "Torstraße"
            },
            {
               "location": [
               13.428554,
               52.523239
               ],
               "hint": "7UcAgP___38fAAAAUQAAACYAAABTAAAAhSQKQrXq5kKRbiZCWJo_Qx8AAABRAAAAJgAAAFMAAADMAAAASufMAOdwIQNL58wA03AhAwMAvxBs52D3",
               "name": "Platz der Vereinten Nationen"
            }
         ],
         "code": "Ok",
         "distances": [
            [
               0,
               1886.89,
               3791.3
            ],
            [
               1824,
               0,
               2838.09
            ],
            [
               3275.36,
               2361.73,
               0
            ]
         ],
         "fallback_speed_cells": [
            [ 0, 1 ],
            [ 1, 0 ]
         ]
         }

   This service accepst following parameters in addition to the :ref:`common parameters <common_options>`.

   :param array sources: Use location with given index as source. :default:`all` or
      `{index};{index}[;{index} ...]`

   :param array destinations: Use location with given index as destination.
      :default:`all` or `{index};{index}[;{index} ...]` Index is an integer `0 <=
      integer < #locations`

   :param keyword annotations: Return the requested table or tables in response.
      :default:`duration`, `distance`, or `duration,distance`

   :param number fallback_speed: If no route found between a source/destination pair,
      calculate the as-the-crow-flies distance, then use this speed to estimate duration.
      `double > 0`

   :param keyword fallback_coordinate: When using a `fallback_speed`, use the
      user-supplied coordinate (`input`), or the snapped location (`snapped`) for
      calculating distances. :default:`input`, or `snapped`

   :param number scale_factor: Use in conjunction with `annotations=durations`. Scales
      the table `duration` values by this number. `double > 0`

   Unlike other array encoded options, the length of `sources` and `destinations` can be
   **smaller than or equal** the number of input locations.
   :code:`sources=0;5;7&destinations=5;1;4;2;3;6`

   With `skip_waypoints` set to `true`, both `sources` and `destinations` arrays will be skipped.

   :>json string code: `NoTable` if no route was found. `NotImplemented` if this request
    is not supported.

   :>json durations:
      array of arrays that stores the matrix in row-major order. `durations[i][j]` gives
      the travel time from the i-th source to the j-th destination. Values are given in
      seconds. Can be `null` if no route between `i` and `j` can be found.

   :>json distances:
      array of arrays that stores the matrix in row-major order. `distances[i][j]` gives
      the travel distance from the i-th source to the j-th destination. Values are given
      in meters. Can be `null` if no route between `i` and `j` can be found.

   :>json sources:
      array of `Waypoint` objects describing all sources in order

   :>json destinations:
      array of `Waypoint` objects describing all destinations in order

   :>json fallback_speed_cells:
      (optional) array of arrays containing `i,j` pairs indicating which cells contain
      estimated values based on `fallback_speed`.  Will be absent if `fallback_speed` is
      not used.
