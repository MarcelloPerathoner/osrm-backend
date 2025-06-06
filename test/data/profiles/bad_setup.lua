-- Copy of testbot profile, with setup throwing a runtime error

api_version = 4

function setup()
  if (2 < nil) then -- arithmetic with nil should error
    return {}
  end

  return {
    properties = {
      continue_straight_at_waypoint = true,
      max_speed_for_map_matching    = 30/3.6, --km -> m/s
      weight_name                   = 'duration',
      process_call_tagless_node     = false,
      u_turn_penalty                = 20,
      traffic_signal_penalty        = 7,     -- seconds
      use_turn_restrictions         = true
    },

    classes = {"motorway", "toll", "TooWords2"},

    excludable = {
        {["motorway"] = true},
        {["toll"] = true},
        {["motorway"] = true, ["toll"] = true}
    },

    default_speed  = 24,
    speeds = {
      primary = 36,
      secondary = 18,
      tertiary = 12,
      steps = 6
    }
  }
end

function process_node (profile, node, result)
  -- check if node is a traffic light
  -- TODO: a way to set the penalty value
end

function process_way (profile, way, result)
  local highway = way:get_value_by_key("highway")
  local toll = way:get_value_by_key("toll")
  local name = way:get_value_by_key("name")
  local oneway = way:get_value_by_key("oneway")
  local route = way:get_value_by_key("route")
  local duration = way:get_value_by_key("duration")
  local maxspeed = tonumber(way:get_value_by_key ( "maxspeed"))
  local maxspeed_forward = tonumber(way:get_value_by_key( "maxspeed:forward"))
  local maxspeed_backward = tonumber(way:get_value_by_key( "maxspeed:backward"))
  local junction = way:get_value_by_key("junction")

  if name then
    result.name = name
  end

  result.forward_mode = mode.driving
  result.backward_mode = mode.driving

  if duration and durationIsValid(duration) then
    result.duration = math.max( 1, parseDuration(duration) )
    result.forward_mode = mode.route
    result.backward_mode = mode.route
  else
    local speed_forw = profile.speeds[highway] or profile.default_speed
    local speed_back = speed_forw

    if highway == "river" then
      local temp_speed = speed_forw
      result.forward_mode = mode.river_down
      result.backward_mode = mode.river_up
      speed_forw = temp_speed*1.5
      speed_back = temp_speed/1.5
    elseif highway == "steps" then
      result.forward_mode = mode.steps_down
      result.backward_mode = mode.steps_up
    end

    if maxspeed_forward ~= nil and maxspeed_forward > 0 then
      speed_forw = maxspeed_forward
    else
      if maxspeed ~= nil and maxspeed > 0 and speed_forw > maxspeed then
        speed_forw = maxspeed
      end
    end

    if maxspeed_backward ~= nil and maxspeed_backward > 0 then
      speed_back = maxspeed_backward
    else
      if maxspeed ~=nil and maxspeed > 0 and speed_back > maxspeed then
        speed_back = maxspeed
      end
    end

    result.forward_speed = speed_forw
    result.backward_speed = speed_back
  end

  if oneway == "no" or oneway == "0" or oneway == "false" then
    -- nothing to do
  elseif oneway == "-1" then
    result.forward_mode = mode.inaccessible
  elseif oneway == "yes" or oneway == "1" or oneway == "true" or junction == "roundabout" then
    result.backward_mode = mode.inaccessible
  end

  if highway == 'motorway' then
      result.forward_classes["motorway"] = true
      result.backward_classes["motorway"] = true
  end

  if toll == "yes" then
      result.forward_classes["toll"] = true
      result.backward_classes["toll"] = true
  end

  if junction == 'roundabout' then
    result.roundabout = true
  end
end

function process_turn (profile, turn)
  if turn.is_u_turn then
    turn.duration = turn.duration + profile.properties.u_turn_penalty
    turn.weight = turn.weight + profile.properties.u_turn_penalty
  end
  if turn.has_traffic_light then
     turn.duration = turn.duration + profile.properties.traffic_signal_penalty
  end
end

return {
  setup = setup,
  process_way = process_way,
  process_node = process_node,
  process_turn = process_turn
}
