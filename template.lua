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
    printOMP("player", playerid, "sent command:", text)
    return false
end