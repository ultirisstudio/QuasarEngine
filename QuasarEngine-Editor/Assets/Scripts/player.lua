function onStart()
    print("[player.lua] Script démarré")
end

function update(dt)	
    local transform = self:getComponent("TransformComponent")
    if transform ~= nil then
		transform.position.x = transform.position.x + 0.1
        transform.position.y = transform.position.y + 0.1
		
        print(string.format("[player.lua] Position: x = %.2f, y = %.2f, z = %.2f", 
            transform.position.x, 
            transform.position.y, 
            transform.position.z
        ))
    else
        print("[player.lua] TransformComponent not found")
    end
end
