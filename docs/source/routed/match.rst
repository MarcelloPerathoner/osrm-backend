.. _match:

match
=====

.. http:get:: /match/v1/(profile)/(coordinates)

   Map matching matches/snaps given GPS points to the road network in the most plausible
   way.  Please note the request might result in multiple sub-traces. Large jumps in the
   timestamps (> 60s) or improbable transitions lead to trace splits if a complete
   matching could not be found.  The algorithm might not be able to match all points.
   Outliers are removed if they can not be matched successfully.

   This service accepst following parameters in addition to the :ref:`common parameters <common_options>`.

   :param boolean steps: Return route steps for each route. :default:`false`, `true`

   :param keyword geometries: Returned route geometry format (influences overview and
      per step). :default:`polyline`, `polyline6`, `geojson`

   :param keyword annotations: Returns additional metadata for each coordinate along the
      route geometry. :default:`false`, `true`, `nodes`, `distance`, `duration`,
      `datasources`, `weight`, `speed`

   :param keyword overview: Add overview geometry either full, simplified according to
      highest zoom level it could be displayed on, or not at all. :default:`simplified`,
      `full`, `false`

   :param array timestamps: Timestamps for the input locations in seconds since UNIX
      epoch. Timestamps need to be monotonically increasing.
      `{timestamp};{timestamp}[;{timestamp} ...]` Timestamp is an integer representing
      the number of seconds elapsed since the UNIX epoch.

   :param array radiuses: Standard deviation of GPS precision used for map matching. If
      applicable use GPS accuracy. `{radius};{radius}[;{radius} ...]` Radius is a
      positive floating point number in meters (default 5m).

   :param keyword gaps: Allows the input track splitting based on huge timestamp gaps
      between points.  :default: `split`, `ignore`

   :param keyword tidy: Allows the input track modification to obtain better matching quality for noisy tracks.
      :default:`false`, `true`

   :param array waypoints: The input coordinates at the given indices are treated as
      waypoints in the returned `Match` object. Default is to treat all input
      coordinates as waypoints. `{index};{index};{index}...`

   Note: The radius for each point should be the standard error of the location measured
   in meters from the true location.  Use `Location.getAccuracy()` on Android or
   `CLLocation.horizontalAccuracy` on iOS.  This value is used to determine which points
   should be considered as candidates (a larger radius means more candidates) and how
   likely each candidate is (a larger radius means far-away candidates are penalized
   less).  The area to search is chosen such that the correct candidate should be
   considered 99.9% of the time (for more details see :pr:`3184`).

   :>json string code: `NoMatch` if no matchings were found.
   :>json array tracepoints:
      Array of `Waypoint` objects representing all points of the trace in order.  If the
      tracepoint was omitted by map matching because it is an outlier, the entry will be
      `null`.  Each `Waypoint` object has the following additional properties:

      - `matchings_index`: Index to the `Route` object in `matchings` the sub-trace was
        matched to.
      - `waypoint_index`: Index of the waypoint inside the matched route.
      - `alternatives_count`: Number of probable alternative matchings for this
        tracepoint. A value of zero indicates that this point was matched unambiguously.
        Split the trace at these points for incremental map matching.

   :>json array matchings:

      - `matchings`: An array of `Route` objects that assemble the trace. Each `Route`
        object has the following additional properties:
      - `confidence`: Confidence of the matching. `float` value between 0 and 1. 1 is
        very confident that the matching is correct.
