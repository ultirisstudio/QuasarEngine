public = {
    intervalSec      = 1.5,
    maxAlive         = 20,
    spawnRadius      = 18.0,
    enemyScriptPath  = "Assets/Scripts/EnemyChase.lua",
    enemyBaseName    = "Enemy",
    startDelaySec    = 0.0
}

local timer = 0.0
local enemies = {}
local alive = 0

local function random_on_circle(r)
    local a = math.random() * 6.283185307179586
    return r * math.cos(a), r * math.sin(a)
end

local function prune_enemies()
    for i = #enemies, 1, -1 do
        local e = enemies[i]
        if (not e) or (not e:isValid()) then
            table.remove(enemies, i)
        end
    end
    alive = #enemies
end

local VERTS_CUBE = {
    -- +Z
    -1,-1, 1,  0,0,1,  0,0, 0,0,0, 0,0,0,1,
     1,-1, 1,  0,0,1,  1,0, 0,0,0, 0,0,0,1,
     1, 1, 1,  0,0,1,  1,1, 0,0,0, 0,0,0,1,
    -1, 1, 1,  0,0,1,  0,1, 0,0,0, 0,0,0,1,
    -- -Z
     1,-1,-1,  0,0,-1, 1,0, 0,0,0, 0,0,0,1,
    -1,-1,-1,  0,0,-1, 0,0, 0,0,0, 0,0,0,1,
    -1, 1,-1,  0,0,-1, 0,1, 0,0,0, 0,0,0,1,
     1, 1,-1,  0,0,-1, 1,1, 0,0,0, 0,0,0,1,
    -- +X
     1,-1, 1,  1,0,0,  1,0, 0,0,0, 0,0,0,1,
     1,-1,-1,  1,0,0,  0,0, 0,0,0, 0,0,0,1,
     1, 1,-1,  1,0,0,  0,1, 0,0,0, 0,0,0,1,
     1, 1, 1,  1,0,0,  1,1, 0,0,0, 0,0,0,1,
    -- -X
    -1,-1,-1, -1,0,0,  0,0, 0,0,0, 0,0,0,1,
    -1,-1, 1, -1,0,0,  1,0, 0,0,0, 0,0,0,1,
    -1, 1, 1, -1,0,0,  1,1, 0,0,0, 0,0,0,1,
    -1, 1,-1, -1,0,0,  0,1, 0,0,0, 0,0,0,1,
    -- +Y
    -1, 1, 1,  0,1,0,  0,1, 0,0,0, 0,0,0,1,
     1, 1, 1,  0,1,0,  1,1, 0,0,0, 0,0,0,1,
     1, 1,-1,  0,1,0,  1,0, 0,0,0, 0,0,0,1,
    -1, 1,-1,  0,1,0,  0,0, 0,0,0, 0,0,0,1,
    -- -Y
    -1,-1,-1,  0,-1,0, 0,0, 0,0,0, 0,0,0,1,
     1,-1,-1,  0,-1,0, 1,0, 0,0,0, 0,0,0,1,
     1,-1, 1,  0,-1,0, 1,1, 0,0,0, 0,0,0,1,
    -1,-1, 1,  0,-1,0, 0,1, 0,0,0, 0,0,0,1
}

local IDX_CUBE = {
    0,1,2,  0,2,3,
    4,5,6,  4,6,7,
    8,9,10,  8,10,11,
    12,13,14,  12,14,15,
    16,17,18,  16,18,19,
    20,21,22,  20,22,23,
}

local function spawn_one()
    prune_enemies()
    if alive >= public.maxAlive then return end

    local spawnerTc = entity:getComponent("TransformComponent")
    local ox, oz = random_on_circle(public.spawnRadius)
    local pos = vec3(spawnerTc.position.x + ox, spawnerTc.position.y + 1.0, spawnerTc.position.z + oz)

    local name = string.format("%s_%d", public.enemyBaseName, math.floor(getTime()*1000))
    local e = createEntity(name)

    local tc = e:addComponent("TransformComponent")
    tc.position = pos
    tc.rotation = vec3(0,0,0)
    tc.scale    = vec3(1,1,1)

    local rb = e:addComponent("RigidBodyComponent")
    rb.enableGravity = true
    rb:set_body_type("DYNAMIC")
    rb:set_linear_damping(1.0)
    rb:set_angular_damping(1.0)

    local bc = e:addComponent("BoxColliderComponent")
    bc.useEntityScale = true
    bc.size = vec3(1,1,1)
    bc.mass = 1.0
    bc.friction = 0.5
    bc.bounciness = 0.1
    bc:Init()

    local tag = e:addComponent("TagComponent")
    tag:Add(TagMask.Enemy | TagMask.Dynamic)

    attachScript(e, public.enemyScriptPath)

    local child_name = string.format("%s_%s", name, "body")
    local child_e = createEntity(child_name)

    child_e:addComponent("MeshRendererComponent")

    local cmc = child_e:addComponent("MeshComponent", child_name)
    cmc:generateMesh(VERTS_CUBE, IDX_CUBE)

    child_e:addComponent("MaterialComponent")
    setParent(child_e, e)

    enemies[#enemies + 1] = e
    alive = #enemies
end

function start()
    math.randomseed(os.time())
end

function update(dt)
    prune_enemies()

    timer = timer + dt
    if timer < public.startDelaySec then return end
    timer = timer - public.startDelaySec
    public.startDelaySec = 0.0

    if timer >= public.intervalSec then
        timer = timer - public.intervalSec
        spawn_one()
    end
end

function stop() end
