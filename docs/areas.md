@page pedestrian_areas Pedestrian Areas

@sa AreaManager

This OSRM feature provides routing through areas, where people are free to choose their
path. The main motivation for this feature are areas tagged with `highway=pedestrian`,
but any other area is configurable.

Areas in OSM are either closed ways or multipolygon relations. Currently OSRM routes
along the perimeter of a closed way area. It does not route over multipolygon areas at
all.

This feature routes over the inside of the area. It does so by "meshing" the area, ie.
by creating virtual ways between every two entry points of the area. These new ways
follow lines of sight, they never go through obstacles in the area.

This feature is opt-in: To enable it you must define a @ref process_relation function in
your profile and return it like this:

```lua
return {
  setup = setup,
  process_way =  process_way,
  process_node = process_node,
  process_turn = process_turn,
  process_relation = process_relation
}
```

You must also keep multipolygon relations, so that you can use the name on the relation
for turn directions. (Remember that the ways in the relation are untagged.) In your
profile's setup function add or edit the @ref relation_types sequence to include the
type `multipolygon`:

```lua
function setup()
  ...
  return {
    ...
    relation_types = Sequence {
      "multipolygon"
    }
    ...
  }
end
```

### process_relation(profile, relation, relations)

The @ref process_relation function is called for every relation in the input file. If
you want a relation to be meshed, you must call [area_manager:relation()](@ref AreaManager::relation()).

Example:

```lua
function process_relation(profile, relation, relations)
  if relation:has_tag('type', 'multipolygon') and relation:has_tag('highway', 'pedestrian') then
    -- register the relation
    area_manager:relation(relation)
  end
end
```

### process_way(profile, way, result, relations)

The @ref process_way function is called for every way in the input file. If you want a
closed way to be meshed, call [area_manager:way()](@ref AreaManager::way()). (Note that
open ways cannot be meshed and will be ignored.)

Multipolygons need some support too. Since the member ways of a multipolygon relation
are as a rule untagged, you have to copy at least the defining tag (and maybe the name)
from the relation to the way. OSRM discards untagged ways.

Example:

```lua
function process_way(profile, way, result, relations)
  ...
  if way:has_tag('highway', 'pedestrian') and way:has_true_tag('area') then
    -- register the way
    area_manager:way(way)
  end

  for _, rel_id in pairs(area_manager:get_relations(way)) do
    -- if this way is a member of a registered relation
    -- we have to set at least one defining tag
    local rel = relations:relation(rel_id)
    data.highway = rel:get_value_by_key('highway')
    WayHandlers.names(profile, rel, result, data)
  end
  ...
end
```
