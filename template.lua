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

function OnPlayerTakeDamage(playerid, issuerid, amount, weaponid, bodypart)
    printOMP("player", playerid, "damaged by", issuerid, "player from", weaponid, "weapon in", bodypart, "bodypart with", amount, "amount")
end

function OnPlayerGiveDamage(playerid, damagedid, amount, weaponid, bodypart)
    printOMP("player", playerid, "damage player", damagedid, "from weapon", weaponid, "in bodypart", bodypart, "with", amount, "amount")
end

function OnPlayerClickMap(playerid, fX, fY, fZ)
    printOMP("player", playerid, "clicked on map pos:", fX, fY, fZ)
end

function OnPlayerClickPlayer(playerid, clickedplayerid, source)
    printOMP("player", playerid, "clicked on", clickedplayerid, "from", source)
end

function OnClientCheckResponse(playerid, actionid, memaddr, retndata)
    printOMP(playerid, actionid, memaddr, retndata)
end

function OnPlayerUpdate(playerid)
    printOMP("called OnPlayerUpdate for", playerid)
end