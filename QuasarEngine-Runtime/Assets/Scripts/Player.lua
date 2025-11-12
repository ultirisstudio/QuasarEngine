public = {}

public.speed = 5
public.smoothSpeed = 5.0
public.sensitivity = 0.1

local lastMouseX, lastMouseY = get_mouse_x(), get_mouse_y()
local yaw, pitch = 0, 0
local cameraDistance = 10.0

function start()
    log("Script initialise")
end

function update(dt)
    local transform = self:getComponent("TransformComponent")
    local pos = transform.position

    if is_key_pressed(KEY_W) or is_key_pressed(KEY_Z) then pos.z = pos.z + public.speed * dt end
    if is_key_pressed(KEY_S) then pos.z = pos.z - public.speed * dt end
    if is_key_pressed(KEY_A) or is_key_pressed(KEY_Q) then pos.x = pos.x - public.speed * dt end
    if is_key_pressed(KEY_D) then pos.x = pos.x + public.speed * dt end

    transform.position = pos

    updateCamera(dt)
end

function updateCamera(dt)
    local player = getEntityByName("Player")
    local camera = getEntityByName("PlayerCamera")

    if not player or not camera then return end
    if not player:hasComponent("TransformComponent") or not camera:hasComponent("TransformComponent") then return end

    local playerTrans = player:getComponent("TransformComponent")
    local camTrans = camera:getComponent("TransformComponent")
    if not playerTrans or not camTrans then return end

    local playerPos = playerTrans.position
    local camTransform = camTrans

    local mouseX, mouseY = get_mouse_x(), get_mouse_y()
    local deltaX = mouseX - lastMouseX
    local deltaY = mouseY - lastMouseY
    lastMouseX, lastMouseY = mouseX, mouseY

    yaw = yaw + deltaX * (public.sensitivity or 0.1)
    pitch = pitch - deltaY * (public.sensitivity or 0.1)
    pitch = math.max(-89, math.min(89, pitch))

    local radYaw = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local targetX = playerPos.x + cameraDistance * math.cos(radPitch) * math.sin(radYaw)
    local targetY = playerPos.y + cameraDistance * math.sin(radPitch)
    local targetZ = playerPos.z + cameraDistance * math.cos(radPitch) * math.cos(radYaw)

    camTransform.position.x = camTransform.position.x + (targetX - camTransform.position.x) * public.smoothSpeed * dt
    camTransform.position.y = camTransform.position.y + (targetY - camTransform.position.y) * public.smoothSpeed * dt
    camTransform.position.z = camTransform.position.z + (targetZ - camTransform.position.z) * public.smoothSpeed * dt

    local up = vec3(0,1,0)
    local viewMatrix = lookAt(camTransform.position, playerPos, up)

    local rotationMatrix = inverse(viewMatrix)
    camTransform.rotation = mat4_to_euler(rotationMatrix)
end
