.. _trip:

Trip Service
============

.. http:get:: /trip/v1/(profile)/(coordinates)

   The trip plugin solves the Traveling Salesman Problem using a greedy heuristic
   (farthest-insertion algorithm) for 10 or more waypoints and uses brute force for less
   than 10 waypoints.  The returned path does not have to be the fastest one. As TSP is
   NP-hard it only returns an approximation.  Note that all input coordinates have to be
   connected for the trip service to work.

   .. dropdown:: Example Requests

      .. code:: bash

         # Round trip in Berlin with three stops:
         curl 'http://router.project-osrm.org/trip/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219'

         # Round trip in Berlin with four stops, starting at the first stop, ending at the last:
         curl 'http://router.project-osrm.org/trip/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219;13.418555,52.523215?source=first&destination=last'

   This service accepts following parameters in addition to the :ref:`common parameters <common_options>`.

   :query boolean roundtrip:
      The sought route is a roundtrip, it returns to the first location.
      :default:`true`, `false`
   :query keyword source:
      The returned route must start at the `first` or can start at `any` coordinate.
      :default:`any`, `first`
   :query keyword destination:
      The returned route must end at the `last` or can end at `any` coordinate
      :default:`any`, `last`
   :query boolean steps:
      Return route instructions for each trip. :default:`false`, `true`
   :query keyword annotations:
      Return additional metadata for each coordinate along the route geometry.
      :default:`false`, `true`, `nodes`, `distance`, `duration`, `datasources`,
      `weight`, `speed`
   :query keyword geometries:
      Returned route geometry format (influences overview and per step).
      :default:`polyline`, `polyline6`, `geojson`
   :query keyword overview:
      Add overview geometry: either full, simplified according to highest zoom level it
      could be displayed on, or none at all. :default:`simplified`, `full`, `false`


   **Fixing Start and End Points**

   It is possible to explicitly set the start or end coordinates of the trip.  If the
   source is set to `first`, the first coordinate is used as the start coordinate of the
   trip. If the destination is set to `last`, the last coordinate will be used as the
   destination of the trip. If you specify `any`, any of the coordinates can be used as
   the first or last coordinate in the output.

   However, if `source=any&destination=any&roundtrip=true` the returned round-trip will
   still start at the first input coordinate by default.

   Currently, not all combinations of `roundtrip`, `source`, and `destination` are
   supported.  Right now, the following combinations are possible:

   ========= ====== =========== =========
   roundtrip source destination supported
   ========= ====== =========== =========
   true      first  last        **yes**
   true      first  any         **yes**
   true      any    last        **yes**
   true      any    any         **yes**
   false     first  last        **yes**
   false     first  any         **yes**
   false     any    last        **yes**
   false     any    any         no
   ========= ====== =========== =========

   :>json string code: `NoTrips` if no trips were found because input coordinates are
      not connected. `NotImplemented` if this service is not supported on this server.

   :>json array waypoints:
      Array of `Waypoint` objects representing all waypoints in input order. Each
      `Waypoint` object has the following additional properties:

        - `trips_index`: Index to `trips` of the sub-trip the point was matched to.
        - `waypoint_index`: Index of the point in the trip.

   :>json array trips:
      Array of `Route` objects that assemble the trace.
