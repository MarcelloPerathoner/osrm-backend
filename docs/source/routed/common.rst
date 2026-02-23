.. _common:

Common Parameters
=================

.. http:get:: /(service)/(version)/(profile)/(coordinates)[.(format)]

   Common request structure

   **Example Requests**

   .. code:: http

      GET /route/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?overview=false HTTP/1.1
      Host: router.project-osrm.org

   .. code:: http

      GET /route/v1/driving/13.388860,52.517037;13.397634,52.529407?exclude=motorway HTTP/1.1
      Host: router.project-osrm.org

   .. code:: http

      GET /route/v1/driving/polyline(ofp_Ik_vpAilAyu@te@g`E)?overview=false HTTP/1.1
      Host: router.project-osrm.org

   .. _common_options:

   URL parameters common to all services:

   :param string service:
      One of the following values: :ref:`route`, :ref:`nearest`, :ref:`table`, :ref:`match`, :ref:`trip`, or :ref:`tile`

   :param string version:
      Version of the protocol implemented by the service. `v1` for all OSRM 5.x
      installations

   :param string profile:
      Mode of transportation. This is the filename of the Lua profile given to
      `osrm-extract`.  Typical values are: `car`, `bike` or `foot`.

   :param array<coordinate> coordinates:
      A semicolon-separated list of coordinates or a `polyline({polyline})` or a
      `polyline6({polyline6})`. If coordinates are given, each coordinate must have the
      format `{longitude},{latitude}`. `polyline` follows Google's polyline format with
      precision 5 by default and can be generated using
      `this package <https://www.npmjs.com/package/@mapbox/polyline>`.

   Query parameters common to all services:

   Note that the lists given to the parameters: `bearings`, `radiuses`, `hints`, and
   `approaches` must contain the same number of elements as there are coordinates.  An
   empty element means the default value, eg: `{element};;{element}`

   :query array<int,int> bearings: Limits the search to segments with given bearing in
      degrees towards true north in a clockwise direction.  The parameter is given as a
      semicolon-separated list of `{value}, {range}` pairs, where `value` is an integer in
      the range 0..360 and `range` is an integer in the range 0..180.
   :query array<float> radiuses: Limits the search to a given radius in meters. The
      parameter is given as a semicolon-separated list of positive floating point numbers
      or the keyword :default:`unlimited`.
   :query array<string> hints: The hints as obtained from a previous request.  The
      parameter is given as a semicolon-separated list of base64 strings.
   :query array<keyword> approaches: Restrict the direction on the road network at a waypoint,
      relative to the input coordinate. The parameter is given as a semicolon-separated list
      of keywords: `curb`, `opposite` or :default:`unrestricted`
   :query bool generate_hints: Wheter to include hints in the response, which can be
      used in subsequent requests, see the `hints` parameter. :default:`true` or `false`.
   :query array<String> exclude: A semicolon-separated list of road classes to avoid. Each element
      is a class name determined by the Lua profile or `none`.
   :query keyword snapping: Default snapping avoids `is_startpoint`-edges (see profile),
      `any` will snap to any edge in the graph. :default:`default`, or `any`.
   :query bool skip_waypoints: Removes `waypoints` from the response. Waypoints are
      still calculated, but not serialized. Reduces data transfer volume.

   Responses common to all services:

   :>json string code: Codes common to all services include:

      ================= ================================================================================
      Code              Description
      ================= ================================================================================
      `Ok`              The request completed with success.
      `InvalidUrl`      URL string is invalid.
      `InvalidService`  Service name is invalid.
      `InvalidVersion`  Version is not found.
      `InvalidOptions`  Options are invalid.
      `InvalidQuery`    The query string is syntactically malformed.
      `InvalidValue`    The successfully parsed query parameters are invalid.
      `NoSegment`       One of the supplied input coordinates could not snap to the street segment.
      `TooBig`          The request size violates one of the service-specific request size restrictions.
      `DisabledDataset` The request tried to access a disabled dataset.
      ================= ================================================================================

   :>json string message: is an **optional** human-readable error message. All other status
      types are service-dependent.
   :>json string data_version: contains a timestamp from the original OpenStreetMap file.
      This field will only be present if the `data_version` argument was given to
      `osrm-extract` and the OSM file did have an `osmosis_replication_timestamp` section.

   :http:statuscode: 200 and `code` will be `Ok`
   :http:statuscode: 400 in case of error

   Example of common response codes:

   .. code:: json

      {
      "code": "Ok",
      "message": "Everything worked",
      "data_version": "2017-11-17T21:43:02Z"
      }
