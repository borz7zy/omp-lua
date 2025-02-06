function OnIncomingConnection(playerid, ip_address, port)
    printOMP("Player incoming conntection:", playerid, ip_address, port)
end

function OnPlayerConnect(playerid)
    printOMP("player connected:", playerid)
end

function OnPlayerDisconnect(playerid, reason)
    printOMP("player disconnected: ", playerid, reason)
end

function onPlayerClientInit(playerid)
    -- TODO
end

function OnPlayerRequestSpawn(playerid)
    printOMP("player request spawn:", playerid)
    return true
end

function OnPlayerStreamIn(playerid, forplayerid)
    printOMP("player", playerid, "stream in for", forplayerid)
end

function OnPlayerStreamOut(playerid, forplayerid)
    printOMP("player", playerid, "stream out for", forplayerid)
end

function OnPlayerText(playerid, text)
    printOMP("player", playerid, "sent message in chat:", text)
    return true
end

function OnPlayerCommandText(playerid, cmdtext)
    printOMP("player", playerid, "sent command:", cmdtext)
    return false
end

function OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, fX, fY, fZ)
    printOMP("player", playerid, "made a shot:", weaponid, hittype, hitid, fX, fY, fZ)
end

function OnPlayerInteriorChange(playerid, newinteriorid, oldinteriorid)
    printOMP("player", playerid, "enter in", newinteriorid, "interior from", oldinteriorid)
end

function OnPlayerStateChange(playerid, newstate, oldstate)
    printOMP("player", playerid, "new state:", newstate, ", old state:", oldstate)
end

function OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
    printOMP("player", playerid, "pressed new keys:", newkeys, ", old keys:", oldkeys)
end

function OnPlayerDeath(playerid, killerid, reason)
    printOMP("player", playerid, "killed by:", killerid, ", weapon/reason:", reason)
end