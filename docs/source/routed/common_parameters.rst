:param string profile:
   Mode of transportation. This is the filename of the Lua profile given to
   `osrm-extract`.  Typical values are: `car`, `bike` or `foot`.

:param coordinates: `array<coordinate>`, `polyline({polyline})`, or
   `polyline6({polyline6})`. A `coordinate` is a pair of `{longitude},{latitude}`.  A
   `polyline` follows Google's polyline format with precision 5.  A `polyline6` follows
   Google's polyline format with precision 6.  Polylines can be generated using `this
   package <https://www.npmjs.com/package/@mapbox/polyline>`_.

:param keyword format: :default:`json` or `flatbuffers`. This parameter is optional.
