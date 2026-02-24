:query array<bearing> bearings: Limits the search to segments with given bearing in
   degrees towards true north in a clockwise direction.  A bearing is a pair of
   `{value},{range}`, where `value` is an integer in the range 0..360 and `range` is an
   integer in the range 0..180.  The array must contain as many elements as there are
   coordinates.

:query array<radius> radiuses: Limits the search to a given radius in meters. A radius
   is a positive floating point number or the keyword `unlimited`. The array must
   contain as many elements as there are coordinates.

:query array<approach> approaches: An approach is either :default:`unrestricted`,
   `curb`, or `opposite`.  Restrict the direction on the road network at a waypoint,
   relative to the input coordinate. The array must contain as many elements as there
   are coordinates.

:query array<hint> hints: A hint is an opaque base64-string obtained from a previous
   request. The array must contain as many elements as there are coordinates.

:query bool generate_hints: :default:`true` or `false`. Whether to add hints to the
   response, which can be used in subsequent requests. See the `hints` parameter.

:query exclude: A semicolon-separated list of road classes to avoid. Each element is a
   class name as declared in the Lua profile or `none`.

:query keyword snapping: :default:`default` or `any`. Default snapping avoids
   `is_startpoint`-edges (see Lua profile), `any` will snap to any edge in the graph.

:query bool skip_waypoints: :default:`false` or `true`. Removes `waypoints` from the
   response. Waypoints are still calculated, but not serialized. Reduces the data
   transfer volume.
