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
