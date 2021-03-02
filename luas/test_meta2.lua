local mt = {}

function vector(x, y)
    local v = {x=x, y= y}
    setmetatable(v, mt)
    return v
end

mt.__call= function(v)
    print("[".. v.x .. ", " .. v.y .. "]")
end

mt.__add = function(v1, v2)
    local x = v1.x + v2.x
    local y = v1.y + v2.y 
    --local ret= vector(x,y)
    return vector(x,y)
end
--[[
function foo(a,b)
    return vector(a,b)
end

local x = foo(1,2)
x()
print(x.x)
print(x.y)
--]]
v1 = vector(1, 2)
v2 = vector(3, 4)

-- test arich method
v3 = v1 + v2
v3()
print("v3.x is:",v3.x)
print(v3.y)
