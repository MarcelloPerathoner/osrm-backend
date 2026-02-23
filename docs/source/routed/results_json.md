## Result Objects in JSON Format

(route-object)=
### Route object

Represents a route through (potentially multiple) waypoints.

**Properties**

- `distance`: The distance traveled by the route, in `float` meters.
- `duration`: The estimated travel time, in `float` number of seconds.
- `geometry`: The whole geometry of the route value depending on `overview` parameter, format depending on the `geometries` parameter. See `RouteStep`'s `geometry` property for the parameter documentation.
- `weight`: The calculated weight of the route.
- `weight_name`: The name of the weight profile used during the extraction phase.

| overview   | Description                                                                                   |
| ---------- | --------------------------------------------------------------------------------------------- |
| simplified | Geometry is simplified according to the highest zoom level it can still be displayed in full. |
| full       | Geometry is not simplified.                                                                   |
| false      | Geometry is not added.                                                                        |

- `legs`: The legs between the given waypoints, an array of `RouteLeg` objects.

#### Example

Three input coordinates, `geometry=geojson`, `steps=false`:

```json
{
  "distance": 90.0,
  "duration": 300.0,
  "weight": 300.0,
  "weight_name": "duration",
  "geometry": {"type": "LineString", "coordinates": [[120.0, 10.0], [120.1, 10.0], [120.2, 10.0], [120.3, 10.0]]},
  "legs": [
    {
      "distance": 30.0,
      "duration": 100.0,
      "steps": []
    },
    {
      "distance": 60.0,
      "duration": 200.0,
      "steps": []
    }
  ]
}
```

### RouteLeg object

Represents a route between two waypoints.

**Properties**

- `distance`: The distance traveled by this route leg, in `float` meters.
- `duration`: The estimated travel time, in `float` number of seconds.
- `weight`: The calculated weight of the route leg.
- `summary`: Summary of the route taken as `string`. Depends on the `steps` parameter:

| steps |                                                                            |
| ----- | -------------------------------------------------------------------------- |
| true  | Names of the two major roads used. Can be empty if the route is too short. |
| false | empty `string`                                                             |

- `steps`: Depends on the `steps` parameter.

| steps |                                                                       |
| ----- | --------------------------------------------------------------------- |
| true  | array of `RouteStep` objects describing the turn-by-turn instructions |
| false | empty array                                                           |

- `annotation`: Additional details about each coordinate along with the route geometry:

| annotations |                                                                                |
| ----------- | ------------------------------------------------------------------------------ |
| true        | An `Annotation` object containing node ids, durations, distances, and weights. |
| false       | `undefined`                                                                    |

#### Example

With `steps=false` and `annotations=true`:

```json
{
  "distance": 30.0,
  "duration": 100.0,
  "weight": 100.0,
  "steps": [],
  "annotation": {
    "distance": [5,5,10,5,5],
    "duration": [15,15,40,15,15],
    "datasources": [1,0,0,0,1],
    "metadata": { "datasource_names": ["traffic","lua profile","lua profile","lua profile","traffic"] },
    "nodes": [49772551,49772552,49786799,49786800,49786801,49786802],
    "speed": [0.3, 0.3, 0.3, 0.3, 0.3]
  }
}
```

### Annotation object

Annotation of the whole route leg with fine-grained information about each segment or node id.

**Properties**

- `distance`: The distance, in meters, between each pair of coordinates
- `duration`: The duration between each pair of coordinates, in seconds.  Does not include the duration of any turns.
- `datasources`: The index of the data source for the speed between each pair of coordinates. `0` is the default profile, other values are supplied via `--segment-speed-file` to `osrm-contract` or `osrm-customize`.  String-like names are in the `metadata.datasource_names` array.
- `nodes`: The OSM node ID for each coordinate along the route, excluding the first/last user-supplied coordinates
- `weight`: The weights between each pair of coordinates.  Does not include any turn costs.
- `speed`: Convenience field, calculation of `distance / duration` rounded to one decimal place
- `metadata`: Metadata related to other annotations
  - `datasource_names`: The names of the data sources used for the speed between each pair of coordinates.  `lua profile` is the default profile, other values are the filenames supplied via `--segment-speed-file` to `osrm-contract` or `osrm-customize`

#### Example

```json
{
  "distance": [5,5,10,5,5],
  "duration": [15,15,40,15,15],
  "datasources": [1,0,0,0,1],
  "metadata": { "datasource_names": ["traffic","lua profile","lua profile","lua profile","traffic"] },
  "nodes": [49772551,49772552,49786799,49786800,49786801,49786802],
  "weight": [15,15,40,15,15]
}
```


### RouteStep object

A step consists of a maneuver such as a turn or merge, followed
by a distance of travel along a single way to the subsequent
step.

**Properties**

- `distance`: The distance of travel from the maneuver to the subsequent step, in `float` meters.
- `duration`: The estimated travel time, in `float` number of seconds.
- `geometry`: The unsimplified geometry of the route segment, depending on the `geometries` parameter.
- `weight`: The calculated weight of the step.

| `geometry` |                                                                                                      |
| ---------- | ---------------------------------------------------------------------------------------------------- |
| polyline   | [polyline](https://www.npmjs.com/package/polyline) with precision 5 in [latitude,longitude] encoding |
| polyline6  | [polyline](https://www.npmjs.com/package/polyline) with precision 6 in [latitude,longitude] encoding |
| geojson    | [GeoJSON `LineString`](http://geojson.org/geojson-spec.html#linestring)                              |

- `name`: The name of the way along which travel proceeds.
- `ref`: A reference number or code for the way. Optionally included, if ref data is available for the given way.
- `pronunciation`: A string containing an [IPA](https://en.wikipedia.org/wiki/International_Phonetic_Alphabet) phonetic transcription indicating how to pronounce the name in the `name` property. This property is omitted if pronunciation data is unavailable for the step.
- `destinations`: The destinations of the way. Will be `undefined` if there are no destinations.
- `exits`: The exit numbers or names of the way. Will be `undefined` if there are no exit numbers or names.
- `mode`: A string signifying the mode of transportation.
- `maneuver`: A `StepManeuver` object representing the maneuver.
- `intersections`: A list of `Intersection` objects that are passed along the segment, the very first belonging to the StepManeuver
- `rotary_name`: The name for the rotary. Optionally included, if the step is a rotary and a rotary name is available.
- `rotary_pronunciation`: The pronunciation hint of the rotary name. Optionally included, if the step is a rotary and a rotary pronunciation is available.
- `driving_side`: The legal driving side at the location for this step.  Either `left` or `right`.

#### Example

```json
{
   "geometry" : "{lu_IypwpAVrAvAdI",
   "mode" : "driving",
   "duration" : 15.6,
   "weight" : 15.6,
   "intersections" : [
      {  "bearings" : [ 10, 92, 184, 270 ],
         "lanes" : [
            { "indications" : [ "left", "straight" ],
               "valid" : false },
            { "valid" : true,
               "indications" : [ "right" ] }
         ],
         "out" : 2,
         "in" : 3,
         "entry" : [ "true", "true", "true", "false" ],
         "location" : [ 13.39677, 52.54366 ]
      },
      {  "out" : 1,
         "lanes" : [
            { "indications" : [ "straight" ],
               "valid" : true },
            { "indications" : [ "right" ],
               "valid" : false }
         ],
         "bearings" : [ 60, 240, 330 ],
         "in" : 0,
         "entry" : [ "false", "true", "true" ],
         "location" : [ 13.394718, 52.543096 ]
      }
   ],
   "name" : "Lortzingstraße",
   "distance" : 152.3,
   "maneuver" : {
      "modifier" : "right",
      "type" : "turn"
   }
}
```

### StepManeuver object

**Properties**

- `location`: A `[longitude, latitude]` pair describing the location of the turn.
- `bearing_before`: The clockwise angle from true north to the
  direction of travel immediately before the maneuver.  Range 0-359.
- `bearing_after`: The clockwise angle from true north to the
  direction of travel immediately after the maneuver.  Range 0-359.
- `type` A string indicating the type of maneuver. **new identifiers might be introduced without API change**
   Types unknown to the client should be handled like the `turn` type, the existence of correct `modifier` values is guaranteed.

| `type`            | Description                                                                                                                                                                                                                                                                                                                 |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `turn`            | a basic turn into the direction of the `modifier`                                                                                                                                                                                                                                                                           |
| `new name`        | no turn is taken/possible, but the road name changes. The road can take a turn itself, following `modifier`.                                                                                                                                                                                                                |
| `depart`          | indicates the departure of the leg                                                                                                                                                                                                                                                                                          |
| `arrive`          | indicates the destination of the leg                                                                                                                                                                                                                                                                                        |
| `merge`           | merge onto a street (e.g. getting on the highway from a ramp, the `modifier specifies the direction of the merge`)                                                                                                                                                                                                          |
| `ramp`            | **Deprecated**. Replaced by `on_ramp` and `off_ramp`.                                                                                                                                                                                                                                                                       |
| `on ramp`         | take a ramp to enter a highway (direction given my `modifier`)                                                                                                                                                                                                                                                              |
| `off ramp`        | take a ramp to exit a highway (direction given my `modifier`)                                                                                                                                                                                                                                                               |
| `fork`            | take the left/right side at a fork depending on `modifier`                                                                                                                                                                                                                                                                  |
| `end of road`     | road ends in a T intersection turn in direction of `modifier`                                                                                                                                                                                                                                                               |
| `use lane`        | **Deprecated** replaced by lanes on all intersection entries                                                                                                                                                                                                                                                                |
| `continue`        | Turn in direction of `modifier` to stay on the same road                                                                                                                                                                                                                                                                    |
| `roundabout`      | traverse roundabout, if the route leaves the roundabout there will be an additional property `exit` for exit counting. The modifier specifies the direction of entering the roundabout.                                                                                                                                     |
| `rotary`          | a traffic circle. While very similar to a larger version of a roundabout, it does not necessarily follow roundabout rules for right of way. It can offer `rotary_name` and/or `rotary_pronunciation` parameters (located in the RouteStep object) in addition to the `exit` parameter (located on the StepManeuver object). |
| `roundabout turn` | Describes a turn at a small roundabout that should be treated as a normal turn. The `modifier` indicates the turn direction. Example instruction: `At the roundabout turn left`.                                                                                                                                            |
| `notification`    | not an actual turn but a change in the driving conditions. For example the travel mode or classes. If the road takes a turn itself, the `modifier` describes the direction                                                                                                                                                  |
| `exit roundabout` | Describes a maneuver exiting a roundabout (usually preceded by a `roundabout` instruction)                                                                                                                                                                                                                                  |
| `exit rotary`     | Describes the maneuver exiting a rotary (large named roundabout)                                                                                                                                                                                                                                                            |

  Please note that even though there are `new name` and `notification` instructions, the `mode` and `name` can change
  between all instructions. They only offer a fallback in case nothing else is to report.

- `modifier` An optional `string` indicating the direction change of the maneuver.

| `modifier`     | Description                         |
| -------------- | ----------------------------------- |
| `uturn`        | indicates the reversal of direction |
| `sharp right`  | a sharp right turn                  |
| `right`        | a normal turn to the right          |
| `slight right` | a slight turn to the right          |
| `straight`     | no relevant change in direction     |
| `slight left`  | a slight turn to the left           |
| `left`         | a normal turn to the left           |
| `sharp left`   | a sharp turn to the left            |

  The list of turns without a modifier is limited to: `depart/arrive`. If the source/target location is close enough to the `depart/arrive` location, no modifier will be given.

  The meaning depends on the `type` property.

| `type`            | Description                                                                                                           |
| ----------------- | --------------------------------------------------------------------------------------------------------------------- |
| `turn`            | `modifier` indicates the change in direction accomplished through the turn                                            |
| `depart`/`arrive` | `modifier` indicates the position of departure point and arrival point in relation to the current direction of travel |

- `exit` An optional `integer` indicating the number of the exit to take. The property exists for the `roundabout` / `rotary` property:
  Number of the roundabout exit to take. If an exit is `undefined` the destination is on the roundabout.


New properties (potentially depending on `type`) may be introduced in the future without an API version change.

### Lane object

A `Lane` represents a turn lane at the corresponding turn location.

**Properties**

- `indications`: an indication (e.g. marking on the road) specifying the turn lane. A road can have multiple indications (e.g. an arrow pointing straight and left). The indications are given in an array, each containing one of the following types. Further indications might be added on without an API version change.

| `value`        | Description                                                                 |
| -------------- | --------------------------------------------------------------------------- |
| `none`         | No dedicated indication is shown.                                           |
| `uturn`        | An indication signaling the possibility to reverse (i.e. fully bend arrow). |
| `sharp right`  | An indication indicating a sharp right turn (i.e. strongly bend arrow).     |
| `right`        | An indication indicating a right turn (i.e. bend arrow).                    |
| `slight right` | An indication indicating a slight right turn (i.e. slightly bend arrow).    |
| `straight`     | No dedicated indication is shown (i.e. straight arrow).                     |
| `slight left`  | An indication indicating a slight left turn (i.e. slightly bend arrow).     |
| `left`         | An indication indicating a left turn (i.e. bend arrow).                     |
| `sharp left`   | An indication indicating a sharp left turn (i.e. strongly bend arrow).      |

- `valid`: a boolean flag indicating whether the lane is a valid choice in the current maneuver

#### Example

```json
{
    "indications": ["left", "straight"],
    "valid": false
}
 ```

### Intersection object

An intersection gives a full representation of any cross-way the path passes by. For every step, the very first intersection (`intersections[0]`) corresponds to the
location of the StepManeuver. Further intersections are listed for every cross-way until the next turn instruction.

**Properties**

- `location`: A `[longitude, latitude]` pair describing the location of the turn.
- `bearings`: A list of bearing values (e.g. [0,90,180,270]) that are available at the intersection. The bearings describe all available roads at the intersection.  Values are between 0-359 (0=true north)
- `classes`: An array of strings signifying the classes (as specified in the profile) of the road exiting the intersection.
- `entry`: A list of entry flags, corresponding in a 1:1 relationship to the bearings. A value of `true` indicates that the respective road could be entered on a valid route.
  `false` indicates that the turn onto the respective road would violate a restriction.
- `in`: index into bearings/entry array. Used to calculate the bearing just before the turn. Namely, the clockwise angle from true north to the
  direction of travel immediately before the maneuver/passing the intersection. Bearings are given relative to the intersection. To get the bearing
  in the direction of driving, the bearing has to be rotated by a value of 180. The value is not supplied for `depart` maneuvers.
- `out`: index into the bearings/entry array. Used to extract the bearing just after the turn. Namely, The clockwise angle from true north to the
  direction of travel immediately after the maneuver/passing the intersection. The value is not supplied for `arrive` maneuvers.
- `lanes`: Array of `Lane` objects that denote the available turn lanes at the intersection. If no lane information is available for an intersection, the `lanes` property will not be present.

#### Example

```json
{
    "location":[13.394718,52.543096],
    "in":0,
    "out":2,
    "bearings":[60,150,240,330],
    "entry":["false","true","true","true"],
    "classes": ["toll", "restricted"],
    "lanes":{
        "indications": ["left", "straight"],
        "valid": false
    }
}
```

(waypoint-object)=
### Waypoint object

The object is used to describe the waypoint on a route.

**Properties**

- `name` Name of the street the coordinate snapped to
- `location` Array that contains the `[longitude, latitude]` pair of the snapped coordinate
- `distance` The distance, in meters, from the input coordinate to the snapped coordinate
- `hint` Unique internal identifier of the segment (ephemeral, not constant over data updates)
   This can be used on subsequent requests to significantly speed up the query and to connect multiple services.
   E.g. you can use the `hint` value obtained by the `nearest` query as `hint` values for `route` inputs.

#### Example

```json
{
   "hint" : "KSoKADRYroqUBAEAEAAAABkAAAAGAAAAAAAAABhnCQCLtwAA_0vMAKlYIQM8TMwArVghAwEAAQH1a66g",
   "distance" : 4.152629,
   "name" : "Friedrichstraße",
   "location" : [
      13.388799,
      52.517033
   ]
}
```
