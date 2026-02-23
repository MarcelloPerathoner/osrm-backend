## Result Objects in Flatbuffers Format

The default response format is `json`, but OSRM supports binary [`flatbuffers`](https://google.github.io/flatbuffers/) format, which
is much faster in serialization/deserialization, comparing to `json`.

The format itself is described in message descriptors, located at `include/engine/api/flatbuffers` directory. Those descriptors could
be compiled to provide protocol parsers in Go/Javascript/Typescript/Java/Dart/C#/Python/Lobster/Lua/Rust/PHP/Kotlin. Precompiled
protocol parser for C++ is supplied with OSRM.

`Flatbuffers` format provides exactly the same data, as `json` format with a slightly different layout, which was optimized to minimize
in-transfer size.

### Root object

Root object is the only object, available from a 'raw' `flatbuffers` buffer. It can be constructed with a following call:

         auto osrm = osrm::engine::api::fbresult::GetFBResult(some_input_buffer);

**Properties**

- `error`: `bool` Marks response as erroneous. An erroneous response should include the `code` fieldset, all the other fields may not be present.
- `code`: `Error` Error description object, only present, when `error` is `true`
- `waypoints`: `[Waypoint]` Array of `Waypoint` objects. Should present for every service call, unless `skip_waypoints` is set to `true`. Table service will put `sources` array here.
- `routes`: `[RouteObject]` Array of `RouteObject` objects. May be empty or absent. Should present for Route/Trip/Match services call.
- `table`: `Table` Table object, may absent. Should be present in case of Table service call.

### Error object

Contains error information.

**Properties**

- `code`: `string` Error code
- `message`: `string` Detailed error message

### Waypoint object

Almost the same as `json` Waypoint object. The following properties differ:

- `location`: `Position` Same as `json` location field, but different format.
- `nodes`: `Uint64Pair` Same as `json` nodes field, but different format.

### RouteObject object

Almost the same as `json` Route object. The following properties differ:

- `polyline`: `string` Same as `json` geometry.polyline or geometry.polyline6 fields. One field for both formats.
- `coordinates`: `[Position]` Same as `json` geometry.coordinates field, but different format.
- `legs`: `[Leg]` Array of `Leg` objects.

### Leg object

Almost the same as `json` Leg object. The following properties differ:

- `annotations`: `Annotation` Same as `json` annotation field, but different format.
- `steps`: `[Step]` Same as `step` annotation field, but different format.

### Step object

Almost the same as `json` Step object. The following properties differ:

- `polyline`: `string` Same as `json` geometry.polyline or geometry.polyline6 fields. One field for both formats.
- `coordinates`: `[Position]` Same as `json` geometry.coordinates field, but different format.
- `maneuver`: `StepManeuver` Same as `json` maneuver field, but different format.

| `type`           | Description                                                                                                                                                                                                                                                                                                                 |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Turn`           | a basic turn into the direction of the `modifier`                                                                                                                                                                                                                                                                           |
| `NewName`        | no turn is taken/possible, but the road name changes. The road can take a turn itself, following `modifier`.                                                                                                                                                                                                                |
| `Depart`         | indicates the departure of the leg                                                                                                                                                                                                                                                                                          |
| `Arrive`         | indicates the destination of the leg                                                                                                                                                                                                                                                                                        |
| `Merge`          | merge onto a street (e.g. getting on the highway from a ramp, the `modifier specifies the direction of the merge`)                                                                                                                                                                                                          |
| `OnRamp`         | take a ramp to enter a highway (direction given my `modifier`)                                                                                                                                                                                                                                                              |
| `OffRamp`        | take a ramp to exit a highway (direction given my `modifier`)                                                                                                                                                                                                                                                               |
| `Fork`           | take the left/right side at a fork depending on `modifier`                                                                                                                                                                                                                                                                  |
| `EndOfRoad`      | road ends in a T intersection turn in direction of `modifier`                                                                                                                                                                                                                                                               |
| `Continue`       | Turn in direction of `modifier` to stay on the same road                                                                                                                                                                                                                                                                    |
| `Roundabout`     | traverse roundabout, if the route leaves the roundabout there will be an additional property `exit` for exit counting. The modifier specifies the direction of entering the roundabout.                                                                                                                                     |
| `Rotary`         | a traffic circle. While very similar to a larger version of a roundabout, it does not necessarily follow roundabout rules for right of way. It can offer `rotary_name` and/or `rotary_pronunciation` parameters (located in the RouteStep object) in addition to the `exit` parameter (located on the StepManeuver object). |
| `RoundaboutTurn` | Describes a turn at a small roundabout that should be treated as a normal turn. The `modifier` indicates the turn direction. Example instruction: `At the roundabout turn left`.                                                                                                                                            |
| `Notification`   | not an actual turn but a change in the driving conditions. For example the travel mode or classes. If the road takes a turn itself, the `modifier` describes the direction                                                                                                                                                  |
| `ExitRoundabout` | Describes a maneuver exiting a roundabout (usually preceded by a `roundabout` instruction)                                                                                                                                                                                                                                  |
| `ExitRotary`     | Describes the maneuver exiting a rotary (large named roundabout)                                                                                                                                                                                                                                                            |

- `driving_side`: `bool` True stands for left side driving.
- `intersections`: `[Intersection]` Same as `json` intersections field, but different format.

### Intersection object

Almost the same as `json` Intersection object. The following properties differ:

- `location`: `Position` Same as `json` location property, but in a different format.
- `lanes`: `[Lane]` Array of `Lane` objects.

### Lane object

Almost the same as `json` Lane object. The following properties differ:

- `indications`: `Turn` Array of `Turn` enum values.

| `value`       | Description                                                                 |
| ------------- | --------------------------------------------------------------------------- |
| `None`        | No dedicated indication is shown.                                           |
| `UTurn`       | An indication signaling the possibility to reverse (i.e. fully bend arrow). |
| `SharpRight`  | An indication indicating a sharp right turn (i.e. strongly bend arrow).     |
| `Right`       | An indication indicating a right turn (i.e. bend arrow).                    |
| `SlightRight` | An indication indicating a slight right turn (i.e. slightly bend arrow).    |
| `Straight`    | No dedicated indication is shown (i.e. straight arrow).                     |
| `SlightLeft`  | An indication indicating a slight left turn (i.e. slightly bend arrow).     |
| `Left`        | An indication indicating a left turn (i.e. bend arrow).                     |
| `SharpLeft`   | An indication indicating a sharp left turn (i.e. strongly bend arrow).      |

### StepManeuver object

Almost the same as `json` StepManeuver object. The following properties differ:

- `location`: `Position` Same as `json` location property, but in a different format.
- `type`: `ManeuverType` Type of a maneuver (enum)

| `type`           | Description                                                                                                                                                                                                                                                                                                                 |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Turn`           | a basic turn into the direction of the `modifier`                                                                                                                                                                                                                                                                           |
| `NewName`        | no turn is taken/possible, but the road name changes. The road can take a turn itself, following `modifier`.                                                                                                                                                                                                                |
| `Depart`         | indicates the departure of the leg                                                                                                                                                                                                                                                                                          |
| `Arrive`         | indicates the destination of the leg                                                                                                                                                                                                                                                                                        |
| `Merge`          | merge onto a street (e.g. getting on the highway from a ramp, the `modifier specifies the direction of the merge`)                                                                                                                                                                                                          |
| `OnRamp`         | take a ramp to enter a highway (direction given my `modifier`)                                                                                                                                                                                                                                                              |
| `OffRamp`        | take a ramp to exit a highway (direction given my `modifier`)                                                                                                                                                                                                                                                               |
| `Fork`           | take the left/right side at a fork depending on `modifier`                                                                                                                                                                                                                                                                  |
| `EndOfRoad`      | road ends in a T intersection turn in direction of `modifier`                                                                                                                                                                                                                                                               |
| `Continue`       | Turn in direction of `modifier` to stay on the same road                                                                                                                                                                                                                                                                    |
| `Roundabout`     | traverse roundabout, if the route leaves the roundabout there will be an additional property `exit` for exit counting. The modifier specifies the direction of entering the roundabout.                                                                                                                                     |
| `Rotary`         | a traffic circle. While very similar to a larger version of a roundabout, it does not necessarily follow roundabout rules for right of way. It can offer `rotary_name` and/or `rotary_pronunciation` parameters (located in the RouteStep object) in addition to the `exit` parameter (located on the StepManeuver object). |
| `RoundaboutTurn` | Describes a turn at a small roundabout that should be treated as a normal turn. The `modifier` indicates the turn direction. Example instruction: `At the roundabout turn left`.                                                                                                                                            |
| `Notification`   | not an actual turn but a change in the driving conditions. For example the travel mode or classes. If the road takes a turn itself, the `modifier` describes the direction                                                                                                                                                  |
| `ExitRoundabout` | Describes a maneuver exiting a roundabout (usually preceded by a `roundabout` instruction)                                                                                                                                                                                                                                  |
| `ExitRotary`     | Describes the maneuver exiting a rotary (large named roundabout)                                                                                                                                                                                                                                                            |

- `modifier`: `Turn` Maneuver turn (enum)

### Annotation object

Exactly the same as `json` annotation object.


### Position object

A point on Earth.

***Properties***
- `longitude`: `float` Point's longitude
- `latitude`: `float` Point's latitude

### Uint64Pair

A pair of long long integers. Used only by `Waypoint` object.

***Properties***
- `first`: `uint64` First pair value.
- `second`: `uint64` Second pair value.

### Table object

Almost the same as `json` Table object. The main difference is that 'sources' field is absent and the root's object 'waypoints' field is
used instead. All the other differences follow:

- `durations`: `[float]` Flat representation of a durations matrix. Element at row;col can be addressed as [row * cols + col]
- `distances`: `[float]` Flat representation of a destinations matrix. Element at row;col can be addressed as [row * cols + col]
- `destinations`: `[Waypoint]` Array of `Waypoint` objects. Will be `null` if `skip_waypoints` will be set to `true`
- `rows`: `ushort` Number of rows in durations/destinations matrices.
- `cols`: `ushort` Number of cols in durations/destinations matrices.
