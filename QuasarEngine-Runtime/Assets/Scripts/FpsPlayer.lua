public = {
    moveForce        = 32.0,
    airControlMul    = 1.0,
    jumpImpulse      = 7.5, 
    maxInputAccel    = 1.0,
    mouseSensitivity = 0.08,
    eyeHeight        = 1.7,
    groundProbeDist  = 0.28,
    viewPitchMinDeg  = -89.0,
    viewPitchMaxDeg  =  89.0,
    rayLength        = 100.0,

    cctRadius        = 0.40,
    cctHeight        = 1.80,
    cctStepOffset    = 0.30,
    cctSlopeLimitDeg = 45.0,
    cctContactOffset = 0.10,
    cctLayer         = 1,
    cctMask          = 0xFFFFFFFF,
    cctIncludeTriggers = false,

    gravityY         = -9.81
}

local tcPlayer    = nil
local cam         = nil
local tcCam       = nil

local grounded    = false
local yawDeg      = 0.0
local pitchDeg    = 0.0
local lastMouseX  = nil
local lastMouseY  = nil

local ctrl        = nil
local velY        = 0.0
local minCCTDist  = 0.001

local prevLMB = false
local lastHitId, lastLogTime = nil, 0

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

local function update_view_from_mouse(dt)
    local dx, dy = capture_mouse_delta()
    local sens = public.mouseSensitivity
    yawDeg   = yawDeg + dx * sens
    pitchDeg = clamp(pitchDeg - dy * sens, public.viewPitchMinDeg, public.viewPitchMaxDeg)
end

local function wish_dir_on_plane()
    local fwd = 0.0
    local side = 0.0
    if is_key_pressed(KEY_Z) or is_key_pressed(KEY_W) then fwd =  1.0 end
    if is_key_pressed(KEY_S) then fwd = -1.0 end
    if is_key_pressed(KEY_D) then side =  1.0 end
    if is_key_pressed(KEY_Q) or is_key_pressed(KEY_A) then side = -1.0 end

    local fwdDir = forward_from_yawpitch(yawDeg, 0.0)
    fwdDir.y = 0.0
    local rightDir = right_from_yaw(yawDeg)

    local wish = vec3(
        fwdDir.x * fwd + rightDir.x * side,
        0.0,
        fwdDir.z * fwd + rightDir.z * side
    )
    local len = math.sqrt(wish.x*wish.x + wish.z*wish.z)
    if len > 1e-6 then
        wish.x, wish.z = wish.x/len, wish.z/len
    else
        wish.x, wish.z = 0.0, 0.0
    end
    return wish
end

local function move_with_cct(dt)
    local wish = wish_dir_on_plane()

    local speed = public.moveForce
    if (wish.x ~= 0.0 or wish.z ~= 0.0) then
        local planar = vec3(wish.x * speed * dt, 0.0, wish.z * speed * dt)
        if not grounded then
            planar.x = planar.x * public.airControlMul
            planar.z = planar.z * public.airControlMul
        end
        velY = velY + public.gravityY * dt
        if grounded and is_key_just_pressed(KEY_SPACE) then
            velY = public.jumpImpulse  -- m/s
        end

        local disp = vec3(planar.x, velY * dt, planar.z)
        local result = ctrl:move(disp, dt, minCCTDist)

        grounded = result.grounded or false
        if grounded and velY < 0.0 then
            velY = 0.0
        end

        local p = result.position
        tcPlayer.position = p
    else
        velY = velY + public.gravityY * dt
        if grounded and velY < 0.0 then velY = 0.0 end
        local disp = vec3(0.0, velY * dt, 0.0)
        local result = ctrl:move(disp, dt, minCCTDist)
        grounded = result.grounded or false
        if grounded and velY < 0.0 then velY = 0.0 end
        tcPlayer.position = result.position
    end
end

local function update_camera_fps()
    if not cam or not tcCam then return end

    local basePos = tcPlayer.position
    tcCam.position = vec3(basePos.x, basePos.y + public.eyeHeight, basePos.z)

    local fwd = forward_from_yawpitch(yawDeg, pitchDeg)
    local lookTarget = vec3(tcCam.position.x + fwd.x, tcCam.position.y + fwd.y, tcCam.position.z + fwd.z)

    local view = lookAt(tcCam.position, lookTarget, vec3(0,1,0))
    local rotM = inverse(view)
    local euler = mat4_to_euler(rotM)
    tcCam.rotation = euler

    tcPlayer.rotation = vec3(0.0, -deg2rad(yawDeg), 0.0)
end

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
    local now = time_seconds and time_seconds() or 0
    if id ~= lastHitId or (now - lastLogTime) > 0.25 then
        log(string.format("[raycam] Hit '%s' (%.2fm)", entName, dist))
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

    local tag = entity:hasComponent("TagComponent") and entity:getComponent("TagComponent") or entity:addComponent("TagComponent")
    tag:Add(TagMask.Player)

    if entity.removeComponent and entity:hasComponent("RigidBodyComponent") then
        entity:removeComponent("RigidBodyComponent")
    end

    ctrl = character.create_capsule(tcPlayer.position, public.cctRadius, public.cctHeight, public.cctStepOffset, public.cctSlopeLimitDeg, public.cctContactOffset)
    if ctrl then
        ctrl:set_layer_mask(public.cctLayer, public.cctMask)
        ctrl:set_include_triggers(public.cctIncludeTriggers)
        ctrl:set_position(vec3(tcPlayer.position.x, tcPlayer.position.y + 0.02, tcPlayer.position.z))
    else
        log("[player] Echec creation CharacterController (capsule).")
    end

    local c = getEntityByName("PlayerCamera")
    if c and c:hasComponent("TransformComponent") then
        cam = c
        tcCam = cam:getComponent("TransformComponent")
    end
end

function update(dt)
    if not tcPlayer or not ctrl then return end

    update_view_from_mouse(dt)
    move_with_cct(dt)
    update_camera_fps()
    raycast_from_camera()
end

function stop()
    if ctrl then
        character.destroy(ctrl)
        ctrl = nil
    end
end
