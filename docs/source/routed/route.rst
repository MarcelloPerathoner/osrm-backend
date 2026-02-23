.. _route:

route
=====

.. http:get:: /route/v1/(profile)/(coordinates)

   Finds the fastest route between the coordinates in the supplied order.

   **Example Request**

   .. code:: http

      GET /route/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?overview=false HTTP/1.1
      Host: router.project-osrm.org

   .. dropdown:: **Example Response**

      .. code:: json

         {
            "code": "Ok",
            "routes": [
               {
               "legs": [
                  {
                     "steps": [],
                     "weight": 263.8,
                     "summary": "",
                     "duration": 260.3,
                     "distance": 1888
                  },
                  {
                     "steps": [],
                     "weight": 370.9,
                     "summary": "",
                     "duration": 370.9,
                     "distance": 2845.2
                  }
               ],
               "weight_name": "routability",
               "weight": 634.7,
               "duration": 631.2,
               "distance": 4733.2
               }
            ],
            "waypoints": [
               {
               "hint": "TQEKgK5inoUXAAAABQAAAAAAAAAdAAAAuFtKQYXNK0AAAAAAIMuBQQsAAAADAAAAAAAAAA8AAADHHQEAFEzMAKpYIQM8TMwArVghAwAA3woAAAAA",
               "location": [
                  13.38882,
                  52.517034
               ],
               "name": "Friedrichstraße",
               "distance": 2.73531597
               },
               {
               "hint": "XfbegWobkIcGAAAACQAAAAAAAABeAAAAz-ONQIv-tkAAAAAAeYZHQgYAAAAJAAAAAAAAAF4AAADHHQEAfm7MABmJIQOCbswA_4ghAwAATwUAAAAA",
               "location": [
                  13.39763,
                  52.529433
               ],
               "name": "Torstraße",
               "distance": 2.905919634
               },
               {
               "hint": "Pi0YgP___38fAAAAUQAAACYAAABSAAAAeosKQlNOX0Ki6yZCakq4Qh8AAABRAAAAJgAAAFIAAADHHQEASufMAOdwIQNL58wA03AhAwgAvxAAAAAA",
               "location": [
                  13.428554,
                  52.523239
               ],
               "name": "Platz der Vereinten Nationen",
               "distance": 2.226580806
               }
            ]
         }

   This service accepst following parameters in addition to the :ref:`common parameters <common_options>`.

   :query boolean|number alternatives: `true`, `false` (default), or Number.  Search
      for alternative routes. Passing a number `alternatives=n` searches for up to `n`
      alternative routes.  ** Please note that even if alternative routes are
      requested, a result cannot be guaranteed.**
   :query boolean steps: `true`, `false` (default). Whether to return route
      steps for each route leg.
   :query boolean|string annotations: `true`, `false` (default), `nodes`, `distance`, `duration`,
      `datasources`, `weight`, `speed`. Returns additional metadata for each
      coordinate along the route geometry.
   :query string geometries:  `polyline` (default), `polyline6`, `geojson`. Return route
      geometry format (influences overview and per step)
   :query string overview:  `simplified` (default), `full`, `false`.  Add overview geometry
      either full, simplified according to highest zoom level it could be displayed
      on, or not at all.
   :query boolean|string continue_straight: `default` (default), `true`, `false`.  Forces the route to
      keep going straight at waypoints constraining uturns there even if it would be
      faster. Default value depends on the profile.
   :query array<number> waypoints: `{index};{index};{index}...` Treats input coordinates indicated by
      given indices as waypoints in returned Match object. Default is to treat all
      input coordinates as waypoints.

   :>json string code: `NoRoute` if no route was found.
   :>json array waypoints: array of `Waypoint` objects representing all waypoints in order.
   :>json array routes: array of `Route` objects, ordered by descending recommendation rank.
