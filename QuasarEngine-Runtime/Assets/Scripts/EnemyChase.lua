public = {
    moveSpeed = 6.0,
    turnLerp  = 10.0,
    gravity   = true,
    stopDistance = 5.0
}

local selfTc, selfRb
local player, playerTc

local function wrap_delta_deg(d)
    while d > 180.0 do d = d - 360.0 end
    while d < -180.0 do d = d + 360.0 end
    return d
end

function start()
    selfTc = entity:getComponent("TransformComponent")
    if entity:hasComponent("RigidBodyComponent") then
        selfRb = entity:getComponent("RigidBodyComponent")
    else
        selfRb = entity:addComponent("RigidBodyComponent")
        selfRb.enableGravity = public.gravity
        selfRb:set_body_type("DYNAMIC")
        selfRb:set_linear_damping(1.0)
        selfRb:set_angular_damping(1.0)
    end

    local tag = entity:hasComponent("TagComponent") and entity:getComponent("TagComponent") or entity:addComponent("TagComponent")
    tag:Add(TagMask.Enemy)

    player = getEntityByName("Player")
    if player and player:hasComponent("TransformComponent") then
        playerTc = player:getComponent("TransformComponent")
    end
end

function update(dt)
    if not playerTc or not selfTc then return end

    local to = vec3(playerTc.position.x - selfTc.position.x, 0.0, playerTc.position.z - selfTc.position.z)
    local len = math.sqrt(to.x*to.x + to.z*to.z)
    if len < 1e-3 then return end
    to.x, to.z = to.x/len, to.z/len

    if len <= public.stopDistance then
        local v = physics.get_linear_velocity(entity)
        physics.set_linear_velocity(entity, vec3(0.0, v.y, 0.0))
        return
    end

    local v = physics.get_linear_velocity(entity)
    local vx, vz = to.x * public.moveSpeed, to.z * public.moveSpeed
    physics.set_linear_velocity(entity, vec3(vx, v.y, vz))

    local desiredYaw = math.deg(atan2(to.x, -to.z))
    local curYaw = math.deg(selfTc.rotation.y)
    local delta = wrap_delta_deg(desiredYaw - curYaw)
    local t = math.min(1.0, public.turnLerp * dt)
    local newYaw = curYaw + delta * t

    physics.set_rotation(entity, vec3(0.0, math.rad(newYaw), 0.0))
end

function stop() end
