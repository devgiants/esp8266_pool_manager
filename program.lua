dofile('lcd/lcd.lua')
-- Init device communication.
-- In this example, pin 3 -> SDA, pin 4 -> SCL, 0x27 is the device I2C address, 20 columns, 4 rows (4x20 LCD display)

constants = {
    UPDATE_INTERVAL = 10000,
    OW_PIN = 2,
    WATER_TEMP_SENSOR_ADDRESS = '28:CA:98:CF:05:00:00:51',
    SDA = 3,
    SCL = 4
}

currentScreen = 1
updateTimer = tmr.create()
poolTemperature = 0
updateDisplay = false

-- For displaying arrows to switch between screens
function displayArrows()
    hd44780.setCursor(3,0)
    hd44780.printString('<')
    hd44780.setCursor(3,19)
    hd44780.printString('>')
end

-- Display indicator between arrows to inform user that data is updating
function toggleUpdateIndicator()
    hd44780.setCursor(3,7)
    if(updateDisplay == false) then        
        hd44780.printString('MAJ...')
        updateDisplay = true
    else        
        hd44780.printString('      ')
        updateDisplay = false
    end
end

-- For displaying different screens
function printScreen(screenId)
    if(screenId == 1) then
        -- Clear available screen part
        hd44780.setCursor(0,0)
        hd44780.clearRow(0)
        hd44780.clearRow(1)
        hd44780.clearRow(2)
        
        -- Display pool temperature
        hd44780.setCursor(0,0)
        hd44780.printString('Tpisc : ' .. tostring(poolTemperature) .. ' C')
    end
end


if(hd44780.init(constants.SDA, constants.SCL, 0x27, 20, 4)) then
    -- Set backlight on
    hd44780.setBacklight(hd44780.BACKLIGHT_ON)
    -- Set cursor 5th columns 1st line
    hd44780.setCursor(0,5)
    -- Print text
    hd44780.printString('SYSTEME DE')
    hd44780.setCursor(1,2)
    hd44780.printString('GESTION PISCINE')
    hd44780.setCursor(3,0)
    hd44780.printString('V1.0')

    tmr.delay(2000000)
    
    -- Display arrows
    hd44780.clear()
    displayArrows()

    -- Sets up OW for DS18B20
    ds18b20.setup(constants.OW_PIN)

    -- Sets the max resolution
    ds18b20.setting({constants.WATER_TEMP_SENSOR_ADDRESS}, 12)
    
    tmr.register(updateTimer, constants.UPDATE_INTERVAL, tmr.ALARM_AUTO, 
        function (t)
            -- Display update
            toggleUpdateIndicator()

            -- Make sensors update
            ds18b20.read(
                function(ind,rom,res,temp,tdec,par)
                    -- Format rom display for comparison
                    formattedRom  = string.format("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",string.match(rom,"(%d+):(%d+):(%d+):(%d+):(%d+):(%d+):(%d+):(%d+)"))                    
                    if constants.WATER_TEMP_SENSOR_ADDRESS == formattedRom then
                        poolTemperature = temp
                    end
                    --print(ind,rom,res,temp,tdec,par)                    
                    --hd44780.printString(tostring() .. ' C')
                end,{});
            -- Print current screen
            printScreen(currentScreen)     

            -- Hide update
            toggleUpdateIndicator()
        end)
    tmr.start(updateTimer)
end
