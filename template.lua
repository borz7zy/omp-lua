function OnIncomingConnection(playerid, ip_address, port)
    printOMP("Player incoming conntection:", playerid, ip_address, port)
end

function OnPlayerConnect(playerid)
    printOMP("player connected:", playerid)
end

function OnPlayerDisconnect(playerid, reason)
    printOMP("player disconnected: ", playerid, reason)
end