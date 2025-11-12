public = {
    moveSpeed = 12.0,
    turnLerp  = 10.0,
    gravity   = true,
    stopDistance = 5.0,
	
	headingSign = -1.0,
    headingOffsetDeg = 0.0
}

local selfTc, selfRb
local player, playerTc

function start()
    selfTc = entity:getComponent("TransformComponent")
    if entity:hasComponent("RigidBodyComponent") then
        selfRb = entity:getComponent("RigidBodyComponent")
    else
        selfRb = entity:addComponent("RigidBodyComponent")
        selfRb.enableGravity = public.gravity
        selfRb:set_body_type("DYNAMIC")
        selfRb:set_linear_damping(0.2)
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

    local dx = playerTc.position.x - selfTc.position.x
    local dz = playerTc.position.z - selfTc.position.z
    local len = math.sqrt(dx*dx + dz*dz)
    if len < 1e-6 then return end

    local toX, toZ = dx/len, dz/len

    -- Hystérésis pour éviter l’oscillation au seuil
    local stop = public.stopDistance or 5.0
    local start = stop + 0.5

    if len > start then
        local accel = public.moveSpeed
        physics.add_force(entity, vec3(toX * accel, 0.0, toZ * accel), "force")
        -- Option : tu peux réveiller explicitement si jamais
        -- local v = physics.get_linear_velocity(entity)
        -- if v.x*v.x + v.z*v.z < 1e-4 then physics.add_force(entity, vec3(toX*accel, 0.0, toZ*accel), "impulse") end
    end

    local desiredYawDeg = math.deg(atan2(toX, -toZ))
    local yawOutDeg = (public.headingSign or -1.0) * desiredYawDeg + (public.headingOffsetDeg or 0.0)
    if physics.set_rotation then
        physics.set_rotation(entity, vec3(0.0, math.rad(yawOutDeg), 0.0))
    end
end

function stop() end


