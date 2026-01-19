public = {
    moveForce       = 32.0,
    airControlMul   = 0.35,
    jumpImpulse     = 220.0,
    maxInputAccel   = 1.0,
    mouseSensitivity= 0.08,
    eyeHeight       = 1.7,
    groundProbeDist = 0.28,
    viewPitchMinDeg = -89.0,
    viewPitchMaxDeg =  89.0,
    rayLength       = 100.0
}

local rb          = nil
local tcPlayer    = nil
local cam         = nil
local tcCam       = nil

local grounded    = false
local yawDeg      = 0.0
local pitchDeg    = 0.0
local lastMouseX  = nil
local lastMouseY  = nil

local function ground_check()
    local origin = tcPlayer.position + vec3(0.0, 0.1, 0.0)
    local hit = physics.raycast(origin, vec3(0.0, -1.0, 0.0), public.groundProbeDist, true, true)
    grounded = (hit.hit == true)
end

local function capture_mouse_delta()
    local mx, my = get_mouse_x(), get_mouse_y()
    if lastMouseX == nil then
        lastMouseX, lastMouseY = mx, my
        return 0.0, 0.0
    end
    local dx, dy = mx - lastMouseX, my - lastMouseY
    lastMouseX, lastMouseY = mx, my
    return dx, dy
end

local function apply_movement(dt)
    local fwd = 0.0
    local side = 0.0
    if is_key_pressed(KEY_Z) or is_key_pressed(KEY_W) then fwd =  1.0 end
    if is_key_pressed(KEY_S) then fwd = -1.0 end
    if is_key_pressed(KEY_D) then side = 1.0 end
    if is_key_pressed(KEY_Q) or is_key_pressed(KEY_A) then side = -1.0 end

    local fwdDir  = forward_from_yawpitch(yawDeg, 0.0)
    fwdDir.y = 0.0
    local rightDir= right_from_yaw(yawDeg)

    local wish = vec3(
        fwdDir.x * fwd + rightDir.x * side,
        0.0,
        fwdDir.z * fwd + rightDir.z * side
    )
    local len = math.sqrt(wish.x*wish.x + wish.z*wish.z)
    if len > 1e-6 then
        wish.x, wish.z = wish.x/len, wish.z/len

        local ctrl = grounded and 1.0 or public.airControlMul
        local force = public.moveForce * ctrl
        force = math.min(force, public.moveForce * public.maxInputAccel)

        physics.add_force(entity, vec3(wish.x * force, 0.0, wish.z * force), "force")
    end

    if grounded and is_key_just_pressed(KEY_SPACE) then
        physics.add_force(entity, vec3(0.0, public.jumpImpulse, 0.0), "impulse")
    end
end

local function update_view_from_mouse(dt)
    local dx, dy = capture_mouse_delta()
    local sens = public.mouseSensitivity
    yawDeg   = yawDeg + dx * sens
    pitchDeg = pitchDeg - dy * sens
    pitchDeg = clamp(pitchDeg, public.viewPitchMinDeg, public.viewPitchMaxDeg)
end

local function update_camera_fps()
    if not cam or not tcCam then return end

    tcCam.position = vec3(
        tcPlayer.position.x,
        tcPlayer.position.y + public.eyeHeight,
        tcPlayer.position.z
    )

    tcCam.rotation = vec3(
        deg2rad(pitchDeg),
        -deg2rad(yawDeg),
        0.0
    )
end

local prevLMB = false
local lastHitId, lastLogTime = nil, 0

local function raycast_from_camera()
    if not cam or not tcCam then return end

    local length = (public.rayLength and public.rayLength > 0) and public.rayLength or 100.0
    local origin = tcCam.position
    local dir = normalize(forward_from_yawpitch(yawDeg, pitchDeg))

    local hit = physics.raycast(origin, dir, length, true, true)
    if not hit.hit then return end

    local ent = hit.entity
    local entName = (ent and ent:name()) or "?"
    local dist = hit.distance or 0.0

    local id = ent and ent:id() or 0
    local now = getTime and getTime() or 0
    if id ~= lastHitId or (now - lastLogTime) > 0.25 then
        log(string.format("[raycam] Hit '%s' a %.2fm", entName, dist))
        lastHitId, lastLogTime = id, now
    end

    local lmb = is_mouse_button_pressed(MOUSE_BUTTON_LEFT)
    local justPressed = lmb and not prevLMB
    prevLMB = lmb

    if justPressed and ent and ent:hasComponent("TagComponent") then
        local tag = ent:getComponent("TagComponent")
        if tag:HasAny(TagMask.Enemy | TagMask.Boss) then
            destroyEntity(ent)
        end
    end
end

function start()
    if not entity:hasComponent("TransformComponent") then
        entity:addComponent("TransformComponent")
    end
    tcPlayer = entity:getComponent("TransformComponent")

    if entity:hasComponent("RigidBodyComponent") then
        rb = entity:getComponent("RigidBodyComponent")
    else
        rb = entity:addComponent("RigidBodyComponent")
        rb.enableGravity = true
        rb:set_body_type("DYNAMIC")
        rb:set_linear_damping(4.0)
        rb:set_angular_damping(4.0)
    end

    local c = getEntityByName("PlayerCamera")
    if c and c:hasComponent("TransformComponent") then
        cam = c
        tcCam = cam:getComponent("TransformComponent")
    end
end

function update(dt)
    if not tcPlayer or not rb then return end

    update_view_from_mouse(dt)

    physics.set_rotation(entity, vec3(0.0, -deg2rad(yawDeg), 0.0))

    ground_check()
    apply_movement(dt)
    update_camera_fps()
    raycast_from_camera()
end

function stop()
end
