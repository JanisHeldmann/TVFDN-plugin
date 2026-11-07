-- Run in a separate REAPER instance with an isolated -cfgfile (see README).
local directory = assert(os.getenv("TVFDN_TEST_DIR"), "Set TVFDN_TEST_DIR")
local phase = assert(os.getenv("TVFDN_TEST_PHASE"), "Set TVFDN_TEST_PHASE to write or read")
assert(phase == "write" or phase == "read", "Invalid test phase")
local log = assert(io.open(directory .. "/" .. phase .. ".log", "w"))
local parameters = {
    ["RT_DC"] = true, ["RT_NY"] = true, ["RT_CrossOverFrequency"] = true,
    ["Osc_Frequency"] = true, ["Delay_Factor"] = true, ["Frequency Spread"] = true,
    ["TV Bypassed"] = true, ["Absorption Bypassed"] = true
}

local function test()
    if phase == "write" then
        assert(reaper.CountTracks(0) == 0, "Run write phase in an empty test project")
        reaper.InsertTrackAtIndex(0, false)
        local track = reaper.GetTrack(0, 0)
        reaper.SetMediaTrackInfo_Value(track, "I_NCHAN", 64)
        assert(reaper.TrackFX_AddByName(track, "VST3: TVFDN 64", false, -1) == 0,
            "VST3 unavailable; check the isolated configuration's VST path")
    end
    assert(reaper.CountTracks(0) == 1, "Expected one test track")
    local track = reaper.GetTrack(0, 0)
    assert(reaper.TrackFX_GetCount(track) == 1, "Expected one loaded plugin")
    assert(not reaper.TrackFX_GetOffline(track, 0), "Plugin is offline")
    local _, name = reaper.TrackFX_GetFXName(track, 0, "")
    assert(name:find("VST3:") and name:find("TVFDN 64"), "Wrong plugin: " .. name)
    local _, inputs, outputs = reaper.TrackFX_GetIOSize(track, 0)
    assert(inputs == 64 and outputs == 64, "Expected 64 inputs and outputs")
    local expected = {}
    if phase == "read" then
        for line in io.lines(directory .. "/expected.tsv") do
            local key, value = line:match("^(.-)\t(.+)$")
            expected[key] = tonumber(value)
        end
    end
    local count = 0
    for index = 0, reaper.TrackFX_GetNumParams(track, 0) - 1 do
        local _, parameter = reaper.TrackFX_GetParamName(track, 0, index, "")
        if parameters[parameter] then
            local value = reaper.TrackFX_GetParamNormalized(track, 0, index)
            if phase == "write" then
                assert(reaper.TrackFX_SetParamNormalized(track, 0, index, value < 0.5 and 1 or 0))
                expected[parameter] = reaper.TrackFX_GetParamNormalized(track, 0, index)
                assert(math.abs(value - expected[parameter]) > 0.01, "Parameter did not change")
            else
                assert(expected[parameter] and math.abs(value - expected[parameter]) < 0.0001,
                    string.format("Parameter not restored: %s (expected %s, got %.9g)",
                        parameter, tostring(expected[parameter]), value))
            end
            count = count + 1
        end
    end
    assert(count == 8, "Expected all eight processor parameters")
    if phase == "write" then
        local values = assert(io.open(directory .. "/expected.tsv", "w"))
        for parameter, value in pairs(expected) do values:write(parameter, "\t", string.format("%.17g", value), "\n") end
        values:close()
        reaper.Main_SaveProjectEx(0, directory .. "/state-test.rpp", 8)
        local project = assert(io.open(directory .. "/state-test.rpp", "r"))
        project:close()
    end
    log:write("PASS: ", phase, "; ", reaper.GetAppVersion(), "; VST3 64-in/64-out; eight parameters\n")
end

local ok, err = xpcall(test, debug.traceback)
if not ok then log:write("FAIL: ", err, "\n") end
log:close()
reaper.Main_OnCommand(40004, 0) -- Quit only the isolated test instance.
