.. _match:

Match Service
=============

.. http:get:: /match/v1/(profile)/(coordinates)[.(format)]

   Map matching matches/snaps given GPS points to the road network in the most plausible
   way.  Please note the request might result in multiple sub-traces. Large jumps in the
   timestamps (> 60s) or improbable transitions lead to trace splits if a complete
   matching could not be found.  The algorithm might not be able to match all points.
   Outliers are removed if they can not be matched successfully.

   .. include:: common_parameters.rst

   .. include:: common_queries.rst

   :query boolean steps: :default:`false`, `true`. Return route steps for each route.

   :query keyword geometries: :default:`polyline`, `polyline6`, `geojson`. Which
      route geometry format to return (influences overview and per step).

   :query keyword annotations: :default:`false`, `true`, `nodes`, `distance`,
      `duration`, `datasources`, `weight`, `speed`. Return additional metadata for each
      coordinate along the route geometry.

   :query keyword overview: :default:`simplified`, `full`, `false`. Add an overview
      geometry either: full, simplified according to highest zoom level it could be
      displayed on, or none at all.

   :query array<integer> timestamps: `{timestamp};{timestamp};{timestamp}...` Timestamp
      for each input location. Timestamps must be monotonically increasing.  Each
      timestamp is an integer representing the number of seconds elapsed since the UNIX
      epoch.

   :query array<float> radiuses: `{radius};{radius};{radius}...` Standard deviation of GPS
      precision used for map matching. If applicable use GPS accuracy. Radius is a
      positive floating point number in meters (default 5m).  Note: The radius for each
      point should be the standard error of the location measured in meters from the
      true location.  Use `Location.getAccuracy()` on Android or
      `CLLocation.horizontalAccuracy` on iOS.  This value is used to determine which
      points should be considered as candidates (a larger radius means more candidates)
      and how likely each candidate is (a larger radius means far-away candidates are
      penalized less).  The area to search is chosen such that the correct candidate
      should be considered 99.9% of the time (for more details see :pr:`3184`).

   :query keyword gaps: :default:`split`, `ignore`. Split the input track on large
    timestamp gaps between points.

   :query keyword tidy: :default:`false`, `true`. Allows modifications to the input
    track to obtain better matching quality for noisy tracks.

   :query array<integer> waypoints: `{index};{index};{index}...` The input coordinates
      at the given indices are treated as waypoints. Default is to treat all input
      coordinates as waypoints.

   .. include:: common_responses.rst

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
      Array of `Route` objects that assemble the trace. Each `Route` object has the
      following additional properties:

      - `confidence`: Confidence of the matching. `float` value between 0 and 1. 1 is
        very confident that the matching is correct.

   .. include:: common_statuscodes.rst
