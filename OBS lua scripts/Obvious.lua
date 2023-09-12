local version = "0.1"

PositionProportionateX=10
PositionProportionateY=20
ScaleX=30
ScaleY=40
MediaRestart=50
MediaStop=60
MediaPlay=70
MediaPause=80
MediaCursor=90
Hue=100
Saturation=110
RollSpeedH=120
RollSpeedV=130
Opacity=140
CropTop=150
CropBottom=160
CropLeft=170
CropRight=180
SetVisible=190
TypeText=200
TypeSetCursorCharacter=210
TypeSetCursorVisible=220
TypeSetNumChars=230
Disconnect=240
Heartbeat=250
HeartbeatResponse=260

ResponseCodeError=10
ResponseCodeRequestTypeText=20
ResponseCodeRequestTypeCursorChar=30
ResponseCodeRequestTypeCursorVisible=40
ResponseCodeRequestTypeNumChars=50

CommandDelimiter=string.char(30)
CommandChunkDelimiter=string.char(31)


DefaultPort = 11111

obs = obslua
local socket = require("ljsocket")

-- Configuration items
local port = DefaultPort
local server = nil
local clients = {}

-- Set true to get debug printing
local debug_print_enabled = true

-- Description displayed in the Scripts dialog window
function script_description()
    return '<h2>Obvious ' .. version ..'</h2>' ..
        [[
           <p>Receiver for Obvious, VST controller for OBS, by Bjørn Felle</p>
        ]]
end

-- Log a message if debugging is enabled
function debug_print(a_string)
    if debug_print_enabled then
        print(a_string)
    end
end

function script_load(settings)

    connectServer()

    obs.timer_add(serverCallback, 10)
    obs.timer_add(clientCallback, 10)

end

function clientCallback()

  local closeClients = {}

  for i, client in ipairs(clients) do

      local str, err = client:receive()

      if str then
          handleClientMessage(client, str)
      elseif err == "closed" then
          table.insert(removeClients, client)
      elseif err ~= "timeout" then
          error(err)
      end

  end

  for i, client in ipairs(closeClients) do
      closeClient(client)
  end

end

function script_unload()

    obs.timer_remove(serverCallback)
    obs.timer_remove(clientCallback)

    disconnectClients()
    disconnectServer()

end

function connectServer()

  if server ~= nil then
      disconnectServer()
  end

  local info = socket.find_first_address("*", port)

  server = assert(socket.create(info.family, info.socket_type, info.protocol))
  server:set_blocking(false)
  assert(server:set_option("nodelay", true, "tcp"))
  assert(server:set_option("reuseaddr", true))
  assert(server:bind(info))
  assert(server:listen())

end

function disconnectServer()

    if server ~= nil then
        print('Shutting down server')
        assert(server:close())
        server = nil
    end

end

function disconnectClients()

    closeClients = {}

    for i, client in ipairs(clients) do
        table.insert(closeClients, client)
    end

    for i, client in ipairs(closeClients) do
        disconnectClient(client)
    end

end

function tableIndexOf(t, val)
    for k,v in ipairs(t) do
        if v == val then return k end
    end
end

function disconnectClient(client)

    local n = tableIndexOf(clients, client)
    local host, port = client:get_peer_name()
    print("Disconnecting client @ " .. host .. ":" .. port)
    assert(client:close())
    table.remove(clients, n)

end

function script_defaults(settings)
    obs.obs_data_set_default_int(settings, "Port", DefaultPort)
end

function script_properties()
    local props = obs.obs_properties_create()
    obs.obs_properties_add_int(props, "Port", "Port", 0, 99999, 1)
    return props
end

function script_update(settings)
    port = obs.obs_data_get_int(settings, "Port")
    disconnectClients()
    disconnectServer()
    connectServer()
end

function script_save(settings)

end

local function clientSend(client, message, responseCode)
    if client then client:send(responseCode .. CommandChunkDelimiter .. message .. CommandDelimiter) end
end

local function printAndSend(client, message, responseCode)
    print("Error code " .. responseCode .. ": " .. message)
    clientSend(client, message, responseCode)
end

local function findSceneItem(client, sceneName, sourceName)
    local sceneSource = obs.obs_get_source_by_name(sceneName)
    if sceneSource then

        local scene = obs.obs_scene_from_source(sceneSource)
        local source = obs.obs_scene_find_source_recursive(scene, sourceName)

        obs.obs_source_release(sceneSource)

        if source then
            return source
        else
            printAndSend(client, "Could not find source named '" .. sourceName .. "' in scene named '" .. sceneName .. "'", ResponseCodeError)
        end


    else
        printAndSend(client, "Could not find scene named '" .. sceneName .. "'", ResponseCodeError)
    end

    return nil
end

local function getSourceFromSceneItem(sceneItem)
    if sceneItem then return obs.obs_sceneitem_get_source(sceneItem) end
    return nil
end

local function findSource(client, sceneName, sourceName)
    local sceneItem = findSceneItem(client, sceneName, sourceName)
    if sceneItem then return obs.obs_sceneitem_get_source(sceneItem) end
end

local function setScale(client, sceneName, sourceName, scaleX, scaleY)
    local source = findSceneItem(client, sceneName, sourceName)
    if source then
        local scale = obs.vec2()
        obs.obs_sceneitem_get_scale(source, scale)
        if scaleX then
            scale.x = scaleX
        end
        if scaleY then
            scale.y = scaleY
        end
        obs.obs_sceneitem_set_scale(source, scale)

    end
end

local function setProportionatePositions(client, sceneName, sourceName, xProportion, yProportion)
    local sceneItem = findSceneItem(client, sceneName, sourceName)
    local source = getSourceFromSceneItem(sceneItem)
    if source then

        local position = obs.vec2()
        obs.obs_sceneitem_get_pos(sceneItem, position)
        -- see also obs_sceneitem_get_crop

        if xProportion then
            local width = obs.obs_source_get_width(source)
            position.x = width * xProportion
        end

        if yProportion then
            local height = obs.obs_source_get_height(source)
            position.y = height * yProportion
        end

        obs.obs_sceneitem_set_pos(sceneItem, position)

    end
end

local function setFilterValue(client, sceneName, sourceName, filterName, filterParameterName, value)

    local source = findSource(client, sceneName, sourceName)
    if source then
        local filter = obs.obs_source_get_filter_by_name(source, filterName)
        if filter then
            local filterSettings = obs.obs_source_get_settings(filter)
            if filterSettings then
                obs.obs_data_set_double(filterSettings, filterParameterName, value)
                obs.obs_source_update(filter, filterSettings)
            else
                printAndSend(client, "Could not get settings for filter named '" .. filterName .. "' on source named '" .. sourceName .. "' in scene named '" .. sceneName .. "'", ResponseCodeError)
            end

            obs.obs_data_release(filterSettings)
            obs.obs_source_release(filter)

        else
            printAndSend(client, "No filter named '" .. filterName .. "' was found on source named '" .. sourceName .. "' in scene named '" .. sceneName .. "'", ResponseCodeError)
        end
    end

end

local function crop(client, sceneName, sourceName, command, value)

    local sceneItem = findSceneItem(client, sceneName, sourceName)
    if sceneItem then
        local crop = obs.obs_sceneitem_crop()
        obs.obs_sceneitem_get_crop(sceneItem, crop)
        if command == CropTop then crop.top = value
        elseif command == CropBottom then crop.bottom = value
        elseif command == CropLeft then crop.left = value
        elseif command == CropRight then crop.right = value
        else
            printAndSend(client, "Invalid crop parameter '" .. command .. "'", ResponseCodeError)
        end
        obs.obs_sceneitem_set_crop(sceneItem, crop)
    end

end

local typeTextDict = {}
local typeNumCharsDict = {}
local cursorCharacterDict = {}
local cursorCharacterVisibleDict = {}

local function typeDictionaryKey(sceneName, sourceName)
    return sceneName .. sourceName
end

local function typeText(client, sceneName, sourceName)

    local source = findSource(client, sceneName, sourceName)
    if source then

        local key = typeDictionaryKey(sceneName, sourceName)
        local fullString = typeTextDict[key]
        if fullString then


            local numChars = typeNumCharsDict[key]
            if numChars then
                -- local reformattedText = string.gsub(fullString, "<SPACE>", " ")
                -- local subText = string.sub(reformattedText, 1, numChars)
                local subText = string.sub(fullString, 1, numChars)

                if cursorCharacterVisibleDict[key] then

                    local cursorCharacter = cursorCharacterDict[key]
                    if cursorCharacter then

                        if cursorCharacterVisibleDict[key] == "yes" then subText = subText .. cursorCharacter end

                        local sourceSettings = obs.obs_source_get_settings(source)
                        if sourceSettings then

                            obs.obs_data_set_string(sourceSettings, "text", subText)
                            obs.obs_source_update(source, sourceSettings)

                            obs.obs_data_release(sourceSettings)

                        else
                            printAndSend(client, "Could not get settings for text source named '" .. sourceName .. "' in scene named '" .. sceneName .. "'", ResponseCodeError, ResponseCodeError)
                        end

                    else
                        print("Requesting cursor character")
                        clientSend(client, ".", ResponseCodeRequestTypeCursorChar)
                    end

                else
                    print("Requesting cursor visibility")
                    clientSend(client, ".", ResponseCodeRequestTypeCursorVisible)
                end



            else
                print("Requesting number of characters to print")
                clientSend(client, ".", ResponseCodeRequestTypeNumChars)
            end
        else
            print("Requesting text to print")
            clientSend(client, ".", ResponseCodeRequestTypeText)
        end

    end

end

function serverCallback()

    if server == nil then
        return
    end

    --if client == nil then
        local client, err = server:accept()
        if client then
            assert(client:set_blocking(false))
            local host, port = client:get_peer_name()
            print("Client connected @ " .. host .. ":" .. port)
            table.insert(clients, client)

        end
    --end

end

function handleClientMessage(client, messagesString)

    messages = {}

    for w in string.gmatch(messagesString, "[^" .. CommandDelimiter .. "]+") do
        table.insert(messages, w)
    end

    for _, message in ipairs(messages) do
    -- for message in values(messages) do
        words = {}

        for w in string.gmatch(message, "[^" .. CommandChunkDelimiter .. "]+") do
            table.insert(words, w)
        end

        -- print(message)

        local commandID = tonumber(words[1])
        local sceneName = words[2]
        local sourceName = words[3]
        local value = tonumber(words[#words])

        if commandID == Heartbeat then

            clientSend(client, ".", HeartbeatResponse)

        elseif commandID == Disconnect then

            disconnectClient(client)

        elseif commandID == PositionProportionateX then

            setProportionatePositions(client, sceneName, sourceName, value, nil)

        elseif commandID == PositionProportionateY then

            setProportionatePositions(client, sceneName, sourceName, nil, value)

        elseif commandID == ScaleX then

            setScale(client, sceneName, sourceName, value, nil)

        elseif commandID == ScaleY then

            setScale(client, sceneName, sourceName, nil, value)

        elseif commandID == MediaRestart then

            if value ~= 1 then return end

            local source = findSource(client, sceneName, sourceName)
            if source then
                obs.obs_source_media_restart(source)
            end

        elseif commandID == MediaStop then

            if value ~= 1 then return end

            local source = findSource(client, sceneName, sourceName)
            if source then
                obs.obs_source_media_stop(source)
            end

        elseif commandID == MediaPlay then

            if value ~= 1 then return end

            local source = findSource(client, sceneName, sourceName)
            if source then
                obs.obs_source_media_play_pause(source, false)
            end

        elseif commandID == MediaPause then

            if value ~= 1 then return end

            local source = findSource(client, sceneName, sourceName)
            if source then
                obs.obs_source_media_play_pause(source, true)
            end

        elseif commandID == MediaCursor then

            local source = findSource(client, sceneName, sourceName)
            if source then
                obs.obs_source_media_set_time(source, value)
            end

        elseif commandID == Hue then

            local filterName = words[4]
            setFilterValue(client, sceneName, sourceName, filterName, "hue_shift", value)

        elseif commandID == Saturation then

            local filterName = words[4]
            setFilterValue(client, sceneName, sourceName, filterName, "saturation", value)

        elseif commandID == RollSpeedH then

            local filterName = words[4]
            setFilterValue(client, sceneName, sourceName, filterName, "speed_x", value)

        elseif commandID == RollSpeedV then

            local filterName = words[4]
            setFilterValue(client, sceneName, sourceName, filterName, "speed_y", value)

        elseif commandID == Opacity then

            local filterName = words[4]
            setFilterValue(client, sceneName, sourceName, filterName, "opacity", value)

        elseif commandID == SetVisible then

            local sceneItem = findSceneItem(client, sceneName, sourceName)
            if sceneItem then
                local visible = false
                if value == 1 then visible = true end
                obs.obs_sceneitem_set_visible(sceneItem, visible)
            end

        elseif commandID == TypeText then

            local text = words[#words]
            typeTextDict[typeDictionaryKey(sceneName, sourceName)] = text
            typeText(client, sceneName, sourceName)

        elseif commandID == TypeSetNumChars then

            typeNumCharsDict[typeDictionaryKey(sceneName, sourceName)] = math.floor(value)
            typeText(client, sceneName, sourceName)

        elseif commandID == TypeSetCursorCharacter then

            local text = words[#words]
            local key = typeDictionaryKey(sceneName, sourceName)
            cursorCharacterDict[key] = text
            typeText(client, sceneName, sourceName)

        elseif commandID == TypeSetCursorVisible then

            local visible = "no"
            if value == 1 then visible = "yes" end
            cursorCharacterVisibleDict[typeDictionaryKey(sceneName, sourceName)] = visible -- visible
            typeText(client, sceneName, sourceName)

        elseif commandID == CropTop or commandID == CropBottom or commandID == CropLeft or commandID == CropRight then

            crop(client, sceneName, sourceName, commandID, value)

        else
            printAndSend(client, "Unknown command ID (" .. commandID .. ")", ResponseCodeError)
        end
    end
end
