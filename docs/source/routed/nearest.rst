.. _nearest:

Nearest Service
===============

.. http:get:: /nearest/v1/(profile)/(coordinate)

   Snaps a given coordinate to the street network and returns the `n` nearest matches.

   .. dropdown:: Example Request

      .. code:: bash

         curl 'http://router.project-osrm.org/nearest/v1/driving/13.388860,52.517037?number=3&bearings=0,20'

   .. dropdown:: Example Response

      .. code:: json

         {
            "waypoints" : [
               {
                  "nodes": [
                     2264199819,
                     0
                  ],
                  "hint" : "KSoKADRYroqUBAEAEAAAABkAAAAGAAAAAAAAABhnCQCLtwAA_0vMAKlYIQM8TMwArVghAwEAAQH1a66g",
                  "distance" : 4.152629,
                  "name" : "Friedrichstraße",
                  "location" : [
                     13.388799,
                     52.517033
                  ]
               },
               {
                  "nodes": [
                     2045820592,
                     0
                  ],
                  "hint" : "KSoKADRYroqUBAEABgAAAAAAAAAAAAAAKQAAABhnCQCLtwAA7kvMAAxZIQM8TMwArVghAwAAAQH1a66g",
                  "distance" : 11.811961,
                  "name" : "Friedrichstraße",
                  "location" : [
                     13.388782,
                     52.517132
                  ]
               },
               {
                  "nodes": [
                     0,
                     21487242
                  ],
                  "hint" : "KioKgDbbDgCUBAEAAAAAABoAAAAAAAAAPAAAABlnCQCLtwAA50vMADJZIQM8TMwArVghAwAAAQH1a66g",
                  "distance" : 15.872438,
                  "name" : "Friedrichstraße",
                  "location" : [
                     13.388775,
                     52.51717
                  ],
               }
            ],
            "code" : "Ok"
         }

   This service accepts the following parameters in addition to the :ref:`common parameters <common_options>`.
   Note that this service accepts only one coordinate.

   :query integer number: How many nearest segments should be returned. Integer > 0, default: 1.
   :query boolean skip_waypoints:  As `waypoints` contain the only data returned by this
      service, setting this option to `true` makes no sense, but is still possible. In
      that case, only the `code` field will be returned.

   :>json string code: `Ok` if the request was successful. See also the general status codes.
   :>json array waypoints: array of `Waypoint` objects sorted by distance to the input coordinate.
      Each object has at least the following additional properties:

      - `nodes`: Array of OpenStreetMap node ids.
