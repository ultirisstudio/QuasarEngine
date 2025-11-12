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

-- script.lua
function start()
    -- conteneur vertical racine
    local root = ui.container("root", "vertical")
    root:set_pos(20, 20)
    root:set_padding(8)
    root:set_bg(0.10, 0.11, 0.12, 1.0)

    -- titre
    local title = ui.text("title", "Hello from Lua UI")
    root:addChild(title)

    -- champ de saisie
    local input = ui.text_input("nameBox", "Player")
    input:set_size(220, 28)
    root:addChild(input)

    -- checkbox
    local chk = ui.checkbox("c1", "Enable magic mode", true)
    root:addChild(chk)

    -- slider
    local s = ui.slider("volume", 0.0, 100.0, 50.0)
    s:set_size(240, 24)
    root:addChild(s)

    -- progress
    local p = ui.progress("loading", 0.0, 1.0, 0.25)
    p:set_size(240, 20)
    root:addChild(p)

    -- bouton
    local btn = ui.button("okBtn", "Start", function()
        log("Clicked! name=" .. input:get_text() .. ", vol=" .. s:get_value())
    end)
    btn:set_size(120, 28)
    root:addChild(btn)

    -- on branche la racine
    ui.set_root(root)
end

function update(dt)
    -- exemple : avance la barre de progression
    -- (vous pouvez stocker une ref globale si besoin)
end


